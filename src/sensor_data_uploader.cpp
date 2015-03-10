/*
 * Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland
 *
 * Authors: Andreas Fenkart, digitalSTROM ag <andreas.fenkart@dev.digitalstrom.org>
 *
 * This file is part of digitalSTROM Server.
 *
 * digitalSTROM Server is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * digitalSTROM Server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "sensor_data_uploader.h"

#include <boost/thread/locks.hpp>

#include "event.h"
#include "foreach.h"
#include "model/scenehelper.h"
#include "model/group.h"
#include "webservice_api.h"

namespace dss {

/****************************************************************************/
/* Sensor Log                                                               */
/****************************************************************************/

__DEFINE_LOG_CHANNEL__(SensorLog, lsInfo)

static const char* pp_prio_pending = "prio_pending";
static const char* pp_normal_pending = "normal_pending";
static const char* pp_last_upload = "last_upload";

/*
 * TODO figure out how to reuse PropertyProxyMemberFunction
 */
class PendingEventsProxy : public PropertyProxy<int> {
public:
  PendingEventsProxy(const SensorLog::m_events_t &queue) : m_events(queue) {}
  virtual int getValue() const {
    return static_cast<int>(m_events.size());
  }
  virtual void setValue(int value) {/* readonly */}
  virtual PropertyProxy<int>* clone() const {
    return new PendingEventsProxy(m_events);
  }
private:
  const SensorLog::m_events_t &m_events;
};

SensorLog::SensorLog(const std::string hubName, Uploader *uploader)
  : m_pending_upload(false), m_hubName(hubName), m_uploader(uploader),
    m_upload_run(0), m_lastUpload("never")
{
  m_propFolder =
    DSS::getInstance()->getPropertySystem().createProperty("/system/" + hubName + "/eventlog");

  m_propFolder->createProperty(pp_last_upload)
    ->linkToProxy(PropertyProxyReference<std::string>(m_lastUpload, false));
  m_propFolder->createProperty(pp_prio_pending)
    ->linkToProxy(PendingEventsProxy(m_eventsHighPrio));
  m_propFolder->createProperty(pp_normal_pending)
    ->linkToProxy(PendingEventsProxy(m_events));
}

SensorLog::~SensorLog()
{
  m_propFolder->removeChild(m_propFolder->getPropertyByName(pp_last_upload));
  m_propFolder->removeChild(m_propFolder->getPropertyByName(pp_prio_pending));
  m_propFolder->removeChild(m_propFolder->getPropertyByName(pp_normal_pending));
  m_propFolder->getParentNode()->removeChild(m_propFolder);
}

/**
 * must hold m_lock when being calling
 */
void SensorLog::packet_append(std::vector<boost::shared_ptr<Event> > &events)
{
  // how much space left in the packet
  size_t free = std::distance(m_packet.begin(), m_packet.end());
  size_t space = std::min(events.size(), max_post_events - free);
  m_packet.insert(m_packet.end(), events.begin(), events.begin() + space);
  events.erase(events.begin(), events.begin() + space);
}

/**
 * @next -- send followup package
 */
void SensorLog::send_packet(bool next) {
  boost::mutex::scoped_lock lock(m_lock);
  log("[" + m_hubName + "] send_packet: start", lsDebug);
  if (next) {
    m_upload_run += m_packet.size();
    m_packet.clear();
  } else {
    // not clear packet, might be retry
    m_upload_run = 0;
  }

  if (m_eventsHighPrio.empty()) {
    // never stop uploading as long high priority events are pending
    if (m_events.empty() && m_packet.empty()) {
      // no retries needed and no more events to upload
      m_pending_upload = false;
      log("[" + m_hubName + "] send_packet: nothing to upload", lsDebug);
      return;
    } else if ((m_upload_run % max_post_events != 0) &&
               (m_events.size() < max_post_events)) {
      //
      // one of the previous packets was not full and
      // this packet wouldn't be a full either. hence two
      // non-full packets in this run.
      // delay upload till next trigger
      //
      m_pending_upload = false;
      log("[" + m_hubName + "] send_packet: tickle prevention, no upload", lsDebug);
      return;
    }
  }

  packet_append(m_eventsHighPrio);
  packet_append(m_events);
  assert(!m_packet.empty());

  lock.unlock(); // upload call might trigger callback immediately
  if (!m_uploader->upload(m_packet.begin(), m_packet.end(), shared_from_this())) {
    boost::mutex::scoped_lock lock(m_lock);
    m_pending_upload = false;
  }
}


void SensorLog::append(boost::shared_ptr<Event> event, bool highPrio) {
  boost::mutex::scoped_lock lock(m_lock);

  if (highPrio) {

    if (m_eventsHighPrio.size() >= max_prio_elements) {
      // should not happen, but prevent unlimited memory usage
      log("[" + m_hubName + "] event overflow discarding prio event: " +
          event->getName(), lsWarning);
      return;
    }
    log("[" + m_hubName + "] append: add high prio event: " + event->getName(), lsDebug);
    m_eventsHighPrio.push_back(event);
    lock.unlock();
    triggerUpload();
    return;
  }

  if (m_events.size() + m_packet.size() >= max_elements) {
    // MS-Hub will detect from jumps in sequence id
    log("[" + m_hubName + "] event overflow, discarding: " + event->getName(),
        lsWarning);
    return;
  }
  log("[" + m_hubName + "] append: add event: " + event->getName(), lsDebug);
  m_events.push_back(event);
}

void SensorLog::triggerUpload() {
  boost::mutex::scoped_lock lock(m_lock);

  if (m_pending_upload) {
    log("[" + m_hubName + "] triggerUpload: upload still pending", lsDebug);
    return;
  }

  if (m_packet.empty() && m_eventsHighPrio.empty() && m_events.empty()) {
    log("[" + m_hubName + "] triggerUpload: all queues empty", lsDebug);
    return;
  }

  m_pending_upload = true;

  lock.unlock();
  send_packet();
}

void SensorLog::done(RestTransferStatus_t status, WebserviceReply reply) {
  //
  // MS-Hub, will detect jumps in sequence ID, when we throw away events
  //
  log("[" + m_hubName + "] upload done", lsDebug);
  switch (status) {
  case NETWORK_ERROR:
    // keep events in the upload queue, retry with next tick
    log("[" + m_hubName + "] network problem", lsWarning);
    {
      boost::mutex::scoped_lock lock(m_lock);
      m_pending_upload = false;
    }
    return;

  case JSON_ERROR:
    // well, retrying will probably not help...
    log("[" + m_hubName + "] failed to parse webservice reply", lsWarning);
    send_packet(true);
    return;

  case REST_OK:
    m_lastUpload = DateTime().toISO8601();
    if (reply.code) {
      // the webservice dislikes our request, scream for HELP and continue.
      log("[" + m_hubName + "] webservice problem: " + intToString(reply.code) +
          "/" + reply.desc, lsWarning);
      send_packet(true);
    } else {
      // the normal case
      send_packet(true);
    }
    break;
  }
}

/****************************************************************************/
/* Sensor Data Upload Ms Hub Plugin                                         */
/****************************************************************************/

bool MSUploadWrapper::upload(SensorLog::It begin, SensorLog::It end,
                             WebserviceCallDone_t callback) {
  return WebserviceMsHub::doUploadSensorData<SensorLog::It>(begin, end, callback);
};

__DEFINE_LOG_CHANNEL__(SensorDataUploadMsHubPlugin, lsInfo);

SensorDataUploadMsHubPlugin::SensorDataUploadMsHubPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload_ms_hub", _pInterpreter),
    m_log(boost::make_shared<SensorLog>("mshub", &m_uploader)),
    m_subscribed(false)
{
  m_websvcActive =
    DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_mshub_active);
  m_websvcActive ->addListener(this);
}

SensorDataUploadMsHubPlugin::~SensorDataUploadMsHubPlugin()
{ }

namespace EventName {
  std::string UploadMsHubEventLog = "upload_ms_hub_event_log";
}

void SensorDataUploadMsHubPlugin::scheduleBatchUploader() {
  log(std::string(__func__) + " start uploading", lsInfo);
  boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::UploadMsHubEventLog);
  pEvent->setProperty(EventProperty::ICalStartTime, DateTime().toRFC2445IcalDataTime());
  int batchDelay = DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_event_batch_delay)->getIntegerValue();
  pEvent->setProperty(EventProperty::ICalRRule, "FREQ=SECONDLY;INTERVAL=" + intToString(batchDelay));
  DSS::getInstance()->getEventQueue().pushEvent(pEvent);
}

void SensorDataUploadMsHubPlugin::propertyChanged(PropertyNodePtr _caller,
                                             PropertyNodePtr _changedNode) {
  if (_changedNode->getBoolValue() == true) {
    doSubscribe();
    scheduleBatchUploader();
  } else {
    log(std::string(__func__) + " stop uploading", lsInfo);
    DSS::getInstance()->getEventRunner().removeEventByName(EventName::UploadMsHubEventLog);
    doUnsubscribe();
  }
}

void SensorDataUploadMsHubPlugin::doSubscribe() {
  if (m_subscribed) {
    return;
  }
  m_subscribed = true;

  std::vector<std::string> events = getSubscriptionEvents();
  boost::shared_ptr<EventSubscription> subscription;
  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().subscribe(subscription);
  }
}

void SensorDataUploadMsHubPlugin::doUnsubscribe() {
  if (!m_subscribed) {
    return;
  }
  m_subscribed = false;

  std::vector<std::string> events = getSubscriptionEvents();
  boost::shared_ptr<EventSubscription> subscription;
  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().unsubscribe(subscription);
  }
}

std::vector<std::string> SensorDataUploadMsHubPlugin::getSubscriptionEvents() {
  std::vector<std::string> events;

  events = MsHub::uploadEvents();
  events.push_back(EventName::Running);
  events.push_back(EventName::UploadMsHubEventLog);
  return events;
}

void SensorDataUploadMsHubPlugin::handleEvent(Event& _event,
                                              const EventSubscription& _subscription) {

  if (!WebserviceMsHub::isAuthorized()) {
    // no upload when webservice is disabled
    return;
  }

  try {

    /* #7725
     * EventName::DeviceSensorValue
     * EventName::ZoneSensorValue with SensorType != 50 or 51
     */
    bool highPrio = true;
    if (_event.getName() == EventName::DeviceSensorValue) {
      highPrio = false;
    }
    if (_event.getName() == EventName::ZoneSensorValue) {
      int sensorType = strToInt(_event.getPropertyByName("sensorType"));
      switch (sensorType) {
        case SensorIDRoomTemperatureControlVariable:
        case SensorIDRoomTemperatureSetpoint:
          break;
        default:
          highPrio = false;
          break;
      }
    }

    if (_event.getName() == EventName::Running) {
      scheduleBatchUploader();
    } else if (_event.getName() == EventName::DeviceSensorValue ||
               _event.getName() == EventName::ZoneSensorValue ||
               _event.getName() == EventName::ZoneSensorError ||
               _event.getName() == EventName::DeviceStatus ||
               _event.getName() == EventName::DeviceInvalidSensor ||
               _event.getName() == EventName::HeatingEnabled ||
               _event.getName() == EventName::HeatingControllerSetup ||
               _event.getName() == EventName::HeatingControllerValue ||
               _event.getName() == EventName::HeatingControllerState ||
               _event.getName() == EventName::AddonToCloud) {
      m_log->append(_event.getptr(), highPrio);

    } else if (_event.getName() == EventName::CallScene ||
               _event.getName() == EventName::UndoScene) {
      if (_event.getRaiseLocation() == erlGroup) {
        boost::shared_ptr<const Group> pGroup = _event.getRaisedAtGroup();
        if (pGroup->getID() != GroupIDControlTemperature) {
          // ignore
          return;
        }
        if (_event.hasPropertySet("sceneID")) {
          // limited set of allowed operation modes on the temperature control group
          int sceneID = strToInt(_event.getPropertyByName("sceneID"));
          if (sceneID < 0 || sceneID > 15) {
            return;
          }
        }
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::StateChange ||
               _event.getName() == EventName::OldStateChange) {
      std::string sName = _event.getPropertyByName("statename");
      if (sName == "holiday" || sName == "presence") {
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::AddonStateChange) {
      std::string sName = _event.getPropertyByName("scriptID");
      if (sName == "heating-controller") {
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::UploadMsHubEventLog) {
      m_log->triggerUpload();
    } else {
      log("Unhandled event " + _event.getName(), lsInfo);
    }
  } catch (std::runtime_error &e) {
    log(std::string("exception ") + e.what(), lsWarning);
  }
}


/****************************************************************************/
/* Sensor Data Upload dS Hub Plugin                                         */
/****************************************************************************/

bool DSUploadWrapper::upload(SensorLog::It begin, SensorLog::It end,
                             WebserviceCallDone_t callback) {
  return WebserviceDsHub::doUploadSensorData<SensorLog::It>(begin, end, callback);
};

__DEFINE_LOG_CHANNEL__(SensorDataUploadDsHubPlugin, lsInfo);

SensorDataUploadDsHubPlugin::SensorDataUploadDsHubPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload_ds_hub", _pInterpreter),
    m_log(boost::make_shared<SensorLog>("dshub", &m_uploader)),
    m_subscribed(false)
{
  m_websvcActive =
    DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_dshub_active);
  m_websvcActive ->addListener(this);

}

SensorDataUploadDsHubPlugin::~SensorDataUploadDsHubPlugin()
{ }

namespace EventName {
  std::string UploadDsHubEventLog = "upload_ds_hub_event_log";
}

void SensorDataUploadDsHubPlugin::scheduleBatchUploader() {
  log(std::string(__func__) + " start uploading", lsInfo);
  boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::UploadDsHubEventLog);
  pEvent->setProperty(EventProperty::ICalStartTime, DateTime().toRFC2445IcalDataTime());
  int batchDelay = DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_event_batch_delay)->getIntegerValue();
  pEvent->setProperty(EventProperty::ICalRRule, "FREQ=SECONDLY;INTERVAL=" + intToString(batchDelay));
  DSS::getInstance()->getEventQueue().pushEvent(pEvent);
}

void SensorDataUploadDsHubPlugin::propertyChanged(PropertyNodePtr _caller,
                                             PropertyNodePtr _changedNode) {
  if (_changedNode->getBoolValue() == true) {
    doSubscribe();
    scheduleBatchUploader();
  } else {
    log(std::string(__func__) + " stop uploading", lsInfo);
    DSS::getInstance()->getEventRunner().removeEventByName(EventName::UploadDsHubEventLog);
    doUnsubscribe();
  }
}

void SensorDataUploadDsHubPlugin::subscribe() {
  if (DSS::getInstance()->getPropertySystem().getBoolValue(pp_websvc_dshub_active)) {
    doSubscribe();
  }
}

void SensorDataUploadDsHubPlugin::doSubscribe() {
  if (m_subscribed) {
    return;
  }
  m_subscribed = true;

  std::vector<std::string> events = getSubscriptionEvents();

  boost::shared_ptr<EventSubscription> subscription;
  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().subscribe(subscription);
  }
}

void SensorDataUploadDsHubPlugin::doUnsubscribe() {
  if (!m_subscribed) {
    return;
  }
  m_subscribed = false;

  std::vector<std::string> events = getSubscriptionEvents();
  boost::shared_ptr<EventSubscription> subscription;
  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().unsubscribe(subscription);
  }
}

std::vector<std::string> SensorDataUploadDsHubPlugin::getSubscriptionEvents() {
  std::vector<std::string> events;

  events = DsHub::uploadEvents();
  events.push_back(EventName::Running);
  events.push_back(EventName::UploadDsHubEventLog);
  return events;
}

void SensorDataUploadDsHubPlugin::handleEvent(Event& _event,
                                              const EventSubscription& _subscription) {

  if (!WebserviceDsHub::isAuthorized()) {
    // no upload when webservice is disabled
    return;
  }

  try {

    /* #7725
     * EventName::DeviceSensorValue
     * EventName::ZoneSensorValue with SensorType != 50 or 51
     */
    bool highPrio = false;
    if (_event.getName() == EventName::DeviceSensorValue) {
      highPrio = false;
    }

    if (_event.getName() == EventName::ZoneSensorValue) {
      int sensorType = strToInt(_event.getPropertyByName("sensorType"));
      switch (sensorType) {
        case SensorIDRoomTemperatureControlVariable:
        case SensorIDRoomTemperatureSetpoint:
          break;
        default:
          highPrio = false;
          break;
      }
    }

    if (_event.getName() == EventName::Running) {
      scheduleBatchUploader();
    } else if (_event.getName() == EventName::DeviceBinaryInputEvent ||
               _event.getName() == EventName::DeviceSensorValue ||
               _event.getName() == EventName::ZoneSensorValue ||
               _event.getName() == EventName::ZoneSensorError ||
               _event.getName() == EventName::DeviceStatus ||
               _event.getName() == EventName::DeviceInvalidSensor ||
               _event.getName() == EventName::HeatingEnabled ||
               _event.getName() == EventName::HeatingControllerSetup ||
               _event.getName() == EventName::HeatingControllerValue ||
               _event.getName() == EventName::HeatingControllerState ||
               _event.getName() == EventName::AddonToCloud ||
               _event.getName() == EventName::ExecutionDenied ||
               _event.getName() == EventName::LogFileData) {
      m_log->append(_event.getptr(), highPrio);
    } else if (_event.getName() == EventName::CallScene ||
               _event.getName() == EventName::UndoScene) {
      if (_event.getRaiseLocation() == erlGroup) {
        boost::shared_ptr<const Group> pGroup = _event.getRaisedAtGroup();
        if (pGroup->getID() != GroupIDControlTemperature) {
          // ignore
          return;
        }
        if (_event.hasPropertySet("sceneID")) {
          // limited set of allowed operation modes on the temperature control group
          int sceneID = strToInt(_event.getPropertyByName("sceneID"));
          if (sceneID < 0 || sceneID > 15) {
            return;
          }
        }
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::StateChange ||
               _event.getName() == EventName::OldStateChange) {
      std::string sName = _event.getPropertyByName("statename");
      if (sName == "holiday" || sName == "presence") {
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::AddonStateChange) {
      std::string sName = _event.getPropertyByName("scriptID");
      if (sName == "heating-controller") {
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::UploadDsHubEventLog) {
      m_log->triggerUpload();
    } else {
      log("Unhandled event " + _event.getName(), lsInfo);
    }

  } catch (std::runtime_error &e) {
    log(std::string("exception ") + e.what(), lsWarning);
  }
}
}

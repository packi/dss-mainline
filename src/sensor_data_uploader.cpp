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
  m_uploader->upload(m_packet.begin(), m_packet.end(), shared_from_this());
}


void SensorLog::append(boost::shared_ptr<Event> event, bool highPrio) {
  boost::mutex::scoped_lock lock(m_lock);

  if (highPrio) {
    log("[" + m_hubName + "] append: add high prio event: " + event->getName(), lsDebug);
    m_eventsHighPrio.push_back(event);
    lock.unlock();
    triggerUpload();
    return;
  }

  if (m_events.size() + m_packet.size() > max_elements) {
    // MS-Hub will detect from jumps in sequence id
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

void MSUploadWrapper::upload(SensorLog::It begin, SensorLog::It end,
                             WebserviceCallDone_t callback) {
  WebserviceMsHub::doUploadSensorData<SensorLog::It>(begin, end, callback);
};

__DEFINE_LOG_CHANNEL__(SensorDataUploadMsHubPlugin, lsInfo);

SensorDataUploadMsHubPlugin::SensorDataUploadMsHubPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload_ms_hub", _pInterpreter),
    m_log(boost::make_shared<SensorLog>("mshub", &m_uploader))
{
  websvcEnabledNode =
    DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_enabled);
  websvcEnabledNode ->addListener(this);
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
  // initiate connection as soon as webservice got enabled
  if (_changedNode->getBoolValue() == true) {
    scheduleBatchUploader();
  } else {
    log(std::string(__func__) + " stop uploading", lsInfo);
    DSS::getInstance()->getEventRunner().removeEventByName(EventName::UploadMsHubEventLog);
  }
}

void SensorDataUploadMsHubPlugin::subscribe() {
  boost::shared_ptr<EventSubscription> subscription;

  std::vector<std::string> events;
  events.push_back(EventName::DeviceSensorValue);
  events.push_back(EventName::DeviceStatus);
  events.push_back(EventName::DeviceInvalidSensor);
  events.push_back(EventName::ZoneSensorValue);
  events.push_back(EventName::ZoneSensorError);
  events.push_back(EventName::CallScene);
  events.push_back(EventName::UndoScene);
  events.push_back(EventName::StateChange);
  events.push_back(EventName::AddonStateChange);
  events.push_back(EventName::HeatingEnabled);
  events.push_back(EventName::HeatingControllerSetup);
  events.push_back(EventName::HeatingControllerValue);
  events.push_back(EventName::HeatingControllerState);
  events.push_back(EventName::Running);
  events.push_back(EventName::UploadMsHubEventLog);
  events.push_back(EventName::OldStateChange);
  events.push_back(EventName::AddonToCloud);

  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().subscribe(subscription);
  }
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

void DSUploadWrapper::upload(SensorLog::It begin, SensorLog::It end,
                             WebserviceCallDone_t callback) {
  WebserviceDsHub::doUploadSensorData<SensorLog::It>(begin, end, callback);
};

__DEFINE_LOG_CHANNEL__(SensorDataUploadDsHubPlugin, lsInfo);

SensorDataUploadDsHubPlugin::SensorDataUploadDsHubPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload_ds_hub", _pInterpreter),
    m_log(boost::make_shared<SensorLog>("dshub", &m_uploader))
{
  websvcEnabledNode =
    DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_enabled);
  websvcEnabledNode ->addListener(this);
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
  // initiate connection as soon as webservice got enabled
  if (_changedNode->getBoolValue() == true) {
    scheduleBatchUploader();
  } else {
    log(std::string(__func__) + " stop uploading", lsInfo);
    DSS::getInstance()->getEventRunner().removeEventByName(EventName::UploadDsHubEventLog);
  }
}

void SensorDataUploadDsHubPlugin::subscribe() {
  boost::shared_ptr<EventSubscription> subscription;

  std::vector<std::string> events;
  events.push_back(EventName::DeviceBinaryInputEvent);
  events.push_back(EventName::DeviceSensorValue);
  events.push_back(EventName::DeviceStatus);
  events.push_back(EventName::DeviceInvalidSensor);
  events.push_back(EventName::ExecutionDenied);
  events.push_back(EventName::ZoneSensorValue);
  events.push_back(EventName::ZoneSensorError);
  events.push_back(EventName::CallScene);
  events.push_back(EventName::UndoScene);
  events.push_back(EventName::StateChange);
  events.push_back(EventName::AddonStateChange);
  events.push_back(EventName::HeatingEnabled);
  events.push_back(EventName::HeatingControllerSetup);
  events.push_back(EventName::HeatingControllerValueDsHub);
  events.push_back(EventName::HeatingControllerState);
  events.push_back(EventName::Running);
  events.push_back(EventName::UploadDsHubEventLog);
  events.push_back(EventName::OldStateChange);
  events.push_back(EventName::AddonToCloud);
  events.push_back(EventName::LogFileData);

  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().subscribe(subscription);
  }
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

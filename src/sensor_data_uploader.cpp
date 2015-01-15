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

#include "sensor_data_uploader.h"

#include <boost/thread/locks.hpp>

#include "event.h"
#include "foreach.h"
#include "model/scenehelper.h"
#include "model/group.h"
#include "webservice_api.h"

namespace dss {

typedef std::vector<boost::shared_ptr<Event> >::iterator It;

typedef void (*doUploadSensorDataFunction)(It begin, It end, WebserviceCallDone_t callback);


/****************************************************************************/
/* Sensor Log                                                               */
/****************************************************************************/

class SensorLog : public WebserviceCallDone,
                  public boost::enable_shared_from_this<SensorLog> {
  __DECL_LOG_CHANNEL__
  enum {
    max_post_events = 50,
    max_elements = 10000,
  };
public:
  SensorLog(const std::string hubName, doUploadSensorDataFunction doUpload)
    : m_pending_upload(false), m_hubName(hubName), m_doUpload(doUpload) {};
  virtual ~SensorLog() {};
  void append(boost::shared_ptr<Event> event, bool highPrio = false);
  void triggerUpload();
  void done(RestTransferStatus_t status, WebserviceReply reply);
private:
  std::vector<boost::shared_ptr<Event> > m_events;
  std::vector<boost::shared_ptr<Event> > m_eventsHighPrio;
  std::vector<boost::shared_ptr<Event> > m_uploading;
  boost::mutex m_lock;
  bool m_pending_upload;
  const std::string m_hubName;
  doUploadSensorDataFunction m_doUpload;
};

__DEFINE_LOG_CHANNEL__(SensorLog, lsDebug)

void SensorLog::append(boost::shared_ptr<Event> event, bool highPrio) {
  boost::mutex::scoped_lock lock(m_lock);

  if (highPrio) {
    m_eventsHighPrio.push_back(event);
    lock.unlock();
    triggerUpload();
    return;
  }

  if (m_events.size() + m_uploading.size() > max_elements) {
    // MS-Hub will detect from jumps in sequence id
    return;
  }
  m_events.push_back(event);
}

void SensorLog::triggerUpload() {
  boost::mutex::scoped_lock lock(m_lock);

  if (m_pending_upload) {
    return;
  }

  if (m_eventsHighPrio.size()) {
    m_uploading.insert(m_uploading.end(), m_eventsHighPrio.begin(), m_eventsHighPrio.end());
    m_eventsHighPrio.clear();
  } else {
    m_uploading.insert(m_uploading.end(), m_events.begin(), m_events.end());
    m_events.clear();
  }

  if (m_uploading.empty()) {
    return;
  }

  It chunk_start = m_uploading.begin();
  It chunk_end = m_uploading.begin();

  m_pending_upload = true;

  while (chunk_end != m_uploading.end()) {
    size_t remainder = std::distance(chunk_start, m_uploading.end());
    if (remainder > max_post_events) {
      remainder = max_post_events;
    }

    chunk_end = chunk_start + remainder;
    m_doUpload(chunk_start, chunk_end, shared_from_this());
    if (chunk_end != m_uploading.end()) {
      chunk_start = chunk_end;
    }
  }
}

void SensorLog::done(RestTransferStatus_t status, WebserviceReply reply) {
  boost::mutex::scoped_lock lock(m_lock);

  if (!status && !reply.code) {
    m_uploading.clear();
    m_pending_upload = false;
  }

  if (status) {
    // keep events in the upload queue, retry with next tick
    log("[" + m_hubName + "] save event network problem: " + intToString(status), lsWarning);
  } else if (reply.code) {
    log("[" + m_hubName + "] save event webservice problem: " + intToString(reply.code) + "/" + reply.desc,
        lsWarning);
    m_uploading.clear();
    // MS-Hub, will detect jumps in sequence ID
  }
  m_pending_upload = false;

  // re-schedule the uploader for high prio events
  if (m_eventsHighPrio.size()) {
    lock.unlock();
    triggerUpload();
  }
}


/****************************************************************************/
/* Sensor Data Upload Ms Hub Plugin                                         */
/****************************************************************************/

__DEFINE_LOG_CHANNEL__(SensorDataUploadMsHubPlugin, lsInfo);

SensorDataUploadMsHubPlugin::SensorDataUploadMsHubPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload_ms_hub", _pInterpreter),
    m_log(boost::make_shared<SensorLog>("mshub", WebserviceMsHub::doUploadSensorData<It>))
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
      log(std::string(__func__) + " store event " + _event.getName(), lsDebug);
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
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::StateChange ||
               _event.getName() == EventName::OldStateChange) {
      std::string sName = _event.getPropertyByName("statename");
      if (sName == "holiday" || sName == "presence") {
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::AddonStateChange) {
      std::string sName = _event.getPropertyByName("scriptID");
      if (sName == "heating-controller") {
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::UploadMsHubEventLog) {
      log(std::string(__func__) + " upload event log", lsInfo);
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

__DEFINE_LOG_CHANNEL__(SensorDataUploadDsHubPlugin, lsInfo);

SensorDataUploadDsHubPlugin::SensorDataUploadDsHubPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload_ds_hub", _pInterpreter),
    m_log(boost::make_shared<SensorLog>("dshub", WebserviceDsHub::doUploadSensorData<It>))
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
               _event.getName() == EventName::ExecutionDenied) {
      log(std::string(__func__) + " store event " + _event.getName(), lsDebug);
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
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::StateChange ||
               _event.getName() == EventName::OldStateChange) {
      std::string sName = _event.getPropertyByName("statename");
      if (sName == "holiday" || sName == "presence") {
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::AddonStateChange) {
      std::string sName = _event.getPropertyByName("scriptID");
      if (sName == "heating-controller") {
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr(), highPrio);
      }

    } else if (_event.getName() == EventName::UploadDsHubEventLog) {
      log(std::string(__func__) + " upload event log", lsInfo);
      m_log->triggerUpload();
    } else {
      log("Unhandled event " + _event.getName(), lsInfo);
    }

  } catch (std::runtime_error &e) {
    log(std::string("exception ") + e.what(), lsWarning);
  }
}

}

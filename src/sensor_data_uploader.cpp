/*
 * Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland
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

#include <boost/thread/lock_guard.hpp>

#include "event.h"
#include "foreach.h"
#include "model/apartment.h"
#include "model/device.h"
#include "model/devicereference.h"
#include "model/modelmaintenance.h"
#include "model/scenehelper.h"
#include "model/zone.h"
#include "webservice_api.h"
#include "web/json.h"

namespace dss {

JSONObject toJson(const boost::shared_ptr<Event> &event);

class SensorLog : public WebserviceCallDone,
                  public boost::enable_shared_from_this<SensorLog> {
  __DECL_LOG_CHANNEL__
  enum {
    max_elements = 10000,
  };
public:
  SensorLog() : m_pending_upload(false) {};
  void append(boost::shared_ptr<Event> event);
  void triggerUpload();
  void done(RestTransferStatus_t status, WebserviceReply reply);
private:
  std::vector<boost::shared_ptr<Event> > m_events;
  std::vector<boost::shared_ptr<Event> > m_uploading;
  boost::mutex m_lock;
  bool m_pending_upload;
};

__DEFINE_LOG_CHANNEL__(SensorLog, lsDebug)

void SensorLog::append(boost::shared_ptr<Event> event) {
  boost::mutex::scoped_lock lock(m_lock);
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

  m_uploading.insert(m_uploading.end(), m_events.begin(), m_events.end());
  m_events.clear();

  WebserviceApartment::doUploadSensorData(m_uploading.begin(), m_uploading.end(),
                                          shared_from_this());
}

void SensorLog::done(RestTransferStatus_t status, WebserviceReply reply) {
  boost::mutex::scoped_lock lock(m_lock);

  if (!status && !reply.code) {
    m_uploading.clear();
    m_pending_upload = false;
  }

  if (status) {
    // keep retrying, with next tick
    log("network problem: " + intToString(status), lsWarning);
    m_pending_upload = false;
    return;
  }

  if (reply.code) {
    log("webservice problem: " + intToString(reply.code) + "/" + reply.desc,
        lsWarning);
    m_uploading.clear();
    // MS-Hub, will detect jumps in sequence ID
    m_pending_upload = false;
  }
}

__DEFINE_LOG_CHANNEL__(SensorDataUploadPlugin, lsDebug);

SensorDataUploadPlugin::SensorDataUploadPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sensor_data_upload", _pInterpreter),
    m_log(boost::make_shared<SensorLog>()) {
  websvcEnabledNode =
    DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_enabled);
  websvcEnabledNode ->addListener(this);
}

SensorDataUploadPlugin::~SensorDataUploadPlugin()
{ }

namespace EventName {
  std::string UploadEventLog = "upload_event_log";
}

void SensorDataUploadPlugin::propertyChanged(PropertyNodePtr _caller,
                                             PropertyNodePtr _changedNode) {
  // initiate connection as soon as webservice got enabled
  if (_changedNode->getBoolValue() == true) {
    log(std::string(__func__) + " start uploading", lsInfo);
    boost::shared_ptr<Event> pEvent(new Event(EventName::UploadEventLog));
    pEvent->setProperty(EventProperty::ICalStartTime,
                        DateTime().toRFC2445IcalDataTime());
    pEvent->setProperty(EventProperty::ICalRRule, "FREQ=SECONDLY;INTERVAL=60");
    DSS::getInstance()->getEventQueue().pushEvent(pEvent);
  } else {
    log(std::string(__func__) + " stop uploading", lsInfo);
    DSS::getInstance()->getEventRunner().removeEventByName(EventName::UploadEventLog);
  }
}

void SensorDataUploadPlugin::subscribe() {
  boost::shared_ptr<EventSubscription> subscription;

  std::vector<std::string> events;
  events.push_back(EventName::DeviceSensorValue);
  events.push_back(EventName::DeviceStatus);
  events.push_back(EventName::ZoneSensorValue);
  events.push_back(EventName::CallScene);
  events.push_back(EventName::UndoScene);
  events.push_back(EventName::StateChange);
  events.push_back(EventName::HeatingControllerSetup);
  events.push_back(EventName::Running);
  events.push_back(EventName::UploadEventLog);

  foreach (std::string name, events) {
    subscription.reset(new EventSubscription(name, getName(),
                                             getEventInterpreter(),
                                             boost::shared_ptr<SubscriptionOptions>()));
    getEventInterpreter().subscribe(subscription);
  }
}

void SensorDataUploadPlugin::handleEvent(Event& _event,
                                         const EventSubscription& _subscription) {
  try {
    if (_event.getName() == EventName::Running &&
        webservice_communication_authorized()) {

      log(std::string(__func__) + " install ical rules", lsInfo);
      boost::shared_ptr<Event> pEvent(new Event(EventName::UploadEventLog));
      pEvent->setProperty(EventProperty::ICalStartTime,
                          DateTime().toRFC2445IcalDataTime());
      pEvent->setProperty(EventProperty::ICalRRule, "FREQ=SECONDLY;INTERVAL=60");
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);

    } else if (_event.getName() == EventName::DeviceSensorValue ||
               _event.getName() == EventName::ZoneSensorValue ||
               _event.getName() == EventName::DeviceStatus ||
               _event.getName() == EventName::StateChange ||
               _event.getName() == EventName::HeatingControllerSetup) {
      log(std::string(__func__) + " store event " + _event.getName(), lsDebug);
      m_log->append(_event.getptr());

    } else if (_event.getName() == EventName::CallScene ||
               _event.getName() == EventName::UndoScene) {
      if (strToInt(_event.getPropertyByName("groupID")) !=
          GroupIDControlTemperature) {
        // ignore
        return;
      }

      log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
      m_log->append(_event.getptr());

    } else if (_event.getName() == EventName::UploadEventLog) {
      log(std::string(__func__) + " upload event log", lsDebug);
      m_log->triggerUpload();
    } else {
      log("Unhandled event " + _event.getName(), lsInfo);
    }
  } catch (std::runtime_error &e) {
    log(std::string("exception ") + e.what(), lsWarning);
  }
}

enum DSEnum_EventCategory {
  EventCategory_ApartmentSyncEvent = 1,
  EventCategory_ApartmentEvent = 2,
  EventCategory_ZoneEvent = 3,
  EventCategory_DeviceEvent = 4,
  EventCategory_ClusterEvent = 5,
  EventCategory_AddOnEvent = 6,
  EventCategory_ZoneGroupBlink = 7,
  EventCategory_ZoneGroupCallScene = 8,
  EventCategory_ZoneGroupUndoScene = 9,
  EventCategory_ZoneStateGroup = 10,
  EventCategory_DeviceLocal = 11,
  EventCategory_DeviceEventTable = 12,
  EventCategory_DeviceButtonClick = 13,
  EventCategory_DeviceBlink = 14,
  EventCategory_DeviceState = 15,
  EventCategory_DeviceStatus = 16,
  EventCategory_DeviceScene = 17,
  EventCategory_DeviceBinaryInput = 18,
  EventCategory_ModelReady = 19,
  EventCategory_Running = 20,
  EventCategory_StateApartment = 21,
  EventCategory_UserDefinedAction = 22,
  EventCategory_ExecutionDenied = 23,
  EventCategory_SensorValue = 24,
  EventCategory_CircuitMetering = 25,
  EventCategory_DeviceMetering = 26,
  EventCategory_LogFileData = 27,
  EventCategory_HeatingControllerSetup = 28,
  EventCategory_Last = 29,
};

enum DSEnum_EventGroup {
  EventGroup_ApartmentAndDevice = 1,
  EventGroup_Activity = 2,
  EventGroup_Metering = 3,
  EventGroup_ExternalServices = 4,
};

enum DSEnum_EventSource {
  EventSource_dSS = 1,
  EventSource_dSHub = 2,
};

int getSequenceID(int category) {
  // TODO, should be persistent across reboots
  static std::vector<int> sequenceID(EventCategory_Last, 0);
  static boost::mutex m;

  boost::lock_guard<boost::mutex> lock(m);
  if (category >= EventCategory_Last) {
    throw std::runtime_error("invalid category" + intToString(category));
  }
  return sequenceID[category]++;
}

void appendCategory(JSONObject &obj, int category)
{
  if (category >= EventCategory_Last) {
    throw std::runtime_error("invalid category" + intToString(category));
  }

  obj.addProperty("EventCategory", category);
  obj.addProperty("SequenceID", getSequenceID(category));
}

// TODO this should be moved to webservice_api

JSONObject toJson(const boost::shared_ptr<Event> &event) {
  boost::shared_ptr<const DeviceReference> pDeviceRef;
  JSONObject obj;

  if (event->getName() == EventName::DeviceSensorValue) {
    pDeviceRef = event->getRaisedAtDevice();
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Metering);
    appendCategory(obj, EventCategory_DeviceMetering);
    obj.addProperty("SensorIndex", event->getPropertyByName("sensorIndex"));
    int sensorType = strToInt(event->getPropertyByName("sensorType"));
    obj.addProperty("SensorType", SceneHelper::sensorName(sensorType));
    obj.addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
    obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
  } else if (event->getName() == EventName::ZoneSensorValue) {
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Metering);
    appendCategory(obj, EventCategory_ZoneEvent);
    obj.addProperty("GroupID", strToInt(event->getPropertyByName("groupID")));
    obj.addProperty("ZoneID", strToInt(event->getPropertyByName("zoneID")));
    obj.addProperty("SensorType", event->getPropertyByName("sensorType"));
    obj.addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
  } else if (event->getName() == EventName::CallScene) {
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Activity);
    appendCategory(obj, EventCategory_ZoneGroupCallScene);
    obj.addProperty("GroupID", strToInt(event->getPropertyByName("groupID")));
    obj.addProperty("ZoneID", strToInt(event->getPropertyByName("zoneID")));
    obj.addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
    obj.addProperty("forced", event->hasPropertySet("forced"));
    obj.addProperty("Origin", event->getPropertyByName("originDSUID"));
  } else if (event->getName() == EventName::UndoScene) {
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Activity);
    appendCategory(obj, EventCategory_ZoneGroupUndoScene);
    obj.addProperty("GroupID", strToInt(event->getPropertyByName("groupID")));
    obj.addProperty("ZoneID", strToInt(event->getPropertyByName("zoneID")));
    obj.addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
    obj.addProperty("Origin", event->getPropertyByName("originDSUID"));
  } else if (event->getName() == EventName::StateChange) {
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Activity);
    boost::shared_ptr<const State> pState = event->getRaisedAtState();
    switch (pState->getType()) {
      case StateType_Apartment:
      case StateType_Service:
      case StateType_Script:
        appendCategory(obj, EventCategory_ApartmentEvent);
        break;
      case StateType_Group:
        appendCategory(obj, EventCategory_ZoneStateGroup);
        break;
      case StateType_Device:
        appendCategory(obj, EventCategory_DeviceState);
        break;
      default:
        break;
    }
    obj.addProperty("StateName", pState->getName());
    obj.addProperty("StateValue", pState->toString());
    obj.addProperty("Origin", event->getPropertyByName("originDSUID"));
  } else if (event->getName() == EventName::DeviceStatus) {
    pDeviceRef = event->getRaisedAtDevice();
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Activity);
    appendCategory(obj, EventCategory_DeviceStatus);
    obj.addProperty("StatusIndex", event->getPropertyByName("statusIndex"));
    obj.addProperty("StatusValue", event->getPropertyByName("statusValue"));
    obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
  } else if (event->getName() == EventName::HeatingControllerSetup) {
    obj.addProperty("Timestamp", DateTime().toString());
    obj.addProperty("EventGroup", EventGroup_Activity);
    appendCategory(obj, EventCategory_HeatingControllerSetup);

    // TODO: add event data

  } else {
    Logger::getInstance()->log(std::string(__func__) +
                               "unhandled event " + event->getName() + " skip", lsInfo);
  }
  return obj;
}

template <class Iterator>
void WebserviceApartment::doUploadSensorData(Iterator begin, Iterator end,
                                             WebserviceCallDone_t callback) {

  JSONObject obj;
  boost::shared_ptr<JSONArray<JSONObject> > array(new JSONArray<JSONObject>());

  // AppToken ApartmentID Type piggy backed with websvc_connection?
  obj.addProperty("dssid", DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue());
  obj.addProperty("source", "dss" /*EventSource_dSS*/);
  obj.addElement("eventsList", array);
  for (; begin != end; begin++) {
    array->add(toJson(*begin));
  }

  std::string parameters = obj.toString();
  Logger::getInstance()->log(std::string(__func__) + "event data: " + parameters, lsDebug);

  // https://testdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-Event-Event_token_apartmentid_dssid_source
  boost::shared_ptr<StatusReplyChecker> mcb(new StatusReplyChecker(callback));
  WebserviceConnection::getInstance()->request(
      "public/dss/v1_0/Event/Event",
      parameters, POST, mcb, true);
}

typedef std::vector<boost::shared_ptr<Event> >::iterator It;
template void WebserviceApartment::doUploadSensorData<It>(It begin, It end,
                                                          WebserviceCallDone_t
                                                          callback);

}

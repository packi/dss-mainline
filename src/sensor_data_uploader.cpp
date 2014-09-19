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
#include "model/apartment.h"
#include "model/device.h"
#include "model/devicereference.h"
#include "model/modelmaintenance.h"
#include "model/scenehelper.h"
#include "model/group.h"
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
  virtual ~SensorLog() {};
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

  if (!m_uploading.empty()) {
    m_pending_upload = true;
    WebserviceApartment::doUploadSensorData(m_uploading.begin(), m_uploading.end(),
                                            shared_from_this());
  }
}

void SensorLog::done(RestTransferStatus_t status, WebserviceReply reply) {
  boost::mutex::scoped_lock lock(m_lock);

  if (!status && !reply.code) {
    m_uploading.clear();
    m_pending_upload = false;
  }

  if (status) {
    // keep retrying, with next tick
    log("save event network problem: " + intToString(status), lsWarning);
    m_pending_upload = false;
    return;
  }

  if (reply.code) {
    log("save event webservice problem: " + intToString(reply.code) + "/" + reply.desc,
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
  events.push_back(EventName::DeviceInvalidSensor);
  events.push_back(EventName::ZoneSensorValue);
  events.push_back(EventName::CallScene);
  events.push_back(EventName::UndoScene);
  events.push_back(EventName::StateChange);
  events.push_back(EventName::HeatingEnabled);
  events.push_back(EventName::HeatingControllerSetup);
  events.push_back(EventName::HeatingControllerValue);
  events.push_back(EventName::HeatingControllerState);
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
               _event.getName() == EventName::DeviceInvalidSensor ||
               _event.getName() == EventName::HeatingEnabled ||
               _event.getName() == EventName::HeatingControllerSetup ||
               _event.getName() == EventName::HeatingControllerValue ||
               _event.getName() == EventName::HeatingControllerState) {
      log(std::string(__func__) + " store event " + _event.getName(), lsDebug);
      m_log->append(_event.getptr());

    } else if (_event.getName() == EventName::CallScene ||
               _event.getName() == EventName::UndoScene) {
      if (_event.getRaiseLocation() == erlGroup) {
        boost::shared_ptr<const Group> pGroup = _event.getRaisedAtGroup();
        if (pGroup->getID() != GroupIDControlTemperature) {
          // ignore
          return;
        }
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr());
      }

    } else if (_event.getName() == EventName::StateChange) {
      std::string sName = _event.getPropertyByName("statename");
      if (sName == "holiday" || sName == "presence") {
        log(std::string(__func__) + " activity value " + _event.getName(), lsDebug);
        m_log->append(_event.getptr());
      }

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

void appendCommon(JSONObject &obj, const std::string& group, const std::string& category)
{
  obj.addProperty("EventGroup", group);
  obj.addProperty("EventCategory", category);
  obj.addProperty("Timestamp", DateTime().toISO8601_ms());
}

// TODO this should be moved to webservice_api

const static std::string evtGroup_Metering = "Metering";
const static std::string evtGroup_Activity = "Activity";

const static std::string evtCategory_DeviceSensorValue = "DeviceSensorValue";
const static std::string evtCategory_ZoneSensorValue = "ZoneSensorValue";
const static std::string evtCategory_ZoneGroupCallScene = "ZoneGroupCallScene";
const static std::string evtCategory_ZoneGroupUndoScene = "ZoneGroupUndoScene";
const static std::string evtCategory_ApartmentState = "ApartmentState";
const static std::string evtCategory_ZoneGroupState = "ZoneGroupState";
const static std::string evtCategory_DeviceInputState = "DeviceInputState";
const static std::string evtCategory_DeviceStatusReport = "DeviceStatusReport";
const static std::string evtCategory_HeatingControllerSetup = "HeatingControllerSetup";
const static std::string evtCategory_HeatingControllerValue = "HeatingControllerValue";
const static std::string evtCategory_HeatingControllerState = "HeatingControllerState";
const static std::string evtCategory_HeatingEnabled = "HeatingEnabled";

JSONObject toJson(const boost::shared_ptr<Event> &event) {
  boost::shared_ptr<const DeviceReference> pDeviceRef;
  JSONObject obj;
  std::string propValue;

  try {
    if ((event->getName() == EventName::DeviceSensorValue) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(obj, evtGroup_Metering, evtCategory_DeviceSensorValue);
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      obj.addProperty("SensorType", sensorType);
      obj.addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
      obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
      propValue = event->getPropertyByName("sensorIndex");
      if (!propValue.empty()) {
        obj.addProperty("SensorIndex", propValue);
      }
    } else if ((event->getName() == EventName::ZoneSensorValue) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(obj, evtGroup_Metering, evtCategory_ZoneSensorValue);
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      obj.addProperty("SensorType", sensorType);
      obj.addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
      obj.addProperty("ZoneID",  pGroup->getZoneID());
      obj.addProperty("GroupID", pGroup->getID());
    } else if ((event->getName() == EventName::CallScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(obj, evtGroup_Activity, evtCategory_ZoneGroupCallScene);
      obj.addProperty("ZoneID",  pGroup->getZoneID());
      obj.addProperty("GroupID", pGroup->getID());
      obj.addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
      obj.addProperty("Force", event->hasPropertySet("forced"));
      obj.addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if ((event->getName() == EventName::UndoScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(obj, evtGroup_Activity, evtCategory_ZoneGroupUndoScene);
      obj.addProperty("ZoneID",  pGroup->getZoneID());
      obj.addProperty("GroupID", pGroup->getID());
      obj.addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
      obj.addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if (event->getName() == EventName::StateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      switch (pState->getType()) {
      case StateType_Apartment:
      case StateType_Service:
      case StateType_Script:
        appendCommon(obj, evtGroup_Activity, evtCategory_ApartmentState);
        break;
      case StateType_Group:
        appendCommon(obj, evtGroup_Activity, evtCategory_ZoneGroupState);
        break;
      case StateType_Device:
        appendCommon(obj, evtGroup_Activity, evtCategory_DeviceInputState);
        break;
      default:
        break;
      }
      obj.addProperty("StateName", pState->getName());
      obj.addProperty("StateValue", pState->toString());
      obj.addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if ((event->getName() == EventName::DeviceStatus) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(obj, evtGroup_Activity, evtCategory_DeviceStatusReport);
      obj.addProperty("StatusIndex", event->getPropertyByName("statusIndex"));
      obj.addProperty("StatusValue", event->getPropertyByName("statusValue"));
      obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
    } else if ((event->getName() == EventName::DeviceInvalidSensor) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(obj, evtGroup_Activity, evtCategory_DeviceStatusReport);
      obj.addProperty("EventDescription", "InvalidSensorData");
      obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      obj.addProperty("SensorType", sensorType);
      propValue = event->getPropertyByName("sensorIndex");
      if (!propValue.empty()) {
        obj.addProperty("SensorIndex", propValue);
      }
    } else if (event->getName() == EventName::HeatingControllerSetup) {
      appendCommon(obj, evtGroup_Activity, evtCategory_HeatingControllerSetup);
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        obj.addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingControllerValue) {
      appendCommon(obj, evtGroup_Activity, evtCategory_HeatingControllerValue);
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        obj.addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingControllerState) {
      appendCommon(obj, evtGroup_Activity, evtCategory_HeatingControllerState);
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        obj.addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingEnabled) {
      appendCommon(obj, evtGroup_Activity, evtCategory_HeatingEnabled);
      obj.addProperty("ZoneID", strToInt(event->getPropertyByName("zoneID")));
      obj.addProperty("HeatingEnabled", event->getPropertyByName("HeatingEnabled"));

    } else {
      Logger::getInstance()->log(std::string(__func__) + "unhandled event " + event->getName() + ", skip", lsInfo);
    }
  } catch (std::invalid_argument& e) {
    Logger::getInstance()->log(std::string(__func__) + "Error converting event " + event->getName() + ", skip", lsInfo);
  }
  return obj;
}

template <class Iterator>
void WebserviceApartment::doUploadSensorData(Iterator begin, Iterator end,
                                             WebserviceCallDone_t callback) {

  JSONObject obj;
  boost::shared_ptr<JSONArray<JSONObject> > array(new JSONArray<JSONObject>());

  std::string parameters;
  // AppToken is piggy backed with websvc_connection::request(.., authenticated=true)
  parameters += "dssid=" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue();
  parameters += "&source=dss"; /* EventSource_dSS */

  obj.addElement("eventsList", array);
  for (; begin != end; begin++) {
    array->add(toJson(*begin));
  }

  std::string postdata = obj.toString();
  Logger::getInstance()->log(std::string(__func__) + "event data: " + postdata, lsDebug);

  // https://devdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-DSSEventData-SaveEvent_token_apartmentId_dssid_source
  boost::shared_ptr<StatusReplyChecker> mcb(new StatusReplyChecker(callback));
  WebserviceConnection::getInstance()->request("public/dss/v1_0/DSSEventData/SaveEvent",
                                               parameters,
                                               headers_t(),
                                               postdata,
                                               mcb,
                                               true);
}

typedef std::vector<boost::shared_ptr<Event> >::iterator It;
template void WebserviceApartment::doUploadSensorData<It>(It begin, It end,
                                                          WebserviceCallDone_t
                                                          callback);

}

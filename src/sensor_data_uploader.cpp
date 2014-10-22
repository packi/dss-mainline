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

typedef std::vector<boost::shared_ptr<Event> >::iterator It;
JSONObject toJson(const boost::shared_ptr<Event> &event);

class SensorLog : public WebserviceCallDone,
                  public boost::enable_shared_from_this<SensorLog> {
  __DECL_LOG_CHANNEL__
  enum {
    max_post_events = 50,
    max_elements = 10000,
  };
public:
  SensorLog() : m_pending_upload(false) {};
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
    WebserviceApartment::doUploadSensorData(chunk_start, chunk_end,
                                            shared_from_this());
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
    log("save event network problem: " + intToString(status), lsWarning);
  } else if (reply.code) {
    log("save event webservice problem: " + intToString(reply.code) + "/" + reply.desc,
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

__DEFINE_LOG_CHANNEL__(SensorDataUploadPlugin, lsInfo);

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
  events.push_back(EventName::UploadEventLog);
  events.push_back(EventName::OldStateChange);
  events.push_back(EventName::AddonToCloud);

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

    if (_event.getName() == EventName::Running &&
        webservice_communication_authorized()) {

      log(std::string(__func__) + " install ical rules", lsInfo);
      boost::shared_ptr<Event> pEvent(new Event(EventName::UploadEventLog));
      pEvent->setProperty(EventProperty::ICalStartTime,
                          DateTime().toRFC2445IcalDataTime());
      int batchDelay = DSS::getInstance()->getPropertySystem().getProperty(pp_websvc_event_batch_delay)->getIntegerValue();
      pEvent->setProperty(EventProperty::ICalRRule, "FREQ=SECONDLY;INTERVAL=" + intToString(batchDelay));
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
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

    } else if (_event.getName() == EventName::UploadEventLog) {
      log(std::string(__func__) + " upload event log", lsInfo);
      m_log->triggerUpload();
    } else {
      log("Unhandled event " + _event.getName(), lsInfo);
    }
  } catch (std::runtime_error &e) {
    log(std::string("exception ") + e.what(), lsWarning);
  }
}

void appendCommon(JSONObject &obj, const std::string& group,
                  const std::string& category, const Event *event)
{
  obj.addProperty("EventGroup", group);
  obj.addProperty("EventCategory", category);
  obj.addProperty("Timestamp", event->getTimestamp().toISO8601_ms());
}

// TODO this should be moved to webservice_api

const static std::string evtGroup_Metering = "Metering";
const static std::string evtGroup_Activity = "Activity";
const static std::string evtGroup_ApartmentAndDevice = "ApartmentAndDevice";

const static std::string evtCategory_DeviceSensorValue = "DeviceSensorValue";
const static std::string evtCategory_ZoneSensorValue = "ZoneSensorValue";
const static std::string evtCategory_ZoneGroupCallScene = "ZoneSceneCall";
const static std::string evtCategory_ZoneGroupUndoScene = "ZoneUndoScene";
const static std::string evtCategory_ApartmentState = "ApartmentStateChange";
const static std::string evtCategory_ZoneGroupState = "ZoneStateChange";
const static std::string evtCategory_DeviceInputState = "DeviceInputState";
const static std::string evtCategory_DeviceStatusReport = "DeviceStatusReport";
const static std::string evtCategory_DeviceSensorError = "DeviceSensorError";
const static std::string evtCategory_ZoneSensorError = "ZoneSensorError";
const static std::string evtCategory_HeatingControllerSetup = "HeatingControllerSetup";
const static std::string evtCategory_HeatingControllerValue = "HeatingControllerValue";
const static std::string evtCategory_HeatingControllerState = "HeatingControllerState";
const static std::string evtCategory_HeatingEnabled = "HeatingEnabled";
const static std::string evtCategory_AddonToCloud = "AddonToCloud";

const static int dsEnum_SensorError_invalidValue = 1;
const static int dsEnum_SensorError_noValue = 2;

JSONObject toJson(const boost::shared_ptr<Event> &event) {
  boost::shared_ptr<const DeviceReference> pDeviceRef;
  JSONObject obj;
  std::string propValue;

  try {
    if ((event->getName() == EventName::DeviceSensorValue) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(obj, evtGroup_Metering, evtCategory_DeviceSensorValue,
                   event.get());
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
      appendCommon(obj, evtGroup_Metering, evtCategory_ZoneSensorValue,
                   event.get());
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      obj.addProperty("SensorType", sensorType);
      obj.addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
      obj.addProperty("ZoneID",  pGroup->getZoneID());
      obj.addProperty("GroupID", pGroup->getID());
    } else if ((event->getName() == EventName::CallScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(obj, evtGroup_Activity, evtCategory_ZoneGroupCallScene,
                   event.get());
      obj.addProperty("ZoneID",  pGroup->getZoneID());
      obj.addProperty("GroupID", pGroup->getID());
      obj.addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
      obj.addProperty("Force", event->hasPropertySet("forced"));
      obj.addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if ((event->getName() == EventName::UndoScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(obj, evtGroup_Activity, evtCategory_ZoneGroupUndoScene,
                   event.get());
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
        appendCommon(obj, evtGroup_Activity, evtCategory_ApartmentState,
                     event.get());
        break;
      case StateType_Group:
        appendCommon(obj, evtGroup_Activity, evtCategory_ZoneGroupState,
                     event.get());
        break;
      case StateType_Device:
        appendCommon(obj, evtGroup_Activity, evtCategory_DeviceInputState,
                     event.get());
        break;
      default:
        break;
      }
      obj.addProperty("StateName", pState->getName());
      obj.addProperty("StateValue", pState->toString());
      obj.addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if (event->getName() == EventName::OldStateChange) {
      appendCommon(obj, evtGroup_Activity, evtCategory_ApartmentState, event.get());
      obj.addProperty("StateName", event->getPropertyByName("statename"));
      obj.addProperty("StateValue", event->getPropertyByName("state"));
      obj.addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if (event->getName() == EventName::AddonStateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      std::string addonName = event->getPropertyByName("scriptID");
      appendCommon(obj, evtGroup_Activity, evtCategory_ApartmentState, event.get());
      obj.addProperty("StateName", addonName + "." + pState->getName());
      obj.addProperty("StateValue", pState->toString());
      obj.addProperty("Origin", event->getPropertyByName("scriptID"));
    } else if ((event->getName() == EventName::DeviceStatus) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(obj, evtGroup_Activity, evtCategory_DeviceStatusReport,
                   event.get());
      obj.addProperty("StatusIndex", event->getPropertyByName("statusIndex"));
      obj.addProperty("StatusValue", event->getPropertyByName("statusValue"));
      obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
    } else if ((event->getName() == EventName::DeviceInvalidSensor) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(obj, evtGroup_ApartmentAndDevice,
                   evtCategory_DeviceSensorError, event.get());
      obj.addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
      obj.addProperty("SensorType", strToInt(event->getPropertyByName("sensorType")));
      obj.addProperty("StatusCode", dsEnum_SensorError_noValue);
    } else if ((event->getName() == EventName::ZoneSensorError) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(obj, evtGroup_ApartmentAndDevice,
                   evtCategory_ZoneSensorError, event.get());
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      obj.addProperty("SensorType", sensorType);
      obj.addProperty("ZoneID",  pGroup->getZoneID());
      obj.addProperty("GroupID", pGroup->getID());
      obj.addProperty("StatusCode", dsEnum_SensorError_noValue);
    } else if (event->getName() == EventName::HeatingControllerSetup) {
      appendCommon(obj, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerSetup, event.get());
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        obj.addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingControllerValue) {
      appendCommon(obj, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerValue, event.get());
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        obj.addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingControllerState) {
      appendCommon(obj, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerState, event.get());
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        obj.addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingEnabled) {
      appendCommon(obj, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingEnabled, event.get());
      obj.addProperty("ZoneID", strToInt(event->getPropertyByName("zoneID")));
      obj.addProperty("HeatingEnabled", event->getPropertyByName("HeatingEnabled"));

    } else if (event->getName() == EventName::AddonToCloud) {
      appendCommon(obj, evtGroup_Activity, evtCategory_AddonToCloud, event.get());
      obj.addProperty("EventName", event->getPropertyByName("EventName"));
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      boost::shared_ptr<JSONObject> parameterObj(new JSONObject);
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        parameterObj->addProperty(iParam->first, iParam->second);
      }
      obj.addElement("parameter", parameterObj);

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
  int ct = 0;

  std::string parameters;
  // AppToken is piggy backed with websvc_connection::request(.., authenticated=true)
  parameters += "dssid=" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue();
  parameters += "&source=dss"; /* EventSource_dSS */

  obj.addElement("eventsList", array);
  for (; begin != end; begin++) {
    array->add(toJson(*begin));
    ct++;
  }

  std::string postdata = obj.toString();
  // TODO eventually emit summary: 10 callsceene, 27 metering, ...
  // dumping whole postdata leads to log rotation and is seldomly needed
  // unless for solving a blame war with cloud team
  Logger::getInstance()->log(std::string(__func__) + "upload events: " + intToString(ct) + " bytes: " + intToString(postdata.length()), lsInfo);
  Logger::getInstance()->log(std::string(__func__) + "event data: " + postdata, lsDebug);

  // https://devdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-DSSEventData-SaveEvent_token_apartmentId_dssid_source
  boost::shared_ptr<StatusReplyChecker> mcb(new StatusReplyChecker(callback));
  HashMapStringString sensorUploadHeaders;
  sensorUploadHeaders["Content-Type"] = "application/json;charset=UTF-8";

  WebserviceConnection::getInstance()->request("public/dss/v1_0/DSSEventData/SaveEvent",
                                               parameters,
                                               boost::make_shared<HashMapStringString>(sensorUploadHeaders),
                                               postdata,
                                               mcb,
                                               true);
}

template void WebserviceApartment::doUploadSensorData<It>(It begin, It end,
                                                          WebserviceCallDone_t
                                                          callback);

}

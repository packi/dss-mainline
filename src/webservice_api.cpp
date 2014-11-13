#include "webservice_api.h"

#include <json.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include "dss.h"
#include "event.h"
#include "propertysystem.h"
#include "model/group.h"
#include "web/json.h"

namespace dss {

typedef std::vector<boost::shared_ptr<Event> >::iterator ItEvent;

/*
 * Helpers of Ms Hub
 */

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
const static std::string evtCategory_AddonToCloud = "AddOnToCloud";

const static int dsEnum_SensorError_invalidValue = 1;
const static int dsEnum_SensorError_noValue = 2;

void appendCommon(JSONObject &obj, const std::string& group,
                  const std::string& category, const Event *event)
{
  obj.addProperty("EventGroup", group);
  obj.addProperty("EventCategory", category);
  obj.addProperty("Timestamp", event->getTimestamp().toISO8601_ms());
}

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


ParseError::ParseError(const std::string& _message) : runtime_error( _message )
{
}

/**
 * Extract return code and message
 * @param json -- json encoded reply
 * @return decodod struct or throw ParseError if failed
 */
WebserviceReply parse_reply(const char* buf) {
  WebserviceReply resp;
  bool return_code_seen = false;

  if (!buf) {
    throw ParseError("buffer is NULL");
  }

  json_object * jobj = json_tokener_parse(buf);
  if (!jobj) {
    throw ParseError("invalid JSON");
  }
  json_object_object_foreach(jobj, key, val) { /*Passing through every array element*/
    enum json_type type = json_object_get_type(val);
    if (!strcmp(key, "ReturnCode")) {
      if (type != json_type_int) {
        throw ParseError("invalid type for ReturnCode");
      }
      resp.code = json_object_get_int(val);
      return_code_seen = true;
    } else if (!strcmp(key, "ReturnMessage")) {
      if (type == json_type_null) {
        // empty string
        continue;
      }
      if (type != json_type_string) {
        throw ParseError("invalid type for ReturnMessage");
      }
      resp.desc = json_object_get_string(val);
    } else {
      // ignore, unkown keys
    }
  }
  json_object_put(jobj);

  if (!return_code_seen) {
    throw ParseError("missing mandatory element: ReturnCode");
  }

  return resp;
}

__DEFINE_LOG_CHANNEL__(StatusReplyChecker, lsInfo);

void StatusReplyChecker::result(long code, const std::string &result) {

  if (code != 200) {
    log("HTTP POST failed " + intToString(code), lsError);
    if (m_callback) {
      m_callback->done(NETWORK_ERROR, WebserviceReply());
    }
    return;
  }

  try {
    WebserviceReply resp = parse_reply(result.c_str());
    if (resp.code != 0) {
      log("Webservice complained: <" + intToString(resp.code) + "> " + resp.desc,
          lsWarning);
    }

    if (m_callback) {
      m_callback->done(REST_OK, resp);
    }
  } catch (ParseError &ex) {
    log(std::string("ParseError: <") + ex.what() + "> " + result, lsError);
    if (m_callback) {
      m_callback->done(JSON_ERROR, WebserviceReply());
    }
    return;
  }
}


/**
 * Did the user authorize cloud service
 * @return
 */
bool webservice_communication_authorized() {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  return propSystem.getBoolValue(pp_websvc_enabled);
}

/*
 * Apartment management
 */

__DEFINE_LOG_CHANNEL__(WebserviceApartment, lsInfo)

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

template void WebserviceApartment::doUploadSensorData<ItEvent>(
    ItEvent begin, ItEvent end, WebserviceCallDone_t callback);


void WebserviceApartment::doModelChanged(ChangeType type,
                                         WebserviceCallDone_t callback) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  std::string url;
  std::string params;

  if (!webservice_communication_authorized()) {
    return;
  }

  url = propSystem.getStringValue(pp_websvc_apartment_changed_url_path);
  params = "dssid=" + propSystem.getStringValue(pp_sysinfo_dsid);
  params += "&apartmentChangeType=";
  switch (type) {
  case ApartmentChange:
      params += "Apartment";
      break;
  case TimedEventChange:
      params += "TimedEvent";
      break;
  case UDAChange:
      params += "UserDefinedAction";
      break;
  }

  log("execute: " + url + "?" + params, lsDebug);
  boost::shared_ptr<StatusReplyChecker> mcb(new StatusReplyChecker(callback));
  WebserviceConnection::getInstance()->request(url, params, POST, mcb, true);
}

/*
 * Access Management
 */

__DEFINE_LOG_CHANNEL__(WebserviceAccessManagement, lsInfo)

void WebserviceAccessManagement::doNotifyTokenDeleted(const std::string &token,
                                                      WebserviceCallDone_t callback) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  std::string url;
  std::string params;

  if (!webservice_communication_authorized()) {
    return;
  }

  url += propSystem.getStringValue(pp_websvc_access_mgmt_delete_token_url_path);
  params = "dsid=" + propSystem.getStringValue(pp_sysinfo_dsid);
  params += "&token=" + token;

  // webservice is fire and forget, so use shared ptr for life cycle mgmt
  boost::shared_ptr<StatusReplyChecker> cont(new StatusReplyChecker(callback));
  WebserviceConnection::getInstance()->request(url, params, POST, cont, false);
}

}

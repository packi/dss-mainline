#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "webservice_api.h"

#include <json.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <uuid.h>

#include "dss.h"
#include "event.h"
#include "propertysystem.h"
#include "model/group.h"
#include "web/json.h"

namespace dss {

typedef std::vector<boost::shared_ptr<Event> >::iterator ItEvent;

ParseError::ParseError(const std::string& _message) : runtime_error( _message )
{
}

/****************************************************************************/
/* Helpers of Ms Hub                                                        */
/****************************************************************************/
namespace MsHub {

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
      throw DSSException(std::string("unhandled event ") + event->getName());
    }
  } catch (std::invalid_argument& e) {
    throw DSSException(std::string("Error converting event ") + event->getName());
  }
  return obj;
}

} /* namespace MsHub */


/****************************************************************************/
/* Ms Hub Reply Checker                                                     */
/****************************************************************************/

class MsHubReplyChecker : public URLRequestCallback {
  __DECL_LOG_CHANNEL__
public:
  /**
   * MsHubReplyChecker - triggered by http client, will parse the returned
   * JSON as WebserviceReply structure, and trigger provided callback
   * @callback: trigger
   */
  MsHubReplyChecker(WebserviceCallDone_t callback) : m_callback(callback) {};
  virtual ~MsHubReplyChecker() {};
  virtual void result(long code, const std::string &result);
private:
  WebserviceCallDone_t m_callback;
};

__DEFINE_LOG_CHANNEL__(MsHubReplyChecker, lsInfo);

void MsHubReplyChecker::result(long code, const std::string &result) {

  if (code != 200) {
    log("HTTP POST failed " + intToString(code), lsError);
    if (m_callback) {
      m_callback->done(NETWORK_ERROR, WebserviceReply());
    }
    return;
  }

  try {
    WebserviceReply resp = WebserviceMsHub::parseReply(result.c_str());
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


/****************************************************************************/
/* Webservice MS Hub                                                        */
/****************************************************************************/

__DEFINE_LOG_CHANNEL__(WebserviceMsHub, lsInfo)

bool WebserviceMsHub::isAuthorized() {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  return propSystem.getBoolValue(pp_websvc_enabled);
}

template <class Iterator>
void WebserviceMsHub::doUploadSensorData(Iterator begin, Iterator end,
                                         WebserviceCallDone_t callback) {

  JSONObject obj;
  boost::shared_ptr<JSONArray<JSONObject> > array = boost::make_shared<JSONArray<JSONObject> >();
  int ct = 0;

  std::string parameters;
  // AppToken is piggy backed with websvc_connection::request(.., authenticated=true)
  parameters += "dssid=" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue();
  parameters += "&source=dss"; /* EventSource_dSS */

  obj.addElement("eventsList", array);
  for (; begin != end; begin++) {
    try {
      array->add(MsHub::toJson(*begin));
      ct++;
    } catch (DSSException& e) {
      log(e.what(), lsWarning);
    }
  }

  if (ct == 0) {
    return;
  }

  std::string postdata = obj.toString();
  // TODO eventually emit summary: 10 callsceene, 27 metering, ...
  // dumping whole postdata leads to log rotation and is seldomly needed
  // unless for solving a blame war with cloud team
  log("upload events: " + intToString(ct) + " bytes: " + intToString(postdata.length()), lsInfo);
  log("event data: " + postdata, lsDebug);

  // https://devdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-DSSEventData-SaveEvent_token_apartmentId_dssid_source
  boost::shared_ptr<MsHubReplyChecker> mcb = boost::make_shared<MsHubReplyChecker>(callback);
  HashMapStringString sensorUploadHeaders;
  sensorUploadHeaders["Content-Type"] = "application/json;charset=UTF-8";

  WebserviceConnection::getInstanceMsHub()->request("public/dss/v1_0/DSSEventData/SaveEvent",
                                                    parameters,
                                                    boost::make_shared<HashMapStringString>(sensorUploadHeaders),
                                                    postdata,
                                                    mcb,
                                                    true);
}

template void WebserviceMsHub::doUploadSensorData<ItEvent>(
      ItEvent begin, ItEvent end, WebserviceCallDone_t callback);

void WebserviceMsHub::doDssBackAgain(WebserviceCallDone_t callback)
{
  std::string parameters;

  if (!WebserviceMsHub::isAuthorized()) {
    return;
  }

  // AppToken is piggy backed with websvc_connection::request(.., authenticated=true)
  parameters += "&dssid=" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue();
  boost::shared_ptr<MsHubReplyChecker> mcb = boost::make_shared<MsHubReplyChecker>(callback);
  log("sending DSSBackAgain", lsInfo);
  WebserviceConnection::getInstanceMsHub()->request("internal/dss/v1_0/DSSApartment/DSSBackAgain", parameters, POST, mcb, true);
}

void WebserviceMsHub::doModelChanged(ChangeType type,
                                     WebserviceCallDone_t callback) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  std::string url;
  std::string params;

  if (!WebserviceMsHub::isAuthorized()) {
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
  boost::shared_ptr<MsHubReplyChecker> mcb = boost::make_shared<MsHubReplyChecker>(callback);
  WebserviceConnection::getInstanceMsHub()->request(url, params, POST, mcb, true);
}

void WebserviceMsHub::doNotifyTokenDeleted(const std::string &token,
                                           WebserviceCallDone_t callback) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  std::string url;
  std::string params;

  if (!WebserviceMsHub::isAuthorized()) {
    return;
  }

  url += propSystem.getStringValue(pp_websvc_access_mgmt_delete_token_url_path);
  params = "dsid=" + propSystem.getStringValue(pp_sysinfo_dsid);
  params += "&token=" + token;

  // webservice is fire and forget, so use shared ptr for life cycle mgmt
  boost::shared_ptr<MsHubReplyChecker> cont = boost::make_shared<MsHubReplyChecker>(callback);
  WebserviceConnection::getInstanceMsHub()->request(url, params, POST, cont, false);
}

void WebserviceMsHub::doGetWeatherInformation(boost::shared_ptr<URLRequestCallback> callback) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  std::string url;
  std::string params;

  if (!WebserviceMsHub::isAuthorized()) {
    return;
  }

  url += "public/dss/v1_0/DSSApartment/GetWeatherInformation";
  params = "dssid=" + propSystem.getStringValue(pp_sysinfo_dsid);

  WebserviceConnection::getInstanceMsHub()->request(url, params, GET, callback, false);
}

WebserviceReply WebserviceMsHub::parseReply(const char* buf) {
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



/****************************************************************************/
/* Helpers of dS Hub                                                        */
/****************************************************************************/
namespace DsHub {

const static std::string evtGroup_Metering = "Metering";
const static std::string evtGroup_Activity = "Activity";
const static std::string evtGroup_ApartmentAndDevice = "ApartmentAndDevice";

const static std::string evtCategory_DeviceSensorValue = "DeviceSensorValue";
const static std::string evtCategory_ZoneSensorValue = "ZoneSensorValue";
const static std::string evtCategory_ZoneGroupCallScene = "ZoneSceneCall";
const static std::string evtCategory_ZoneGroupUndoScene = "ZoneUndoScene";
const static std::string evtCategory_ApartmentState = "ApartmentStateChange";
const static std::string evtCategory_ZoneGroupState = "ZoneStateChange";
const static std::string evtCategory_DeviceBinaryInput = "DeviceBinaryInput";
const static std::string evtCategory_DeviceInputState = "DeviceInputState";
const static std::string evtCategory_DeviceStatusReport = "DeviceStatusReport";
const static std::string evtCategory_DeviceSensorError = "DeviceSensorError";
const static std::string evtCategory_ZoneSensorError = "ZoneSensorError";
const static std::string evtCategory_HeatingControllerSetup = "HeatingControllerSetup";
const static std::string evtCategory_HeatingControllerValue = "HeatingControllerValue";
const static std::string evtCategory_HeatingControllerState = "HeatingControllerState";
const static std::string evtCategory_HeatingEnabled = "HeatingEnabled";
const static std::string evtCategory_AddonToCloud = "AddOnToCloud";
const static std::string evtCategory_ExecutionDenied = "ExecutionDenied";
const static std::string evtCategory_LogFileData = "LogFileData";

const static int dsEnum_SensorError_invalidValue = 1;
const static int dsEnum_SensorError_noValue = 2;

int getRandomUUID(std::string &str)
{
    uuid_t *uuid;
    uuid_rc_t rc;

    rc = uuid_create(&uuid);
    if (rc != UUID_RC_OK) {
        return 1;
    }

    rc = uuid_make(uuid, UUID_MAKE_V4);
    if (rc != UUID_RC_OK) {
        uuid_destroy(uuid);
        return 1;
    }

    char *cstr = NULL;
    size_t cstr_len;

    rc = uuid_export(uuid, UUID_FMT_STR, &cstr, &cstr_len);
    if (rc != UUID_RC_OK) {
        uuid_destroy(uuid);
        return 1;
    }

    str = std::string(cstr);

    free(cstr);
    uuid_destroy(uuid);
    return 0;
}

void createHeader(boost::shared_ptr<JSONObject> &header, const std::string& group,
                  const std::string& category, const Event *event)
{
  static unsigned long int sequenceId = 0;

  std::string eventId;
  assert(getRandomUUID(eventId) == 0);

  header->addProperty("EventCategory", category);
  header->addProperty("EventGroup", group);
  header->addProperty("SequenceID", sequenceId++);
  header->addProperty("SequenceIDSince", DateTime(DSS::getInstance()->getStartTime()).toISO8601());
  header->addProperty("Timestamp", event->getTimestamp().toISO8601());
  header->addProperty("EventID", eventId);
  header->addProperty("Source", "dss");
}

JSONObject toJson(const boost::shared_ptr<Event> &event) {
  boost::shared_ptr<const DeviceReference> pDeviceRef;
  boost::shared_ptr<JSONObject> header = boost::make_shared<JSONObject>();
  boost::shared_ptr<JSONObject> body = boost::make_shared<JSONObject>();
  JSONObject eventJson;

  std::string propValue;

  try {
    if ((event->getName() == EventName::DeviceSensorValue) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(header, evtGroup_Metering, evtCategory_DeviceSensorValue,
                   event.get());
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      body->addProperty("SensorType", sensorType);
      body->addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
      body->addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
    } else if ((event->getName() == EventName::ZoneSensorValue) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(header, evtGroup_Metering, evtCategory_ZoneSensorValue,
                   event.get());
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      body->addProperty("SensorType", sensorType);
      body->addProperty("SensorValue", event->getPropertyByName("sensorValueFloat"));
      body->addProperty("ZoneID",  pGroup->getZoneID());
    } else if ((event->getName() == EventName::CallScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(header, evtGroup_Activity, evtCategory_ZoneGroupCallScene,
                   event.get());
      body->addProperty("ZoneID",  pGroup->getZoneID());
      body->addProperty("GroupID", pGroup->getID());
      body->addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
      body->addProperty("Force", event->hasPropertySet("forced"));
      body->addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if ((event->getName() == EventName::UndoScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(header, evtGroup_Activity, evtCategory_ZoneGroupUndoScene,
                   event.get());
      body->addProperty("ZoneID",  pGroup->getZoneID());
      body->addProperty("GroupID", pGroup->getID());
      body->addProperty("SceneID", strToInt(event->getPropertyByName("sceneID")));
      body->addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if (event->getName() == EventName::StateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      switch (pState->getType()) {
      case StateType_Apartment:
      case StateType_Service:
      case StateType_Script:
        createHeader(header, evtGroup_Activity, evtCategory_ApartmentState,
                     event.get());
        body->addProperty("Origin", event->getPropertyByName("originDeviceID"));
        break;
      case StateType_Group:
        createHeader(header, evtGroup_Activity, evtCategory_ZoneGroupState,
                     event.get());
        body->addProperty("ZoneId", event->getPropertyByName("zoneId"));
        body->addProperty("GroupId", event->getPropertyByName("groupId"));
        break;
      case StateType_Device:
        /* TODO Does it correspond to DeviceStateChangeEvent in BOM? */
        createHeader(header, evtGroup_Activity, evtCategory_DeviceInputState,
                     event.get());
        /* TODO Should DeviceStateChangeEvent contain device identifier field named 'DeviceId', or 'Origin'? */
        body->addProperty("Origin", event->getPropertyByName("originDeviceID"));
        break;
      default:
        break;
      }
      body->addProperty("StateName", pState->getName());
      body->addProperty("StateValue", pState->toString());
    } else if (event->getName() == EventName::OldStateChange) {
      createHeader(header, evtGroup_Activity, evtCategory_ApartmentState, event.get());
      body->addProperty("StateName", event->getPropertyByName("statename"));
      body->addProperty("StateValue", event->getPropertyByName("state"));
      body->addProperty("Origin", event->getPropertyByName("originDeviceID"));
    } else if (event->getName() == EventName::AddonStateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      std::string addonName = event->getPropertyByName("scriptID");
      createHeader(header, evtGroup_Activity, evtCategory_ApartmentState, event.get());
      body->addProperty("StateName", addonName + "." + pState->getName());
      body->addProperty("StateValue", pState->toString());
      body->addProperty("Origin", event->getPropertyByName("scriptID"));
    } else if ((event->getName() == EventName::DeviceStatus) && (event->getRaiseLocation() == erlDevice)) {
      /* TODO Does it correspond to DeviceStatusErrorEvent in BOM? */
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(header, evtGroup_Activity, evtCategory_DeviceStatusReport,
                   event.get());
      body->addProperty("Index", event->getPropertyByName("statusIndex"));
      body->addProperty("Value", event->getPropertyByName("statusValue"));
      body->addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
    } else if ((event->getName() == EventName::DeviceInvalidSensor) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(header, evtGroup_ApartmentAndDevice,
                   evtCategory_DeviceSensorError, event.get());
      body->addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
      body->addProperty("SensorType", strToInt(event->getPropertyByName("sensorType")));
      body->addProperty("StatusCode", dsEnum_SensorError_noValue);
    } else if ((event->getName() == EventName::ZoneSensorError) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(header, evtGroup_ApartmentAndDevice,
                   evtCategory_ZoneSensorError, event.get());
      int sensorType = strToInt(event->getPropertyByName("sensorType"));
      body->addProperty("SensorType", sensorType);
      body->addProperty("ZoneID",  pGroup->getZoneID());
      body->addProperty("GroupID", pGroup->getID());
      body->addProperty("StatusCode", dsEnum_SensorError_noValue);
    } else if (event->getName() == EventName::HeatingControllerSetup) {
      createHeader(header, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerSetup, event.get());
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        body->addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingControllerValueDsHub) {
      createHeader(header, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerValue, event.get());
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        body->addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingControllerState) {
      createHeader(header, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerState, event.get());
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        body->addProperty(iParam->first, iParam->second);
      }
    } else if (event->getName() == EventName::HeatingEnabled) {
      createHeader(header, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingEnabled, event.get());
      body->addProperty("HeatingEnabled", event->getPropertyByName("HeatingEnabled"));
    } else if (event->getName() == EventName::AddonToCloud) {
      createHeader(header, evtGroup_Activity, evtCategory_AddonToCloud, event.get());
      body->addProperty("EventName", event->getPropertyByName("EventName"));
      const dss::HashMapStringString& props =  event->getProperties().getContainer();
      boost::shared_ptr<JSONObject> parameterObj(new JSONObject);
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        parameterObj->addProperty(iParam->first, iParam->second);
      }
      body->addElement("Parameter", parameterObj);
    } else if (event->getName() == EventName::DeviceBinaryInputEvent) {
      /* TODO is event group and category correct? */
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(header, evtGroup_ApartmentAndDevice, evtCategory_DeviceBinaryInput, event.get());
      body->addProperty("InputIndex", event->getPropertyByName("inputIndex"));
      body->addProperty("InputValue", event->getPropertyByName("inputValue"));
      body->addProperty("DeviceID", dsuid2str(pDeviceRef->getDSID()));
    } else if (event->getName() == EventName::ExecutionDenied) {
      /* TODO is event group and category correct? */
      createHeader(header, evtGroup_Activity, evtCategory_ExecutionDenied, event.get());
      body->addProperty("ActivityType", event->getPropertyByName("action-type"));
      body->addProperty("ActivityName", event->getPropertyByName("action-name"));
      body->addProperty("SourceName", event->getPropertyByName("source-name"));
      body->addProperty("Reason", event->getPropertyByName("reason"));
    } else if (event->getName() == EventName::LogFileData) {
      createHeader(header, evtGroup_Activity, evtCategory_LogFileData, event.get());
    } else {
      throw DSSException(std::string("unhandled event ") + event->getName());
    }

    eventJson.addElement("EventHeader", header);
    eventJson.addElement("EventBody", body);
  } catch (std::invalid_argument& e) {
    throw DSSException(std::string("Error converting event ") + event->getName());
  }
  return eventJson;
}

} /* namespace dSHub */


/****************************************************************************/
/* DsHubReplyChecker                                                        */
/****************************************************************************/

class DsHubReplyChecker : public URLRequestCallback {
  __DECL_LOG_CHANNEL__
public:
  /**
   * DsHubReplyChecker - triggered by http client, check return code,
   * and trigger provided callback
   * @callback: trigger
   */
  DsHubReplyChecker(WebserviceCallDone_t callback) : m_callback(callback) {};
  virtual ~DsHubReplyChecker() {};
  virtual void result(long code, const std::string &result);
private:
  WebserviceCallDone_t m_callback;
};

__DEFINE_LOG_CHANNEL__(DsHubReplyChecker, lsInfo);

void DsHubReplyChecker::result(long code, const std::string &result) {

  if (code != 200) {
    log("HTTP POST failed " + intToString(code), lsError);
    if (m_callback) {
      m_callback->done(NETWORK_ERROR, WebserviceReply());
    }
    return;
  }

  if (m_callback) {
    m_callback->done(REST_OK, WebserviceReply());
  }
}


/****************************************************************************/
/* Webservice dS Hub                                                        */
/****************************************************************************/

__DEFINE_LOG_CHANNEL__(WebserviceDsHub, lsInfo)

bool WebserviceDsHub::isAuthorized() {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  return propSystem.getBoolValue(pp_websvc_enabled);
}

template <class Iterator>
void WebserviceDsHub::doUploadSensorData(Iterator begin, Iterator end,
                                         WebserviceCallDone_t callback) {

  JSONObject obj;
  boost::shared_ptr<JSONArray<JSONObject> > array = boost::make_shared<JSONArray<JSONObject> >();
  int ct = 0;

  // /v1/DSS/{dssID}/Events
  std::string url;
  url = "v1/DSS/" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue() + "/Events";

  obj.addElement("Events", array);
  for (; begin != end; begin++) {
    try {
      array->add(DsHub::toJson(*begin));
      ct++;
    } catch (DSSException& e) {
      log(e.what(), lsWarning);
    }
  }

  if (ct == 0) {
    return;
  }

  std::string postdata = obj.toString();
  // TODO eventually emit summary: 10 callsceene, 27 metering, ...
  // dumping whole postdata leads to log rotation and is seldomly needed
  // unless for solving a blame war with cloud team
  log("upload events: " + intToString(ct) + " bytes: " + intToString(postdata.length()), lsInfo);
  log("event data: " + postdata, lsDebug);

  // https://devdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-DSSEventData-SaveEvent_token_apartmentId_dssid_source
  boost::shared_ptr<DsHubReplyChecker> mcb = boost::make_shared<DsHubReplyChecker>(callback);
  HashMapStringString sensorUploadHeaders;
  sensorUploadHeaders["Content-Type"] = "application/json;charset=UTF-8";
  sensorUploadHeaders["Accept"] = "application/json";

  WebserviceConnection::getInstanceDsHub()->request(url,
                                                    "",
                                                    boost::make_shared<HashMapStringString>(sensorUploadHeaders),
                                                    postdata,
                                                    mcb,
                                                    true);
}

template void WebserviceDsHub::doUploadSensorData<ItEvent>(
      ItEvent begin, ItEvent end, WebserviceCallDone_t callback);

} /* namespace dss */

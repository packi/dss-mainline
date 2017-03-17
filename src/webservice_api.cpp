#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <uuid.h>

#include <json.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include "dss.h"
#include "event.h"
#include "event/event_fields.h"
#include "foreach.h"
#include "propertysystem.h"
#include "model/devicereference.h"
#include "model/group.h"
#include "model/state.h"
#include "webservice_api.h"
#include "web/webrequests.h"
#include "propertysystem_common_paths.h"


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
const static std::string evtCategory_HeatingSystemCapability = "HeatingSystemCapability";
const static std::string evtCategory_AddonToCloud = "AddOnToCloud";

//const static int dsEnum_SensorError_invalidValue = 1;
const static int dsEnum_SensorError_noValue = 2;

void appendCommon(JSONWriter& json, const std::string& group,
                  const std::string& category, const Event *event)
{
  json.add("EventGroup", group);
  json.add("EventCategory", category);
  json.add("Timestamp", event->getTimestamp().toISO8601_ms());
}

/* mshub */
std::vector<std::string> uploadEvents()
{
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
  events.push_back(EventName::HeatingSystemCapability);
  events.push_back(EventName::HeatingControllerSetup);
  events.push_back(EventName::HeatingControllerValue);
  events.push_back(EventName::HeatingControllerState);
  events.push_back(EventName::OldStateChange);
  events.push_back(EventName::AddonToCloud);
  return events;
}

void toJson(const boost::shared_ptr<Event> &event, JSONWriter& json) {
  boost::shared_ptr<const DeviceReference> pDeviceRef;
  std::string propValue;

  try {
    if ((event->getName() == EventName::DeviceSensorValue) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(json, evtGroup_Metering, evtCategory_DeviceSensorValue,
                   event.get());
      json.add("SensorType", event->getPropertyByName<SensorType>("sensorType"));
      json.add("SensorValue", event->getPropertyByName("sensorValueFloat"));
      json.add("DeviceID", pDeviceRef->getDSID());
      propValue = event->getPropertyByName(ef_sensorIndex);
      if (!propValue.empty()) {
        json.add("SensorIndex", propValue);
      }
    } else if ((event->getName() == EventName::ZoneSensorValue) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(json, evtGroup_Metering, evtCategory_ZoneSensorValue,
                   event.get());
      json.add("SensorType", event->getPropertyByName<SensorType>("sensorType"));
      json.add("SensorValue", event->getPropertyByName("sensorValueFloat"));
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID", pGroup->getID());
    } else if ((event->getName() == EventName::CallScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(json, evtGroup_Activity, evtCategory_ZoneGroupCallScene,
                   event.get());
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID", pGroup->getID());
      json.add("SceneID", strToInt(event->getPropertyByName(ef_sceneID)));
      json.add("Force", event->hasPropertySet(ef_forced));
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
    } else if ((event->getName() == EventName::UndoScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(json, evtGroup_Activity, evtCategory_ZoneGroupUndoScene,
                   event.get());
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID", pGroup->getID());
      json.add("SceneID", strToInt(event->getPropertyByName(ef_sceneID)));
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
    } else if (event->getName() == EventName::StateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      switch (pState->getType()) {
      case StateType_Apartment:
      case StateType_Service:
      case StateType_Script:
        appendCommon(json, evtGroup_Activity, evtCategory_ApartmentState,
                     event.get());
        break;
      case StateType_Group:
        appendCommon(json, evtGroup_Activity, evtCategory_ZoneGroupState,
                     event.get());
        break;
      case StateType_Device:
        appendCommon(json, evtGroup_Activity, evtCategory_DeviceInputState,
                     event.get());
        break;
      case StateType_Circuit:
        appendCommon(json, evtGroup_Activity, evtCategory_DeviceInputState,
                     event.get());
        break;
      default:
        break;
      }
      json.add("StateName", pState->getName());
      json.add("StateValue", pState->toString());
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
    } else if (event->getName() == EventName::OldStateChange) {
      appendCommon(json, evtGroup_Activity, evtCategory_ApartmentState, event.get());
      json.add("StateName", event->getPropertyByName("statename"));
      json.add("StateValue", event->getPropertyByName("state"));
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
    } else if (event->getName() == EventName::AddonStateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      std::string addonName = event->getPropertyByName("scriptID");
      appendCommon(json, evtGroup_Activity, evtCategory_ApartmentState, event.get());
      json.add("StateName", addonName + "." + pState->getName());
      json.add("StateValue", pState->toString());
      json.add("Origin", event->getPropertyByName("scriptID"));
    } else if ((event->getName() == EventName::DeviceStatus) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(json, evtGroup_Activity, evtCategory_DeviceStatusReport,
                   event.get());
      json.add("StatusIndex", event->getPropertyByName("statusIndex"));
      json.add("StatusValue", event->getPropertyByName("statusValue"));
      json.add("DeviceID", pDeviceRef->getDSID());
    } else if ((event->getName() == EventName::DeviceInvalidSensor) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_DeviceSensorError, event.get());
      json.add("DeviceID", pDeviceRef->getDSID());
      json.add("SensorType", event->getPropertyByName<SensorType>("sensorType"));
      json.add("StatusCode", dsEnum_SensorError_noValue);
    } else if ((event->getName() == EventName::ZoneSensorError) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_ZoneSensorError, event.get());
      json.add("SensorType", event->getPropertyByName<SensorType>("sensorType"));
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID", pGroup->getID());
      json.add("StatusCode", dsEnum_SensorError_noValue);
    } else if (event->getName() == EventName::HeatingControllerSetup) {
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerSetup, event.get());
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
    } else if (event->getName() == EventName::HeatingControllerValue) {
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerValue, event.get());
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
    } else if (event->getName() == EventName::HeatingControllerState) {
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingControllerState, event.get());
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
    } else if (event->getName() == EventName::HeatingEnabled) {
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingEnabled, event.get());
      json.add("HeatingEnabled", event->getPropertyByName("HeatingEnabled"));
      json.add("CoolingEnabled", event->getPropertyByName("CoolingEnabled"));
    } else if (event->getName() == EventName::HeatingSystemCapability) {
      appendCommon(json, evtGroup_ApartmentAndDevice,
                   evtCategory_HeatingSystemCapability, event.get());
      json.add("HeatingSupported", event->getPropertyByName("HeatingSupported"));
      json.add("CoolingSupported", event->getPropertyByName("CoolingSupported"));
    } else if (event->getName() == EventName::AddonToCloud) {
      appendCommon(json, evtGroup_Activity, evtCategory_AddonToCloud, event.get());
      json.add("EventName", event->getPropertyByName("EventName"));
      json.startObject("parameter");
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
      json.endObject();

    } else {
      throw DSSException(std::string("unhandled event ") + event->toString());
    }
  } catch (std::invalid_argument& e) {
    throw DSSException(std::string("Error converting event ") + event->toString());
  }
  return;
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
  return propSystem.getBoolValue(pp_websvc_mshub_active);
}

template <class Iterator>
bool WebserviceMsHub::doUploadSensorData(Iterator begin, Iterator end,
                                         WebserviceCallDone_t callback) {

  JSONWriter json(JSONWriter::jsonNoneResult);
  int ct = 0;

  std::string parameters;
  // AppToken is piggy backed with websvc_connection::request(.., authenticated=true)
  parameters += "dssid=" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue();
  parameters += "&source=dss"; /* EventSource_dSS */

  json.startArray("eventsList");
  for (; begin != end; begin++) {
    try {
      json.startObject();
      MsHub::toJson(*begin, json);
      json.endObject();
      ct++;
    } catch (DSSException& e) {
      json.endObject();
      log(e.what(), lsWarning);
    }
  }
  json.endArray();

  if (ct == 0) {
    return false;
  }

  std::string postdata = json.successJSON();
  // TODO eventually emit summary: 10 callsceene, 27 metering, ...
  // dumping whole postdata leads to log rotation and is seldomly needed
  // unless for solving a blame war with cloud team
  log("upload events: " + intToString(ct) + " bytes: " + intToString(postdata.length()), lsInfo);
  log("event data: " + postdata, lsDebug);

  // https://devdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-DSSEventData-SaveEvent_token_apartmentId_dssid_source
  boost::shared_ptr<MsHubReplyChecker> mcb = boost::make_shared<MsHubReplyChecker>(callback);
  std::unordered_map<std::string, std::string> sensorUploadHeaders;
  sensorUploadHeaders["Content-Type"] = "application/json;charset=UTF-8";

  WebserviceConnection::getInstanceMsHub()->request("public/dss/v1_0/DSSEventData/SaveEvent",
                                                    parameters,
                                                    sensorUploadHeaders,
                                                    postdata,
                                                    mcb,
                                                    true);
  return true;
}

template bool WebserviceMsHub::doUploadSensorData<ItEvent>(
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

void WebserviceMsHub::doVdcStoreVdcToken(const std::string &vdcToken, WebserviceCallDone_t callback) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  std::string url;
  std::string params;

  if (!WebserviceMsHub::isAuthorized()) {
    callback->done(NETWORK_ERROR, WebserviceReply({0, ""}));
    return;
  }

  url += "public/vDC/v1_0/Device/StoreVDCToken";
  params += "dssId=" + propSystem.getStringValue(pp_sysinfo_dsid);
  params += "&ospToken=" + propSystem.getStringValue(pp_websvc_rc_osptoken);
  params += "&vdcToken=" + vdcToken;

  // webservice is fire and forget, so use shared ptr for life cycle mgmt
  boost::shared_ptr<MsHubReplyChecker> cont = boost::make_shared<MsHubReplyChecker>(callback);
  WebserviceConnection::getInstanceMsHub()->request(url, params, POST, cont, false);
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

// sensor measurement
const static std::string evtCategory_DeviceSensorValue = "DeviceSensorValue";
const static std::string evtCategory_DeviceSensorError = "DeviceSensorError";
const static std::string evtCategory_ZoneSensorValue = "ZoneSensorValue";
const static std::string evtCategory_ZoneSensorError = "ZoneSensorError";
const static std::string evtCategory_DeviceAutomation = "DeviceAutomation"; // 12
// input
const static std::string evtCategory_DeviceBinaryInput = "DeviceBinaryInput"; // 18
const static std::string evtCategory_DeviceStatusError = "DeviceStatusError"; // 16
const static std::string evtCategory_DeviceAppButtonClick = "DeviceAppButtonClick"; // 13
const static std::string evtCategory_DeviceAppActionClick = "DeviceAppActionClick"; // ??
// activity
const static std::string evtCategory_ZoneGroupCallScene = "ZoneSceneCall";
const static std::string evtCategory_ZoneGroupUndoScene = "ZoneUndoScene";
const static std::string evtCategory_DeviceCallScene = "DeviceSceneCall";
const static std::string evtCategory_ZoneBlink = "ZoneBlink"; // 7
const static std::string evtCategory_DeviceLocal = "DeviceLocal"; // 11
const static std::string evtCategory_DeviceBlink = "DeviceBlink"; // 14
const static std::string evtCategory_UserDefinedActionEvent = "UserDefinedAction"; // 22
// states
const static std::string evtCategory_ApartmentStateChange = "ApartmentStateChange"; // 21
const static std::string evtCategory_ZoneStateChange = "ZoneStateChange"; // 10
const static std::string evtCategory_DeviceStateChange = "DeviceStateChange"; // 15
// heating
const static std::string evtCategory_HeatingControllerSetup = "HeatingControllerSetup";
const static std::string evtCategory_HeatingControllerValue = "HeatingControllerValue";
const static std::string evtCategory_HeatingControllerState = "HeatingControllerState";
const static std::string evtCategory_HeatingEnabled = "HeatingEnabled";
const static std::string evtCategory_HeatingSystemCapability = "HeatingSystemCapability";
// single-device vdc
const static std::string evtCategory_DeviceCustomActionChangedEvent = "DeviceCustomActionChangedEvent";
const static std::string evtCategory_DeviceActionEvent = "DeviceActionEvent";
const static std::string evtCategory_DeviceEventEvent = "DeviceEventEvent";
const static std::string evtCategory_DeviceStateEvent = "DeviceStateEvent";
// misc
const static std::string evtCategory_AddonToCloud = "AddOnToCloud";
const static std::string evtCategory_ExecutionDenied = "ExecutionDenied";
const static std::string evtCategory_LogFileData = "LogFileData";

//const static int dsEnum_SensorError_invalidValue = 1;
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

void createHeader(JSONWriter& json, const std::string& group,
                  const std::string& category, const Event *event)
{
  static unsigned long int sequenceId = 0;

  std::string eventId;
  assert(getRandomUUID(eventId) == 0);

  json.startObject("EventHeader");
  json.add("EventCategory", category);
  json.add("EventGroup", group);
  json.add("SequenceID", (int)sequenceId++);
  json.add("SequenceIDSince", DateTime(DSS::getInstance()->getStartTime()).toISO8601());
  json.add("Timestamp", event->getTimestamp().toISO8601());
  json.add("EventID", eventId);
  json.add("Source", "dss");
  json.endObject();
}

/* dshub */
std::vector<std::string> uploadEvents()
{
  std::vector<std::string> events;
  // sensoric
  events.push_back(EventName::DeviceSensorValue);
  events.push_back(EventName::DeviceInvalidSensor);
  events.push_back(EventName::ZoneSensorValue);
  events.push_back(EventName::ZoneSensorError);
  // inputs
  events.push_back(EventName::DeviceBinaryInputEvent);
  events.push_back(EventName::DeviceButtonClick);
  //events.push_back(EventName::ButtonDeviceAction); TODO: define in EA
  //events.push_back(EventName::DeviceSensorEvent); TODO: refine in EA
  events.push_back(EventName::DeviceStatus);
  // activity
  events.push_back(EventName::CallScene);
  events.push_back(EventName::UndoScene);
  events.push_back(EventName::IdentifyBlink);
  events.push_back(EventName::HighLevelEvent);
  // vdc devices
  events.push_back(EventName::DeviceCustomActionChangedEvent);
  events.push_back(EventName::DeviceActionEvent);
  events.push_back(EventName::DeviceEventEvent);
  events.push_back(EventName::DeviceStateEvent);
  // states and addons
  events.push_back(EventName::StateChange);
  //events.push_back(EventName::AddonStateChange); // TODO: define in EA
  //events.push_back(EventName::OldStateChange); // TODO: undef, remove this finally
  // configuration
  events.push_back(EventName::AddonToCloud);
  //events.push_back(EventName::DeviceEvent); // TODO: defined in EA, but not supported yet
  //events.push_back(EventName::DeviceHeatingTypeChanged); TODO: define in EA
  //events.push_back(EventName::SceneNameChanged); // TODO: define in EA
  // heating
  events.push_back(EventName::HeatingEnabled);
  events.push_back(EventName::HeatingSystemCapability);
  //events.push_back(EventName::HeatingModeSwitch); // TODO: define in EA
  events.push_back(EventName::HeatingControllerSetup);
  events.push_back(EventName::HeatingControllerValueDsHub);
  events.push_back(EventName::HeatingControllerState);
  //events.push_back(EventName::HeatingValveProtection); TODO: define in EA
  // operational
  //events.push_back(EventName::BuildingService); TODO: define in EA
  //events.push_back(EventName::OperationLock); TODO: define in EA
  //events.push_back(EventName::ClusterConfigLock); TODO: define in EA
  // misc
  //events.push_back(EventName::DevicesFirstSeen); TODO: define in EA
  events.push_back(EventName::ExecutionDenied);
  events.push_back(EventName::LogFileData);
  return events;
}

void stateToJson(const boost::shared_ptr<Event> &event, JSONWriter& json) {
  json.add("StateName", event->getPropertyByName("statename"));
  json.add("StateValue", event->getPropertyByName("state"));
  json.add("Value", event->getPropertyByName("value"));
  json.add("OldValue", event->getPropertyByName("oldvalue"));
}

void toJson(const boost::shared_ptr<Event> &event, JSONWriter& json) {
  boost::shared_ptr<const DeviceReference> pDeviceRef;

  std::string propValue;

  try {
    // sensoric
    if ((event->getName() == EventName::DeviceSensorValue) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Metering, evtCategory_DeviceSensorValue, event.get());
      json.startObject("EventBody");
      json.add("SensorType", (int) (event->getPropertyByName<SensorType>("sensorType")));
      json.add("SensorValue", event->getPropertyByName("sensorValueFloat"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::DeviceInvalidSensor) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Metering, evtCategory_DeviceSensorError, event.get());
      json.startObject("EventBody");
      json.add("SensorType", (int) (event->getPropertyByName<SensorType>("sensorType")));
      json.add("StatusCode", dsEnum_SensorError_noValue);
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::ZoneSensorValue) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(json, evtGroup_Metering, evtCategory_ZoneSensorValue, event.get());
      json.startObject("EventBody");
      json.add("SensorType", (int) (event->getPropertyByName<SensorType>("sensorType")));
      json.add("SensorValue", event->getPropertyByName("sensorValueFloat"));
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID",  pGroup->getID());
      json.endObject();
    } else if ((event->getName() == EventName::ZoneSensorError) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(json, evtGroup_Metering, evtCategory_ZoneSensorError, event.get());
      json.startObject("EventBody");
      json.add("SensorType", (int) (event->getPropertyByName<SensorType>("sensorType")));
      json.add("StatusCode", dsEnum_SensorError_noValue);
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID", pGroup->getID());
      json.endObject();

    // inputs
    } else if (event->getName() == EventName::DeviceBinaryInputEvent) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceBinaryInput, event.get());
      json.startObject("EventBody");
      json.add("InputIndex", event->getPropertyByName("inputIndex"));
      json.add("InputValue", event->getPropertyByName("inputValue"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::DeviceStatus) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceStatusError, event.get());
      json.startObject("EventBody");
      json.add("Index", event->getPropertyByName("statusIndex"));
      json.add("Value", event->getPropertyByName("statusValue"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::DeviceSensorEvent) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceAutomation, event.get());
      json.startObject("EventBody");
      json.add("Value", event->getPropertyByName("sensorEvent"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::DeviceButtonClick) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceAppButtonClick, event.get());
      json.startObject("EventBody");
      json.add("ButtonIndex", event->getPropertyByName("buttonIndex"));
      json.add("ClickType", event->getPropertyByName("clickType"));
      json.add("HoldCount", event->getPropertyByName("holdCount"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::ButtonDeviceAction) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceAppActionClick, event.get());
      json.startObject("EventBody");
      json.add("Value", event->getPropertyByName("actionID"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();

    // activity
    } else if (event->getName() == EventName::CallScene) {
      if (event->getRaiseLocation() == erlGroup) {
        createHeader(json, evtGroup_Activity, evtCategory_ZoneGroupCallScene, event.get());
        boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
        json.startObject("EventBody");
        json.add("ZoneID",  pGroup->getZoneID());
        json.add("GroupID", pGroup->getID());
      } else if (event->getRaiseLocation() == erlDevice) {
        createHeader(json, evtGroup_Activity, evtCategory_DeviceCallScene, event.get());
        pDeviceRef = event->getRaisedAtDevice();
        json.startObject("EventBody");
        json.add("DeviceID", pDeviceRef->getDSID());
      }
      json.add("SceneID", strToInt(event->getPropertyByName(ef_sceneID)));
      json.add("Force", event->hasPropertySet(ef_forced));
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
      // TODO: json.add("UserToken", "");
      json.endObject();
    } else if ((event->getName() == EventName::UndoScene) && (event->getRaiseLocation() == erlGroup)) {
      boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
      createHeader(json, evtGroup_Activity, evtCategory_ZoneGroupUndoScene, event.get());
      json.startObject("EventBody");
      json.add("ZoneID",  pGroup->getZoneID());
      json.add("GroupID", pGroup->getID());
      json.add("SceneID", strToInt(event->getPropertyByName(ef_sceneID)));
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
      // TODO: json.add("UserToken", "");
      json.endObject();
    } else if (event->getName() == EventName::IdentifyBlink) {
      if (event->getRaiseLocation() == erlGroup) {
        createHeader(json, evtGroup_Activity, evtCategory_ZoneBlink, event.get());
        boost::shared_ptr<const Group> pGroup = event->getRaisedAtGroup();
        json.startObject("EventBody");
        json.add("ZoneID",  pGroup->getZoneID());
        json.add("GroupID", pGroup->getID());
      } else if (event->getRaiseLocation() == erlDevice) {
        createHeader(json, evtGroup_Activity, evtCategory_DeviceBlink, event.get());
        pDeviceRef = event->getRaisedAtDevice();
        json.startObject("EventBody");
        json.add("DeviceID", pDeviceRef->getDSID());
      }
      json.add("Origin", event->getPropertyByName(ef_callOrigin));
      // TODO: json.add("UserToken", "");
      json.endObject();

    } else if (event->getName() == EventName::HighLevelEvent) {
      std::string evtId = event->getPropertyByName("id");
      std::string evtName;
      PropertySystem &propSystem(DSS::getInstance()->getPropertySystem());
      PropertyNodePtr evtProperty = propSystem.getProperty("/scripts/system-addon-user-defined-actions/" + evtId + "/name");
      if (evtProperty) {
        evtName = evtProperty->getAsString();
      }
      createHeader(json, evtGroup_Activity, evtCategory_UserDefinedActionEvent, event.get());
      json.startObject("EventBody");
      json.add("SourceName", event->getPropertyByName(ef_callOrigin));
      json.add("UserDefinedActionID", evtId);
      json.endObject();

    // states and addons
    } else if (event->getName() == EventName::StateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      switch (pState->getType()) {
      case StateType_Apartment:
      case StateType_Service:
      case StateType_Script:
        {
          createHeader(json, evtGroup_Activity, evtCategory_ApartmentStateChange, event.get());
          json.startObject("EventBody");
          json.add("Origin", event->getPropertyByName(ef_callOrigin));
          stateToJson(event, json);
          json.endObject();
        }
        break;
      case StateType_Group:
      case StateType_SensorZone:
        {
          createHeader(json, evtGroup_Activity, evtCategory_ZoneStateChange, event.get());
          boost::shared_ptr<Group> group = pState->getProviderGroup();
          json.startObject("EventBody");
          json.add("ZoneId", intToString(group->getZoneID()));
          json.add("GroupId", intToString(group->getID()));
          stateToJson(event, json);
          json.endObject();
        }
        break;
      case StateType_Device:
      case StateType_SensorDevice:
        {
          createHeader(json, evtGroup_Activity, evtCategory_DeviceStateChange, event.get());
          boost::shared_ptr<Device> device = pState->getProviderDevice();
          json.startObject("EventBody");
          json.add("DeviceID", device->getDSID());
          stateToJson(event, json);
          json.endObject();
        }
        break;
      case StateType_Circuit:
        break;
      }
    } else if (event->getName() == EventName::AddonStateChange) {
      boost::shared_ptr<const State> pState = event->getRaisedAtState();
      std::string addonName = event->getPropertyByName("scriptID");
      createHeader(json, evtGroup_Activity, evtCategory_ApartmentStateChange, event.get());
      json.startObject("EventBody");
      json.add("StateName", addonName + "." + pState->getName());
      json.add("StateValue", pState->toString());
      json.add("Origin", event->getPropertyByName("scriptID"));
      json.endObject();

    // vdc devices
    } else if ((event->getName() == EventName::DeviceActionEvent) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceActionEvent, event.get());
      json.startObject("EventBody");
      json.add("ActionId", event->getPropertyByName("actionId"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.startArray("Parameter");
      foreach (auto&& param, event->getProperties().getContainer()) {
        if (beginsWith(param.first, "params.")) {
          json.startObject();
          json.add("Name", param.first.substr(7));
          json.add("Value", param.second);
          json.endObject();
        }
      }
      json.endArray();
      json.endObject();
    } else if ((event->getName() == EventName::DeviceEventEvent) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceEventEvent, event.get());
      json.startObject("EventBody");
      json.add("EventId", event->getPropertyByName("eventId"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::DeviceStateEvent) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceStateEvent, event.get());
      json.startObject("EventBody");
      json.add("StateId", event->getPropertyByName("stateId"));
      json.add("Value", event->getPropertyByName("value"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.endObject();
    } else if ((event->getName() == EventName::DeviceCustomActionChangedEvent) && (event->getRaiseLocation() == erlDevice)) {
      pDeviceRef = event->getRaisedAtDevice();
      createHeader(json, evtGroup_Activity, evtCategory_DeviceCustomActionChangedEvent, event.get());
      json.startObject("EventBody");
      json.add("CustomActionId", event->getPropertyByName("customActionId"));
      json.add("ActionId", event->getPropertyByName("actionId"));
      json.add("CustomActionTitle", event->getPropertyByName("customActionTitle"));
      json.add("DeviceID", pDeviceRef->getDSID());
      json.startArray("Parameter");
      foreach (auto&& param, event->getProperties().getContainer()) {
        if (beginsWith(param.first, "params.")) {
          json.startObject();
          json.add("Name", param.first.substr(7));
          json.add("Value", param.second);
          json.endObject();
        }
      }
      json.endArray();
      json.endObject();

    // heating
    } else if (event->getName() == EventName::HeatingControllerSetup) {
      createHeader(json, evtGroup_ApartmentAndDevice, evtCategory_HeatingControllerSetup, event.get());
      json.startObject("EventBody");
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
      json.endObject();
    } else if (event->getName() == EventName::HeatingControllerValueDsHub) {
      createHeader(json, evtGroup_ApartmentAndDevice, evtCategory_HeatingControllerValue, event.get());
      json.startObject("EventBody");
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
      json.endObject();
    } else if (event->getName() == EventName::HeatingControllerState) {
      createHeader(json, evtGroup_ApartmentAndDevice, evtCategory_HeatingControllerState, event.get());
      json.startObject("EventBody");
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
      json.endObject();
    } else if (event->getName() == EventName::HeatingEnabled) {
      createHeader(json, evtGroup_ApartmentAndDevice, evtCategory_HeatingEnabled, event.get());
      json.startObject("EventBody");
      json.add("HeatingEnabled", event->getPropertyByName("HeatingEnabled") == "true" ? true : false);
      json.add("CoolingEnabled", event->getPropertyByName("CoolingEnabled") == "true" ? true : false);
      json.endObject();
    } else if (event->getName() == EventName::HeatingSystemCapability) {
      createHeader(json, evtGroup_ApartmentAndDevice, evtCategory_HeatingSystemCapability, event.get());
      json.startObject("EventBody");
      json.add("HeatingSupported", event->getPropertyByName("HeatingSupported") == "true" ? true : false);
      json.add("CoolingSupported", event->getPropertyByName("CoolingSupported") == "true" ? true : false);
      json.endObject();

    // configuration
    } else if (event->getName() == EventName::AddonToCloud) {
      createHeader(json, evtGroup_ApartmentAndDevice, evtCategory_AddonToCloud, event.get());
      json.startObject("EventBody");
      json.add("EventName", event->getPropertyByName("EventName"));
      json.startObject("Parameter");
      foreach (auto&& param, event->getProperties().getContainer()) {
        json.add(param.first, param.second);
      }
      json.endObject();
      json.endObject();

    // misc
    } else if (event->getName() == EventName::ExecutionDenied) {
      /* TODO is event group and category correct? */
      createHeader(json, evtGroup_Activity, evtCategory_ExecutionDenied, event.get());
      json.startObject("EventBody");
      json.add("ActivityType", event->getPropertyByName("action-type"));
      json.add("ActivityName", event->getPropertyByName("action-name"));
      json.add("SourceName", event->getPropertyByName("source-name"));
      json.add("Reason", event->getPropertyByName("reason"));
      json.endObject();
    } else if (event->getName() == EventName::LogFileData) {
      createHeader(json, evtGroup_Activity, evtCategory_LogFileData, event.get());
      json.startObject("EventBody");
      json.endObject();
    } else {
      throw DSSException(std::string("unhandled event ") + event->toString());
    }

  } catch (std::invalid_argument& e) {
    throw DSSException(std::string("Error converting event ") + event->toString());
  }
  return;
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
  return propSystem.getBoolValue(pp_websvc_dshub_active);
}

template <class Iterator>
bool WebserviceDsHub::doUploadSensorData(Iterator begin, Iterator end,
                                         WebserviceCallDone_t callback) {

  JSONWriter json(JSONWriter::jsonNoneResult);
  int ct = 0;

  // /v1/DSS/{dssID}/Events
  std::string url;
  url = "v1/DSS/" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue() + "/Events";

  json.startArray("Events");
  for (; begin != end; begin++) {
    try {
      json.startObject();
      DsHub::toJson(*begin, json);
      json.endObject();
      ct++;
    } catch (DSSException& e) {
      json.endObject();
      log(e.what(), lsWarning);
    }
  }
  json.endArray();

  if (ct == 0) {
    return false;
  }

  std::string postdata = json.successJSON();
  // TODO eventually emit summary: 10 callsceene, 27 metering, ...
  // dumping whole postdata leads to log rotation and is seldomly needed
  // unless for solving a blame war with cloud team
  log("upload events: " + intToString(ct) + " bytes: " + intToString(postdata.length()), lsInfo);
  log("event data: " + postdata, lsDebug);

  // https://devdsservices.aizo.com/Help/Api/POST-public-dss-v1_0-DSSEventData-SaveEvent_token_apartmentId_dssid_source
  boost::shared_ptr<DsHubReplyChecker> mcb = boost::make_shared<DsHubReplyChecker>(callback);
  std::unordered_map<std::string, std::string> sensorUploadHeaders;
  sensorUploadHeaders["Content-Type"] = "application/json;charset=UTF-8";
  sensorUploadHeaders["Accept"] = "application/json";

  WebserviceConnection::getInstanceDsHub()->request(url,
                                                    "",
                                                    sensorUploadHeaders,
                                                    postdata,
                                                    mcb,
                                                    true);
  return true;
}

template bool WebserviceDsHub::doUploadSensorData<ItEvent>(
      ItEvent begin, ItEvent end, WebserviceCallDone_t callback);

} /* namespace dss */

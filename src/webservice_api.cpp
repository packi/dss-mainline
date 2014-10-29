#include "webservice_api.h"

#include <json.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include "dss.h"
#include "propertysystem.h"

namespace dss {

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

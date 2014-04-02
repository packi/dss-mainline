#include "webservice_api.h"
#ifdef HAVE_CURL

#include <json/json.h>
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
        } else if (!strcmp(key, "ReturnMessage")) {
            if (type != json_type_string) {
                throw ParseError("invalid type for ReturnMessage");
            }
            resp.desc = json_object_get_string(val);
        } else {
            throw ParseError("invalid key for message");
        }
    }
    return resp;
}

__DEFINE_LOG_CHANNEL__(StatusReplyChecker, lsInfo);

void StatusReplyChecker::result(long code, boost::shared_ptr<URLResult> result) {
  if (code != 200) {
    log("HTTP POST failed " + intToString(code), lsError);
    return;
  }

  try {
    WebserviceReply resp = parse_reply(result->content());
    if (resp.code != 0) {
      log(resp.desc, lsError);
      return;
    }
  } catch (ParseError &ex) {
    log(std::string("Invalid return message ") + result->content(), lsError);
    return;
  }
}

}

#endif // ifdef HAVE_CURL

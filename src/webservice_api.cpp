#include <json/json.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include "webservice_api.h"

namespace dss {

ParseError::ParseError(const std::string& _message) : runtime_error( _message )
{
}
/*Parsing the json object*/
ModelChangeResponse parseModelChange(const char* buf)
{
    ModelChangeResponse resp;

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

}

/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/


#include <iostream>
#include <json.h>

#include <boost/pointer_cast.hpp>
#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>

#include "src/web/json.h"
#include "src/web/webrequests.h"

namespace dss {

  class WebFixture {
  public:

    void testOkIs(WebServerResponse& _response, bool _value) {
      std::string result = _response.getResponse();
      struct json_tokener* tok;

      tok = json_tokener_new();
      json_object* json_request = json_tokener_parse_ex(tok, result.c_str(), -1);

      bool resultCode = false;
      if (tok->err == json_tokener_success) {
        json_object* obj;
        if (json_object_object_get_ex(json_request, "ok", &obj)) {
          resultCode = json_object_get_boolean(obj);
        } else {
          BOOST_ASSERT(false);
        }
      }
      json_object_put(json_request);
      json_tokener_free(tok);

      BOOST_ASSERT(resultCode == _value);
    }

//    boost::shared_ptr<JSONObject> getSubObject(const std::string& _name, boost::shared_ptr<JSONElement> _elem) {
//      boost::shared_ptr<JSONElement> resultElem = _elem->getElementByName(_name);
//      if(resultElem == NULL) {
//        BOOST_ASSERT(false);
//      }
//      boost::shared_ptr<JSONObject> result = boost::dynamic_pointer_cast<JSONObject>(resultElem);
//      BOOST_ASSERT(result != NULL);
//      return result;
//    }
//
//    boost::shared_ptr<JSONObject> getResultObject(WebServerResponse& _response) {
//      testOkIs(_response, true);
//      struct json_tokener* tok;
//
//      tok = json_tokener_new();
//      json_object* json_request = json_tokener_parse_ex(tok, _response.getResponse().c_str(), -1);
//
//      json_object* result;
//      if (tok->err == json_tokener_success) {
//        json_object* obj;
//        if (json_object_object_get_ex(json_request, "result", &obj)) {
//          result = json_object_object_get_ex(obj);
//        } else {
//          BOOST_ASSERT(false);
//        }
//      }
//      json_object_put(json_request);
//      json_tokener_free(tok);
//      BOOST_ASSERT(result != NULL);
//      return result;
//    }

    void checkPropertyEquals(const std::string& _name, const int& _expected, WebServerResponse& _response) {
      struct json_tokener* tok;

      tok = json_tokener_new();
      json_object* json_request = json_tokener_parse_ex(tok, _response.getResponse().c_str(), -1);

      if (tok->err == json_tokener_success) {
        json_object* obj;
        if (json_object_object_get_ex(json_request, "result", &obj)) {
          json_object* cont;
          if (json_object_object_get_ex(obj, _name.c_str(), &cont)) {
            switch (json_object_get_type(cont)) {
            case json_type_int:
            {
              int result = json_object_get_int(cont);
              BOOST_CHECK_EQUAL(result, _expected);
              break;
            }
            default:
              BOOST_ASSERT(false);
          }
        } else {
          BOOST_ASSERT(false);
        }

      }
      json_object_put(json_request);
      json_tokener_free(tok);
    }
    }
    void checkPropertyEquals(const std::string& _name, const std::string& _expected, WebServerResponse& _response) {
      struct json_tokener* tok;

      tok = json_tokener_new();
      json_object* json_request = json_tokener_parse_ex(tok, _response.getResponse().c_str(), -1);

      if (tok->err == json_tokener_success) {
        json_object* obj;
        if (json_object_object_get_ex(json_request, "result", &obj)) {
          json_object* cont;
          if (json_object_object_get_ex(obj, _name.c_str(), &cont)) {
            switch (json_object_get_type(cont)) {
            case json_type_string:
            case json_type_array:
            case json_type_object:
            {
              std::string result = json_object_get_string(cont);
              BOOST_CHECK_EQUAL(result, _expected);
              break;
            }
            default:
              BOOST_ASSERT(false);
            }
          }
        } else {
          BOOST_ASSERT(false);
        }

      }
      json_object_put(json_request);
      json_tokener_free(tok);
    }
    void checkPropertyEquals(const std::string& _name, const bool& _expected, WebServerResponse& _response) {
      struct json_tokener* tok;

      tok = json_tokener_new();
      json_object* json_request = json_tokener_parse_ex(tok, _response.getResponse().c_str(), -1);

      if (tok->err == json_tokener_success) {
        json_object* obj;
        if (json_object_object_get_ex(json_request, "result", &obj)) {
          json_object* cont;
          if (json_object_object_get_ex(obj, _name.c_str(), &cont)) {
            switch (json_object_get_type(cont)) {
            case json_type_boolean:
            {
              bool result = json_object_get_boolean(cont);
              BOOST_CHECK_EQUAL(result, _expected);
              break;
            }
            default:
              BOOST_ASSERT(false);
            }
          }
        } else {
          BOOST_ASSERT(false);
        }

      }
      json_object_put(json_request);
      json_tokener_free(tok);
    }

    void checkPropertyEquals(const std::string& _name, const double& _expected, WebServerResponse& _response) {
      struct json_tokener* tok;

      tok = json_tokener_new();
      json_object* json_request = json_tokener_parse_ex(tok, _response.getResponse().c_str(), -1);

      if (tok->err == json_tokener_success) {
        json_object* obj;
        if (json_object_object_get_ex(json_request, "result", &obj)) {
          json_object* cont;
          if (json_object_object_get_ex(obj, _name.c_str(), &cont)) {
            switch (json_object_get_type(cont)) {
            case json_type_double:
            {
              double result = json_object_get_double(cont);
              BOOST_CHECK_EQUAL(result, _expected);
              break;
            }
            default:
              BOOST_ASSERT(false);
            }
          }
        } else {
          BOOST_ASSERT(false);
        }

      }
      json_object_put(json_request);
      json_tokener_free(tok);
    }
  }; // WebFixture

}


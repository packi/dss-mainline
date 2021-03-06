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

#ifndef WEBREQUESTS_H
#define WEBREQUESTS_H

#include "src/web/restful.h"

#include <string>
#include <boost/shared_ptr.hpp>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/encodings.h>

#include "src/logger.h"
#include "src/ds485types.h"
#include "src/dss.h"
#include "src/model/modelconst.h"
#include "src/foreach.h"

#include <external/civetweb/civetweb.h>

using rapidjson::StringBuffer;
using rapidjson::Writer;

namespace dss {

  class RestfulRequest;
  class Session;
  
  class WebServerResponse {
  public:
    WebServerResponse(std::string _response)
    : m_response(_response), m_revokeCookie(false)
    { }
    std::string getResponse() const {
      return m_response;
    }
    void setRevokeSessionToken() {
      m_newSessionToken.clear();
      m_revokeCookie = true;
    }
    bool isRevokeSessionToken() const {
      return m_revokeCookie;
    }
    void setPublishSessionToken(const std::string &token) {
      m_newSessionToken = token;
    }
    bool isPublishSessionToken() const {
      return !m_newSessionToken.empty();
    }
    const std::string &getNewSessionToken() const {
      return m_newSessionToken;
    }
  private:
    std::string m_response;
    std::string m_newSessionToken;
    bool m_revokeCookie;
  };

  class JSONWriter {
  public:
    typedef enum {
      jsonObjectResult,
      jsonArrayResult,
      jsonNoneResult
    } jsonResult_t;
    JSONWriter(jsonResult_t _responseType = jsonObjectResult);
    std::string successJSON();
    void add(std::string _name, std::string _value);
    void add(std::string _name, const char* _value);
    void add(std::string _name, int _value);
    void add(std::string _name, unsigned _value);
    void add(std::string _name, long long int _value);
    void add(std::string _name, unsigned long long int _value);
    void add(std::string _name, bool _value);
    void add(std::string _name, double _value);
    void add(std::string name, const dsuid_t &dsuid) { add(name, dsuid2str(dsuid)); }
    void add(const std::string &value);
    void add(const char* _value);
    void add(int _value);
    void add(unsigned _value);
    void add(long long int _value);
    void add(unsigned long long int _value);
    void add(bool _value);
    void add(double _value);
    void add(const dsuid_t &dsuid) { add(dsuid2str(dsuid)); }

    // serialize all enums as ints
    template <typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
    void add(T x) { add(static_cast<int>(x)); }
    template <typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
    void add(const std::string& name, T x) { add(name); add(x); }

    template <typename T>
    void add(const std::string& name, const std::vector<T>& items) {
      add(name);
      startArray();
      foreach(auto&& item, items) {
        add(item);
      }
      endArray();
    }

    // Add object key. Return false if the object entry cannot be serialized and is skipped.
    bool addKey(const char* x) { add(x); return true; }
    bool addKey(const std::string& x) { add(x); return true; }
    bool addKey(ModelFeatureId x);

    template <class Key, class T, class Compare, class Alloc>
    void add(const std::string& name, const std::map<T, Key, Compare, Alloc>& map) {
      add(name);
      startObject();
      foreach(auto&& entry, map) {
        if (addKey(entry.first)) {
          add(entry.second);
        }
      }
      endObject();
    }

    void addNull();
    void startArray(std::string _name);
    void startArray();
    void endArray();
    void startObject(std::string _name);
    void startObject();
    void endObject();

    std::string raw();
    static std::string success();
    static std::string success(std::string _message);
    static std::string failure(std::string _message);
  private:
    StringBuffer m_buffer;
    Writer<StringBuffer, rapidjson::UTF8<>, rapidjson::ASCII<> > m_writer;
    jsonResult_t m_resultType;
  };

  class WebServerRequestHandlerJSON {
  public:
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request,
                                                boost::shared_ptr<Session> _session, 
                                                const struct mg_connection* _connection) = 0;
    virtual ~WebServerRequestHandlerJSON() {}
  protected:
    void log(const std::string& _line, aLogSeverity _severity = lsDebug) {
        Logger::getInstance()->log("RequestHandler: " + _line, _severity);
    }
  }; // WebServerRequestHandlerJSON



}

#endif // WEBREQUESTS_H

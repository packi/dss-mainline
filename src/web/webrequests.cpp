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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "webrequests.h"

#include <boost/shared_ptr.hpp>

namespace dss {


  //================================================== WebServerRequestHandlerJSON

  std::string WebServerRequestHandlerJSON::handleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    WebServerResponse response = jsonHandleRequest(_request, _session);
    return response.getResponse();
  } // handleRequest


  JSONWriter::JSONWriter(jsonResult_t _responseType) : m_writer(m_buffer), m_resultType(_responseType) {
    startObject();
    switch (m_resultType) {
    case jsonObjectResult:
      startObject("result");
      break;
    case jsonArrayResult:
      startArray("result");
      break;
    case jsonNoneResult:
      break;
    }
  }

  std::string JSONWriter::successJSON() {
    switch (m_resultType) {
    case jsonObjectResult:
      endObject();
      add("ok", true);
      break;
    case jsonArrayResult:
      endArray();
      add("ok", true);
      break;
    case jsonNoneResult:
      break;
    }
    endObject();
    return m_buffer.GetString();
  }
  void JSONWriter::add(std::string _name, std::string _value) {
    add(_name, _value.c_str());
  }
  void JSONWriter::add(std::string _name, const char* _value) {
    m_writer.String(_name);
    m_writer.String(_value);
  }
  void JSONWriter::add(std::string _name, int _value) {
    m_writer.String(_name);
    m_writer.Int(_value);
  }
  void JSONWriter::add(std::string _name, unsigned _value) {
    m_writer.String(_name);
    m_writer.Uint(_value);
  }
  void JSONWriter::add(std::string _name, long long int _value) {
    m_writer.String(_name);
    m_writer.Int64(_value);
  }
  void JSONWriter::add(std::string _name, unsigned long long int _value) {
    m_writer.String(_name);
    m_writer.Uint64(_value);
  }
  void JSONWriter::add(std::string _name, bool _value) {
    m_writer.String(_name);
    m_writer.Bool(_value);
  }
  void JSONWriter::add(std::string _name, double _value) {
    m_writer.String(_name);
    m_writer.Double(_value);
  }
  void JSONWriter::add(std::string _value) {
    add(_value.c_str());
  }
  void JSONWriter::add(const char* _value) {
    m_writer.String(_value);
  }
  void JSONWriter::add(int _value) {
    m_writer.Int(_value);
  }
  void JSONWriter::add(unsigned _value) {
    m_writer.Uint(_value);
  }
  void JSONWriter::add(long long int _value) {
    m_writer.Int64(_value);
  }
  void JSONWriter::add(unsigned long long int _value) {
    m_writer.Uint64(_value);
  }
  void JSONWriter::add(bool _value) {
    m_writer.Bool(_value);
  }
  void JSONWriter::add(double _value) {
    m_writer.Double(_value);
  }
  void JSONWriter::startArray(std::string _name) {
    m_writer.String(_name);
    m_writer.StartArray();
  }
  void JSONWriter::startArray() {
    m_writer.StartArray();
  }
  void JSONWriter::endArray() {
    m_writer.EndArray();
  }
  void JSONWriter::startObject(std::string _name) {
    m_writer.String(_name);
    m_writer.StartObject();
  }
  void JSONWriter::startObject() {
    m_writer.StartObject();
  }
  void JSONWriter::endObject() {
    m_writer.EndObject();
  }
  std::string JSONWriter::success() {
    return success("");
  }
  std::string JSONWriter::success(std::string _message) {
    StringBuffer s;
    Writer<StringBuffer> writer(s);
    writer.StartObject();
    writer.String("ok"); writer.Bool(true);
    if (!_message.empty()) {
      writer.String("message");
      writer.String(_message);
    }
    writer.EndObject();
    return s.GetString();
  }
  std::string JSONWriter::failure(std::string _message) {
    StringBuffer s;
    Writer<StringBuffer> writer(s);
    s.Clear();
    writer.Reset(s);
    writer.StartObject();
    writer.String("ok"); writer.Bool(false);
    if (!_message.empty()) {
      writer.String("message");
      writer.String(_message);
    }
    writer.EndObject();
    return s.GetString();
  }

} // namespace dss

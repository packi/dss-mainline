/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "config.h"

#ifdef HAVE_CURL

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "propertysystem.h"
#include "webservice_connection.h"

namespace dss {

WebserviceConnection* WebserviceConnection::m_instance = NULL;

WebserviceConnection::WebserviceConnection()
  : m_handle(NULL)
{
    m_base_url = DSS::getInstance()->getPropertySystem().getStringValue("/config/webservice-api/base-url");
    if (!endsWith(m_base_url, "/")) {
      m_base_url = m_base_url + "/";
    }
    m_url = boost::shared_ptr<URL>(new URL(true));
}

WebserviceConnection::~WebserviceConnection()
{
}

WebserviceConnection* WebserviceConnection::getInstance()
{
  if (m_instance == NULL) {
    m_instance = new WebserviceConnection();
  }
  assert (m_instance != NULL);
  return m_instance;
}

void WebserviceConnection::shutdown() {
  if (m_instance != NULL) {
    WebserviceConnection* inst = m_instance;
    m_instance = NULL;
    delete inst;
  }
}

void WebserviceConnection::request(const std::string& url, RequestType type,
                                   boost::shared_ptr<URLRequestCallback> cb)
{
    boost::shared_ptr<WebserviceConnection::URLRequestTask>task(
            new WebserviceConnection::URLRequestTask(
                m_url, m_base_url, url, type, cb));
    addEvent(task);
}

void WebserviceConnection::request(const std::string& url,
                                 boost::shared_ptr<HashMapStringString> headers,
                                 const std::string& postdata,
                                 boost::shared_ptr<URLRequestCallback> cb)
{
    boost::shared_ptr<WebserviceConnection::URLRequestTask>task(
            new WebserviceConnection::URLRequestTask(
                m_url, m_base_url, url, headers, postdata, cb));
    addEvent(task);
}


void WebserviceConnection::request(const std::string& url, RequestType type,
                                boost::shared_ptr<HashMapStringString> headers,
                                boost::shared_ptr<HashMapStringString> formpost,
                                   boost::shared_ptr<URLRequestCallback> cb)
{
    boost::shared_ptr<WebserviceConnection::URLRequestTask>task(
            new WebserviceConnection::URLRequestTask(
                m_url, m_base_url, url, type, headers, formpost, cb));
    addEvent(task);
}

WebserviceConnection::URLRequestTask::URLRequestTask(boost::shared_ptr<URL> req,
                                                     const std::string& base,
                                                     const std::string& url,
                                                     RequestType type,
                                    boost::shared_ptr<URLRequestCallback> cb) :
                                                     m_req(req),
                                                     m_base_url(base),
                                                     m_url(url), m_type(type),
                                                     m_result(NULL),
                                                     m_cb(cb),
                                                     m_simple(true)

{
}

WebserviceConnection::URLRequestTask::URLRequestTask(boost::shared_ptr<URL> req,
                                                     const std::string& base,
                                                     const std::string& url,
                               boost::shared_ptr<HashMapStringString> headers,
                                                     const std::string& postdata,
                                    boost::shared_ptr<URLRequestCallback> cb) :
                                                     m_req(req),
                                                     m_base_url(base),
                                                     m_url(url), m_type(POST),
                                                     m_postdata(postdata),
                                                     m_headers(headers),
                                                     m_result(NULL),
                                                     m_cb(cb),
                                                     m_simple(false)

{
}


WebserviceConnection::URLRequestTask::URLRequestTask(boost::shared_ptr<URL> req,
                                                     const std::string& base,
                                                     const std::string& url,
                                                     RequestType type,
                               boost::shared_ptr<HashMapStringString> headers,
                               boost::shared_ptr<HashMapStringString> formpost,
                                    boost::shared_ptr<URLRequestCallback> cb) :
                                                     m_req(req),
                                                     m_base_url(base),
                                                     m_url(url), m_type(type),
                                                     m_headers(headers),
                                                     m_formpost(formpost),
                                                     m_result(NULL),
                                                     m_cb(cb),
                                                     m_simple(false)

{
}


void WebserviceConnection::URLRequestTask::run()
{
    long code;

    if (m_req == NULL) {
      return;
    }

    boost::shared_ptr<URLResult> result(new URLResult());
    if (m_simple) {
      code = m_req->request(m_base_url + m_url, m_type, result.get());
    } else {
      if (m_postdata.empty()) {
        code = m_req->request(m_base_url + m_url, m_type, m_headers,
                              m_formpost, result.get());
      } else {
        code = m_req->request(m_base_url + m_url, m_headers, m_postdata, result.get());
      }
    }
    if (m_cb != NULL) {
      m_cb->result(code, result);
    }
}

} // namespace dss

#endif//HAVE_CURL

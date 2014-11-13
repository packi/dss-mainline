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

#include "webservice_connection.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "event.h"
#include "propertysystem.h"
#include "webservice_api.h"

namespace dss {

/****************************************************************************/
/* WebserviceConnectionMsHub                                                */
/****************************************************************************/

class WebserviceConnectionMsHub : public WebserviceConnection {
public:
  WebserviceConnectionMsHub();
private:
  virtual void authorizeRequest(HttpRequest& req, bool hasUrlParameters);
};

WebserviceConnectionMsHub::WebserviceConnectionMsHub()
  : WebserviceConnection(pp_websvc_url_authority)
{
}

void WebserviceConnectionMsHub::authorizeRequest(HttpRequest& req, bool hasUrlParameters)
{
  std::string osptoken = DSS::getInstance()->getPropertySystem().getStringValue(pp_websvc_rc_osptoken);

  if (!osptoken.empty()) {
    std::string default_webservice_param = "token=" + osptoken;

    if (hasUrlParameters) {
      req.url = req.url + "&" + default_webservice_param;
    } else {
      req.url = req.url + "?" + default_webservice_param;
    }
  } else {
    log("authentication parameters enabled, but token could not be "
        "fetched!", lsError);
  }
}


/****************************************************************************/
/* WebserviceConnection                                                     */
/****************************************************************************/

__DEFINE_LOG_CHANNEL__(WebserviceConnection, lsInfo)

WebserviceConnection* WebserviceConnection::m_instance_mshub = NULL;

WebserviceConnection::WebserviceConnection(const char *pp_base_url)
{
  m_base_url = DSS::getInstance()->getPropertySystem().getStringValue(pp_base_url);
  if (!endsWith(m_base_url, "/")) {
    m_base_url = m_base_url + "/";
  }
  m_url = boost::shared_ptr<HttpClient>(new HttpClient(true));
}

WebserviceConnection::~WebserviceConnection()
{
}

WebserviceConnection* WebserviceConnection::getInstanceMsHub()
{
  if (m_instance_mshub == NULL) {
    m_instance_mshub = new WebserviceConnectionMsHub();
  }
  assert (m_instance_mshub != NULL);
  return m_instance_mshub;
}

void WebserviceConnection::shutdown() {
  if (m_instance_mshub != NULL) {
    WebserviceConnection* inst = m_instance_mshub;
    m_instance_mshub = NULL;
    delete inst;
  }
}

std::string WebserviceConnection::constructURL(const std::string& url,
                                               const std::string& parameters)
{
  std::string ret = m_base_url + url;
  if (!parameters.empty()) {
    ret = ret + "?" + parameters;
  }
  return ret;
}

void WebserviceConnection::request(boost::shared_ptr<HttpRequest> req,
                                   boost::shared_ptr<URLRequestCallback> cb,
                                   bool hasUrlParameters,
                                   bool authenticated)
{
  if (authenticated) {
    authorizeRequest(*req, hasUrlParameters);
  }

  boost::shared_ptr<URLRequestTask> task(new URLRequestTask(m_url, req, cb));
  addEvent(task);
}

void WebserviceConnection::request(const std::string& url,
                                   const std::string& parameters,
                                   RequestType type,
                                   boost::shared_ptr<URLRequestCallback> cb,
                                   bool authenticated)
{
  boost::shared_ptr<HttpRequest> req(new HttpRequest);
  req->url = constructURL(url, parameters);
  req->type = type;

  request(req, cb, !parameters.empty(), authenticated);
}

void WebserviceConnection::request(const std::string& url,
                                 const std::string &parameters,
                                 boost::shared_ptr<HashMapStringString> headers,
                                 const std::string& postdata,
                                 boost::shared_ptr<URLRequestCallback> cb,
                                 bool authenticated)
{
  boost::shared_ptr<HttpRequest> req(new HttpRequest);
  req->url = constructURL(url, parameters);
  req->type = POST;
  req->headers = headers;
  req->postdata = postdata;

  request(req, cb, !parameters.empty(), authenticated);
}


void WebserviceConnection::request(const std::string& url,
                                const std::string& parameters,
                                RequestType type,
                                boost::shared_ptr<HashMapStringString> headers,
                                boost::shared_ptr<URLRequestCallback> cb,
                                bool authenticated)
{
  boost::shared_ptr<HttpRequest> req(new HttpRequest);
  req->url = constructURL(url, parameters);
  req->type = type;
  req->headers = headers;

  request(req, cb, !parameters.empty(), authenticated);
}


/****************************************************************************/
/* URLRequestTask                                                           */
/****************************************************************************/

__DEFINE_LOG_CHANNEL__(URLRequestTask, lsInfo)

URLRequestTask::URLRequestTask(boost::shared_ptr<HttpClient> client,
                               boost::shared_ptr<HttpRequest> req,
                               boost::shared_ptr<URLRequestCallback> cb)
  : m_client(client), m_req(req), m_cb(cb)
{
}

void URLRequestTask::run()
{
  std::string result;
  long code;

  if (m_client == NULL) {
    return;
  }

  log("URLRequestTask::run(): sending request to " + m_req->url, lsDebug);

  code = m_client->request(*m_req, &result);
  log("URLRequestTask::run(): request to " + m_req->url + " returned with HTTP code " +
      intToString(code), lsDebug);

  if (m_cb != NULL) {
    m_cb->result(code, result);
  }
}

} // namespace

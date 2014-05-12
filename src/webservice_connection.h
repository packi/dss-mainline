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

#ifndef __DSS_WEBSERVICE_CONNECTION_H__
#define __DSS_WEBSERVICE_CONNECTION_H__

#include <string>
#include <boost/shared_ptr.hpp>

#include "dss.h"
#include "http_client.h"
#include "logger.h"
#include "propertysystem.h"
#include "taskprocessor.h"

namespace dss {

class URLRequestCallback
{
public:
  virtual void result(long code, const std::string &res) = 0;
};

class WebserviceConnection : public TaskProcessor {
  __DECL_LOG_CHANNEL__
public:
  WebserviceConnection();
  virtual ~WebserviceConnection();
  void request(const std::string& url,
               const std::string &parameters,
               RequestType type,
               boost::shared_ptr<URLRequestCallback> cb,
               bool authenticated = false);
  void request(const std::string& url,
               const std::string &parameters,
               boost::shared_ptr<HashMapStringString> headers,
               const std::string& postdata,
               boost::shared_ptr<URLRequestCallback> cb,
               bool authenticated = false);
  void request(const std::string& url,
               const std::string &parameters,
               RequestType type,
               boost::shared_ptr<HashMapStringString> headers,
               boost::shared_ptr<HashMapStringString> formpost,
               boost::shared_ptr<URLRequestCallback> cb,
               bool authenticated = false);
  static WebserviceConnection *getInstance();
  static void shutdown();

private:
  static WebserviceConnection *m_instance;

  std::string m_base_url;
  boost::shared_ptr<HttpClient> m_url;

  std::string constructURL(const std::string& url,
                           const std::string& parameters, bool authenticated);
};

class URLRequestTask : public Task {
  __DECL_LOG_CHANNEL__
public:
  URLRequestTask(boost::shared_ptr<HttpClient> client,
                 boost::shared_ptr<HttpRequest> req,
                 boost::shared_ptr<URLRequestCallback> cb);
  virtual ~URLRequestTask() {};
  virtual void run();
private:
  boost::shared_ptr<HttpClient> m_client;
  boost::shared_ptr<HttpRequest> m_req;
  boost::shared_ptr<URLRequestCallback> m_cb;
};

} // namespace dss

#endif//__DSS_WEBSERVICE_CONNECTION_H__

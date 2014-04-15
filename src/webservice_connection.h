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

#ifdef HAVE_CURL

#include <string>
#include <boost/shared_ptr.hpp>

#include "dss.h"
#include "taskprocessor.h"
#include "url.h"
#include  "propertysystem.h"
#include "logger.h"

namespace dss {

class URLRequestCallback
{
public:
  virtual void result(long code, boost::shared_ptr<URLResult> res) = 0;
};

class WebserviceConnection : public TaskProcessor {
public:
  __DECL_LOG_CHANNEL__
  WebserviceConnection();
  virtual ~WebserviceConnection();
  void request(const std::string& url, RequestType type,
               boost::shared_ptr<URLRequestCallback> cb);
  void request(const std::string& url,
               boost::shared_ptr<HashMapStringString> headers,
               const std::string& postdata,
               boost::shared_ptr<URLRequestCallback> cb);
  void request(const std::string& url, RequestType type,
               boost::shared_ptr<HashMapStringString> headers,
               boost::shared_ptr<HashMapStringString> formpost,
               boost::shared_ptr<URLRequestCallback> cb);
  static WebserviceConnection *getInstance();
  static void shutdown();

private:
  static WebserviceConnection *m_instance;

  std::string m_base_url;
  boost::shared_ptr<URL> m_url;

  class URLRequestTask : public Task {
  public:
    URLRequestTask(boost::shared_ptr<URL> req,
                   const std::string& base, const std::string& url,
                   RequestType type,
                   boost::shared_ptr<URLRequestCallback> cb);
    URLRequestTask(boost::shared_ptr<URL> req,
                   const std::string& base, const std::string& url,
                   boost::shared_ptr<HashMapStringString> headers,
                   const std::string& postdata,
                   boost::shared_ptr<URLRequestCallback> cb);
    URLRequestTask(boost::shared_ptr<URL> req, 
                   const std::string& base, const std::string& url,
                   RequestType type,
                   boost::shared_ptr<HashMapStringString> headers,
                   boost::shared_ptr<HashMapStringString> formpost,
                   boost::shared_ptr<URLRequestCallback> cb);
    virtual ~URLRequestTask() {};
    virtual void run();
  private:
    boost::shared_ptr<URL> m_req;
    std::string m_base_url;
    std::string m_url;
    RequestType m_type;
    std::string m_postdata;
    boost::shared_ptr<HashMapStringString> m_headers;
    boost::shared_ptr<HashMapStringString> m_formpost;
    boost::shared_ptr<URLRequestCallback> m_cb;
    bool m_simple;
  };
};

class WebserviceTreeListener : public PropertyListener {
public:
  WebserviceTreeListener(PropertyNodePtr _pWebserviceApiEnabledNode);
  virtual ~WebserviceTreeListener();
protected:
  virtual void propertyChanged(PropertyNodePtr _caller,
                               PropertyNodePtr _changedNode);
private:
   PropertyNodePtr m_pWebserviceApiEnabledNode;
};

} // namespace dss

#endif//HAVE_CURL

#endif//__DSS_WEBSERVICE_CONNECTION_H__


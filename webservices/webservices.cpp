/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "webservices.h"
#include "core/propertysystem.h"
#include "core/sessionmanager.h"
#include "core/eventcollector.h"

#include <cassert>
#include <cstdlib>

#include <boost/foreach.hpp>

#include "core/dss.h"
#include "core/eventsubscriptionsession.h"
#include "core/session.h"

namespace dss {
  //================================================== WebServices

  WebServices::WebServices(DSS* _pDSS)
  : ThreadedSubsystem(_pDSS, "WebServices")
  {
  } // ctor

  WebServices::~WebServices() {
  } // dtor

  void WebServices::doStart() {
    run();
  } // doStart

  void WebServices::initialize() {
    Subsystem::initialize();
    log("initializing WebServices");
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "port", 8081, true, false);
    getDSS().getPropertySystem().setStringValue(
       getConfigPropertyBasePath() + "sslcert", 
         getDSS().getPropertySystem().getStringValue("/config/configdirectory")
            + "dsscert.pem" , true, false);
    std::string sslcert = getDSS().getPropertySystem().getStringValue(
                                  getConfigPropertyBasePath() + "sslcert");

    if (m_Service.ssl_server_context(SOAP_SSL_DEFAULT, sslcert.c_str(),
          NULL, sslcert.c_str(), NULL, NULL, NULL, "dss")) {
      throw std::runtime_error("Could not set ssl server context!");
    }

    int soapServerSocket = m_Service.bind(NULL, getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "port"), 10);
    if (soapServerSocket == SOAP_INVALID_SOCKET) {
      throw std::runtime_error("Could not bind to SOAP port");
    }
  }

  void WebServices::execute() {
    for(int iWorker = 0; iWorker < 4; iWorker++) {
      m_Workers.push_back(new WebServicesWorker(this));
      m_Workers.back().run();
    }
    while(!m_Terminated) {
      int socket = m_Service.accept();
      if(socket != SOAP_INVALID_SOCKET) {
        struct soap* req_copy = soap_copy(&m_Service);
        if (!req_copy) {
          soap_closesock(req_copy);
          continue;
        }

        if (soap_ssl_accept(req_copy)) {
          soap_print_fault(req_copy, stderr);
          Logger::getInstance()->log("WebService::execute: SSL request failed!", lsError);
          soap_destroy(req_copy);
          soap_end(req_copy);
          soap_free(req_copy);
          continue;
        }

        m_RequestsMutex.lock();
        m_PendingRequests.push_back(req_copy);
        m_RequestsMutex.unlock();
        m_RequestArrived.signal();
      } else if (!((socket == -1) && (m_Service.error == SOAP_TCP_ERROR))) {
        log("could not accept new connection!", lsError);
      }
    }

    while(!m_Workers.empty()) {
      m_Workers.erase(m_Workers.begin());
    }
  } // execute

  struct soap* WebServices::popPendingRequest() {
    struct soap* result = NULL;
    while(!m_Terminated) {
      m_RequestsMutex.lock();
      if(!m_PendingRequests.empty()) {
        result = m_PendingRequests.front();
        m_PendingRequests.pop_front();
      }
      m_RequestsMutex.unlock();
      if(result == NULL) {
        m_RequestArrived.waitFor(1000);
      } else {
        break;
      }
    }
    return result;
  } // popPendingRequest

  std::string WebServices::newSession(soap* _soapRequest) {
    return m_pSessionManager->registerSession();
  } // newSession

  void WebServices::deleteSession(soap* _soapRequest, const std::string& _token) {
    m_pSessionManager->removeSession(_token);
  } // deleteSession

  bool WebServices::isAuthorized(soap* _soapRequest, const std::string& _token) {
    boost::shared_ptr<Session> session = m_pSessionManager->getSession(_token);
    return (session != NULL ? session->isStillValid() : false);
  } // isAuthorized

  boost::shared_ptr<Session> WebServices::getSession(soap* _soapRequest, const std::string& _token) {
    return m_pSessionManager->getSession(_token);
  }

  boost::shared_ptr<EventCollector> extractFromSession(Session* _session) {
    const std::string kDataIdentifier = "eventCollectorWebServices";
    boost::shared_ptr<boost::any> a = _session->getData(kDataIdentifier);
    boost::shared_ptr<EventCollector> result;

    if((a == NULL) || (a->empty())) {
      boost::shared_ptr<boost::any> b(new boost::any());
      EventInterpreterInternalRelay* pPlugin =
          dynamic_cast<EventInterpreterInternalRelay*>(
              DSS::getInstance()->getEventInterpreter().getPluginByName(
                std::string(EventInterpreterInternalRelay::getPluginName())
              )
          );
      if(pPlugin == NULL) {
        throw std::runtime_error("Need EventInterpreterInternalRelay to be registered");
      }
      result.reset(new EventCollector(*pPlugin));
      *b = result;
      _session->addData(std::string("eventSubscriptionIDs"), b);
    } else {
      try {
        result = boost::any_cast<boost::shared_ptr<EventCollector> >(*a);
      } catch (boost::bad_any_cast& e) {
        Logger::getInstance()->log("Fatal error: unexpected data type stored in session!", lsFatal);
        assert(0);
      }
    }
    return result;
  }

  bool WebServices::waitForEvent(Session* _session, const int _timeoutMS, soap* _soapRequest) {
    const int kSocketDisconnectTimeoutMS = 200;
    bool timedOut = false;
    bool result = false;
    int timeoutMSLeft = _timeoutMS;
    _session->use();
    boost::shared_ptr<EventCollector> subscriptionSession = extractFromSession(_session);
    while(!timedOut && !result) {
      // check if we're still connected
      uint8_t tmp;
      int res = recv(_soapRequest->socket, &tmp, 1,  MSG_PEEK | MSG_DONTWAIT);
      if(res == -1) {
        if((errno != EAGAIN) && (errno != EINTR) && (errno != EWOULDBLOCK)) {
          Logger::getInstance()->log("WebServices::waitForEvent: lost connection", lsInfo);
          break;
        }
      } else if(res == 0) {
        // if we were still connected, recv would return -1 with an errno listed above or 1
        Logger::getInstance()->log("WebServices::waitForEvent: lost connection", lsInfo);
        break;
      }
      // calculate the length of our wait
      int waitTime;
      if(_timeoutMS == -1) {
        timedOut = true;
        waitTime = 0;
      } else if(_timeoutMS != 0) {
        waitTime = std::min(timeoutMSLeft, kSocketDisconnectTimeoutMS);
        timeoutMSLeft -= waitTime;
        timedOut = (timeoutMSLeft == 0);
      } else {
        waitTime = kSocketDisconnectTimeoutMS;
        timedOut = false;
      }
      // wait for the event
      result = subscriptionSession->waitForEvent(waitTime);
    }
    _session->unuse();
    return result;
  } // waitForEvent

  Event WebServices::popEvent(Session* _session) {
    boost::shared_ptr<EventCollector> subscriptionSession = extractFromSession(_session);
    return subscriptionSession->popEvent();
  } // popEvent

  std::string WebServices::subscribeTo(Session* _session, const std::string& _eventName) {
    boost::shared_ptr<EventCollector> subscriptionSession = extractFromSession(_session);
    return subscriptionSession->subscribeTo(_eventName);
  } // subscribeTo

  bool WebServices::hasEvent(Session* _session) {
    boost::shared_ptr<EventCollector> subscriptionSession = extractFromSession(_session);
    return subscriptionSession->hasEvent();
  } // hasEvent


  //================================================== WebServicesWorker

  WebServicesWorker::WebServicesWorker(WebServices* _services)
  : Thread("WebServicesWorker"),
    m_pServices(_services)
  {
    assert(m_pServices != NULL);
  } // ctor

  void WebServicesWorker::execute() {
    while(!m_Terminated) {
      struct soap* req = m_pServices->popPendingRequest();
      if(req != NULL) {
        try {
          soap_serve(req);
        } catch(std::runtime_error& err) {
          Logger::getInstance()->log(std::string("WebserviceWorker::Execute: Caught exception:")
                                     + err.what());
        }
        soap_destroy(req);
        soap_end(req);
        soap_done(req);
        free(req);
      }
    }
  } // execute

} // namespace dss

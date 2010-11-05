/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

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

#include <cassert>
#include <cstdlib>

#include <boost/foreach.hpp>

#include "core/dss.h"

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

    for (size_t i = 0; i < m_Workers.size(); i++) {
      m_Workers.at(i).terminate();
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

  WebServiceSession& WebServices::newSession(soap* _soapRequest, int& token) {
    token = ++m_LastSessionID;
    m_SessionByID[token] = WebServiceSession(token, _soapRequest);
    return m_SessionByID[token];
  } // newSession

  void WebServices::deleteSession(soap* _soapRequest, const int _token) {
    WebServiceSessionByID::iterator iEntry = m_SessionByID.find(_token);
    if(iEntry != m_SessionByID.end()) {
      if(iEntry->second->isOwner(_soapRequest)) {
        m_SessionByID.erase(iEntry);
      }
    }
  } // deleteSession

  bool WebServices::isAuthorized(soap* _soapRequest, const int _token) {
    WebServiceSessionByID::iterator iEntry = m_SessionByID.find(_token);
    if(iEntry != m_SessionByID.end()) {
      if(iEntry->second->isOwner(_soapRequest)) {
        if(iEntry->second->isStillValid()) {
          return true;
        }
      }
    }
    return false;
  } // isAuthorized

  WebServiceSession& WebServices::getSession(soap* _soapRequest, const int _token) {
    return m_SessionByID[_token];
  }

  //================================================== WebServiceSession

  WebServiceSession::WebServiceSession(const int _tokenID, soap* _soapRequest)
  : Session(_tokenID),
    m_OriginatorIP(_soapRequest->ip)
  { } // ctor

  WebServiceSession::~WebServiceSession() {
  } // dtor

  bool WebServiceSession::isOwner(soap* _soapRequest) {
    return _soapRequest->ip == m_OriginatorIP;
  } // isOwner

  WebServiceSession& WebServiceSession::operator=(const WebServiceSession& _other) {
    Session::operator=(_other);
    m_OriginatorIP = _other.m_OriginatorIP;

    return *this;
  } // operator=

  void WebServiceSession::createCollector() {
    if(m_pEventListener == NULL) {
      EventInterpreterInternalRelay* pPlugin =
          dynamic_cast<EventInterpreterInternalRelay*>(
              DSS::getInstance()->getEventInterpreter().getPluginByName(
                std::string(EventInterpreterInternalRelay::getPluginName())
              )
          );
      if(pPlugin == NULL) {
        throw std::runtime_error("Need EventInterpreterInternalRelay to be registered");
      }
      m_pEventListener.reset(new EventCollector(*pPlugin));
    }
    assert(m_pEventListener != NULL);
  } // createCollector

  bool WebServiceSession::waitForEvent(const int _timeoutMS, soap* _soapRequest) {
    createCollector();
    const int kSocketDisconnectTimeoutMS = 200;
    bool timedOut = false;
    bool result = false;
    int timeoutMSLeft = _timeoutMS;
    use();
    while(!timedOut && !result) {
      // check if we're still connected
      uint8_t tmp;
      int res = recv(_soapRequest->socket, &tmp, 1,  MSG_PEEK | MSG_DONTWAIT);
      if(res == -1) {
        if((errno != EAGAIN) && (errno != EINTR) && (errno != EWOULDBLOCK)) {
          Logger::getInstance()->log("WebServiceSession::waitForEvent: lost connection", lsInfo);
          break;
        }
      } else if(res == 0) {
        // if we were still connected, recv would return -1 with an errno listed above or 1
        Logger::getInstance()->log("WebServiceSession::waitForEvent: lost connection", lsInfo);
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
      result = m_pEventListener->waitForEvent(waitTime);
    }
    unuse();
    return result;
  } // waitForEvent

  Event WebServiceSession::popEvent() {
    createCollector();
    return m_pEventListener->popEvent();
  } // popEvent

  std::string WebServiceSession::subscribeTo(const std::string& _eventName) {
    return m_pEventListener->subscribeTo(_eventName);
  } // subscribeTo

  bool WebServiceSession::hasEvent() {
    createCollector();
    return m_pEventListener->hasEvent();
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

#include "webservices.h"

#include <cassert>
#include <cstdlib>

#include <boost/foreach.hpp>

namespace dss {
  //================================================== WebServices

  WebServices::WebServices(DSS* _pDSS)
  : Subsystem(_pDSS, "WebServices"),
    Thread("WebServices")
  {

  } // ctor

  WebServices::~WebServices() {

  } // dtor

  void WebServices::doStart() {
    run();
  } // doStart

  void WebServices::execute() {
    m_Service.bind(NULL, 8081, 10);
    for(int iWorker = 0; iWorker < 4; iWorker++) {
      m_Workers.push_back(new WebServicesWorker(this));
      m_Workers.back().run();
    }
    while(!m_Terminated) {
      int socket = m_Service.accept();
      if(socket != SOAP_INVALID_SOCKET) {
        struct soap* req_copy = soap_copy(&m_Service);
        m_RequestsMutex.lock();
        m_PendingRequests.push_back(req_copy);
        m_RequestsMutex.unlock();
        m_RequestArrived.signal();
//        m_Service.serve();
      }
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
  {
  } // ctor

  bool WebServiceSession::isOwner(soap* _soapRequest) {
    return _soapRequest->ip == m_OriginatorIP;
  } // isOwner

  WebServiceSession& WebServiceSession::operator=(const WebServiceSession& _other) {
    Session::operator=(_other);
    m_OriginatorIP = _other.m_OriginatorIP;

    return *this;
  } // operator=


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
        soap_serve(req);
        soap_destroy(req);
        soap_end(req);
        soap_done(req);
        free(req);
      }
    }
  } // execute

} // namespace dss

#include "webservices.h"

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
  }

  void WebServices::execute() {
    m_Service.bind(NULL, 8081, 10);
    while(!m_Terminated) {
      int socket = m_Service.accept();
      if(socket != SOAP_INVALID_SOCKET) {
        m_Service.serve();
      }
    }
  } // execute

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
  }

  WebServiceSession& WebServiceSession::operator=(const WebServiceSession& _other) {
    Session::operator=(_other);
    m_OriginatorIP = _other.m_OriginatorIP;

    return *this;
  }

}

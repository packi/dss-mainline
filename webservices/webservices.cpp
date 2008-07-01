#include "webservices.h"

#include <boost/foreach.hpp>

namespace dss {
  //================================================== WebServices
  
  WebServices::WebServices()
  : Thread(true, "WebServices")
  {
    
  } // ctor
  
  WebServices::~WebServices() {
    
  } // dtor
  
  void WebServices::Execute() {
    m_Service.bind(NULL, 8081, 10);
    while(!m_Terminated) {
      int socket = m_Service.accept();
      if(socket != SOAP_INVALID_SOCKET) {
        m_Service.serve();
      }
    }
  } // Execute

  WebServiceSession& WebServices::NewSession(soap* _soapRequest, int& token) {
    token = ++m_LastSessionID;
    m_SessionByID[token] = WebServiceSession(token, _soapRequest);
    return m_SessionByID[token];
  } // NewSession
  
  void WebServices::DeleteSession(soap* _soapRequest, const int _token) {
    SessionByID::iterator iEntry = m_SessionByID.find(_token);
    if(iEntry != m_SessionByID.end()) {
      if(iEntry->second->IsOwner(_soapRequest)) {
        m_SessionByID.erase(iEntry);
      }
    }
  } // DeleteSession
  
  bool WebServices::IsAuthorized(soap* _soapRequest, const int _token) {
    SessionByID::iterator iEntry = m_SessionByID.find(_token);
    if(iEntry != m_SessionByID.end()) {
      if(iEntry->second->IsOwner(_soapRequest)) {
        if(iEntry->second->IsStillValid()) {
          return true;
        }
      }
    }
    return false;
  } // IsAuthorized
  
  WebServiceSession& WebServices::GetSession(soap* _soapRequest, const int _token) {
    return m_SessionByID[_token];
  }
  
  
  //================================================== WebServiceSession
  
  WebServiceSession::WebServiceSession(const int _tokenID, soap* _soapRequest) 
  : m_Token(_tokenID),
    m_OriginatorIP(_soapRequest->ip)
  {
    m_LastTouched = DateTime();
  } // ctor
  
  bool WebServiceSession::IsStillValid() {
    const int TheSessionTimeout = 5;
    return m_LastTouched.AddMinute(TheSessionTimeout).After(DateTime());
  } // IsStillValid
  
  bool WebServiceSession::IsOwner(soap* _soapRequest) {
    return _soapRequest->ip == m_OriginatorIP;
  }
  
  bool WebServiceSession::HasSetWithID(const int _id) {
    SetsByID::iterator iKey = m_SetsByID.find(_id);
    return iKey != m_SetsByID.end();
  } // HasSetWithID
  
  Set& WebServiceSession::GetSetByID(const int _id) {
    SetsByID::iterator iEntry = m_SetsByID.find(_id);
    return iEntry->second;    
  } // GetSetByID
  
  Set& WebServiceSession::AllocateSet(int& id) {
    id = ++m_LastSetNr;
    m_SetsByID[id] = Set();
    return m_SetsByID[id];
  } // AllocateSet

  Set& WebServiceSession::AddSet(Set _set, int& id) {
    id = ++m_LastSetNr;
    m_SetsByID[id] = _set;
    return m_SetsByID[id];
  } // AddSet

  void WebServiceSession::FreeSet(const int _id) {
    m_SetsByID.erase(m_SetsByID.find(_id));
  } // FreeSet
  
  WebServiceSession& WebServiceSession::operator=(const WebServiceSession& _other) {
    m_LastSetNr = _other.m_LastSetNr;
    m_Token = _other.m_Token;
    m_OriginatorIP = _other.m_OriginatorIP;
    m_LastTouched = _other.m_LastTouched;
    
    for(SetsByID::const_iterator iEntry = m_SetsByID.begin(), e = m_SetsByID.end();
        iEntry != e; ++iEntry)
    {
      m_SetsByID[iEntry->first] = iEntry->second;
    }
    return *this;
  }
  
}

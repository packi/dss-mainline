#include "webservices.h"

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
    return iKey != m_SetsByID.end());
  } // HasSetWithID
  
  Set& WebServiceSession::GetSetByID(const int _id) {
    return m_SetsByID.lookup(_id);    
  } // GetSetByID
  
  Set& WebServiceSession::AllocateSet(int& _id) {
    _id = ++m_LastSetNr;
    m_SetsByID[_id] = Set();
    return m_SetsByID[_id];
  } // AllocateSet
  
  void WebServiceSession::FreeSet(const int _id) {
    m_SetsByID.erase(m_SetsByID.find(_id));
  } // FreeSet

}

#ifndef WEBSERVICES_H_
#define WEBSERVICES_H_

#include "../core/model.h"
#include "../webservices/soapdssObject.h"
#include "../core/thread.h"
#include "../core/datetools.h"

#include <boost/ptr_container/ptr_hashmap>

namespace dss {
  
  typedef boost::ptr_map<const int, Set> SetsByID;
  
  class WebServiceSession {
  private:
    int32_t m_OriginatorIP;
    int m_Token;

    int m_LastSetNr;
    DateTime m_LastTouched;
    SetsByID m_SetsByID;
  public:
    WebServiceSession(const int _tokenID, soap* _soapRequest);
    
    bool IsStillValid();
    bool IsOwner(soap* _soapRequest);
    bool HasSetWithID(const int _id);
    Set& GetSetByID(const int _id);
    Set& AllocateSet(int& _id);
    void FreeSet(const int _id);
  };
  
  class WebServices : public Thread {
  private:
    dssService m_Service;
    
  public:
    WebServices();
    virtual ~WebServices();
    
    virtual void Execute();
  };
  
}

#endif /*WEBSERVICES_H_*/

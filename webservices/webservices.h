#ifndef WEBSERVICES_H_
#define WEBSERVICES_H_

#include "core/model.h"
#include "webservices/soapdssObject.h"
#include "core/thread.h"
#include "core/datetools.h"
#include "core/subsystem.h"
#include "core/session.h"

#include <boost/ptr_container/ptr_map.hpp>

#include <map>

namespace dss {

  class WebServiceSession : public Session {
  private:
    int m_Token;
    uint32_t m_OriginatorIP;

    int m_LastSetNr;
    DateTime m_LastTouched;
    SetsByID m_SetsByID;
  public:
    WebServiceSession(const int _tokenID, soap* _soapRequest);

    bool IsOwner(soap* _soapRequest);

    WebServiceSession& operator=(const WebServiceSession& _other);
  };

  typedef boost::ptr_map<const int, WebServiceSession> SessionByID;

  class WebServices : public Subsystem,
                      private Thread {
  private:
    dssService m_Service;
    int m_LastSessionID;
    SessionByID m_SessionByID;
  protected:
    virtual void DoStart();
  public:
    WebServices(DSS* _pDSS);
    virtual ~WebServices();


    WebServiceSession& NewSession(soap* _soapRequest, int& token);
    void DeleteSession(soap* _soapRequest, const int _token);
    WebServiceSession& GetSession(soap* _soapRequest, const int _token);

    bool IsAuthorized(soap* _soapRequest, const int _token);

    virtual void Execute();
  };

}

#endif /*WEBSERVICES_H_*/

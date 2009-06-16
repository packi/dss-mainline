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
  protected:
    uint32_t m_OriginatorIP;
  public:
    WebServiceSession() {}
    WebServiceSession(const int _tokenID, soap* _soapRequest);

    bool isOwner(soap* _soapRequest);

    WebServiceSession& operator=(const WebServiceSession& _other);
  };

  typedef boost::ptr_map<const int, WebServiceSession> WebServiceSessionByID;

  class WebServices : public Subsystem,
                      private Thread {
  private:
    dssService m_Service;
    int m_LastSessionID;
    WebServiceSessionByID m_SessionByID;
  protected:
    virtual void doStart();
  public:
    WebServices(DSS* _pDSS);
    virtual ~WebServices();


    WebServiceSession& newSession(soap* _soapRequest, int& token);
    void deleteSession(soap* _soapRequest, const int _token);
    WebServiceSession& getSession(soap* _soapRequest, const int _token);

    bool isAuthorized(soap* _soapRequest, const int _token);

    virtual void execute();
  };

}

#endif /*WEBSERVICES_H_*/

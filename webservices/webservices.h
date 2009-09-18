#ifndef WEBSERVICES_H_
#define WEBSERVICES_H_

#include "core/model.h"
#include "webservices/soapdssObject.h"
#include "core/thread.h"
#include "core/datetools.h"
#include "core/subsystem.h"
#include "core/session.h"
#include "core/mutex.h"
#include "core/syncevent.h"

#include <deque>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class WebServicesWorker;

  class WebServiceSession : public Session {
  protected:
    uint32_t m_OriginatorIP;
  public:
    WebServiceSession() {}
    WebServiceSession(const int _tokenID, soap* _soapRequest);

    bool isOwner(soap* _soapRequest);

    WebServiceSession& operator=(const WebServiceSession& _other);
  }; // WebServiceSession

  typedef boost::ptr_map<const int, WebServiceSession> WebServiceSessionByID;

  class WebServices : public Subsystem,
                      private Thread {
  private:
    dssService m_Service;
    int m_LastSessionID;
    WebServiceSessionByID m_SessionByID;
    boost::ptr_vector<WebServicesWorker> m_Workers;
    std::deque<struct soap*> m_PendingRequests;
    Mutex m_RequestsMutex;
    SyncEvent m_RequestArrived;
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

    struct soap* popPendingRequest();
  }; // WebServices

  class WebServicesWorker : public Thread {
  public:
    WebServicesWorker(WebServices* _services);
    virtual void execute();
  private:
    WebServices* m_pServices;
  }; // WebServicesWorker

}

#endif /*WEBSERVICES_H_*/

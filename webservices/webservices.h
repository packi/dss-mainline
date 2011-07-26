/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#ifndef WEBSERVICES_H_
#define WEBSERVICES_H_

#include "webservices/soapdssSSLObject.h"
#include "core/thread.h"
#include "core/datetools.h"
#include "core/subsystem.h"
#include "core/mutex.h"
#include "core/syncevent.h"
#include "core/event.h"

#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class WebServicesWorker;
  class SessionManager;
  class Session;

  class WebServices : public ThreadedSubsystem {
  private:
    dssSSLService m_Service;
    boost::ptr_vector<WebServicesWorker> m_Workers;
    std::deque<struct soap*> m_PendingRequests;
    Mutex m_RequestsMutex;
    SyncEvent m_RequestArrived;
    boost::shared_ptr<SessionManager> m_pSessionManager;
    bool soapConnectionAlive(soap* _soapRequest);
  protected:
    virtual void doStart();
  public:
    WebServices(DSS* _pDSS);
    virtual ~WebServices();

    void deleteSession(soap* _soapRequest, const std::string& _token);
    boost::shared_ptr<Session> getSession(soap* _soapRequest, const std::string& _token);

    bool isAuthorized(soap* _soapRequest, const std::string& _token);

    virtual void initialize();
    virtual void execute();

    void setSessionManager(boost::shared_ptr<SessionManager> _value) {
      m_pSessionManager = _value;
    }

    boost::shared_ptr<SessionManager> getSessionManager() {
      return m_pSessionManager;
    }

    struct soap* popPendingRequest();

    bool waitForEvent(Session* _session, const int _timeoutMS, soap* _soapRequest);
    Event popEvent(Session* _session);
    bool hasEvent(Session* _session);

    std::string subscribeTo(Session* _session, const std::string& _eventName);
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

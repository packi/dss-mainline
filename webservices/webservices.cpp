#include "webservices.h"

#include <cassert>
#include <cstdlib>

#include <boost/foreach.hpp>

#include "core/dss.h"

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


  //================================================== WebServiceEventListener

  class WebServiceEventListener : public EventRelayTarget {
  public:
    WebServiceEventListener(EventInterpreterInternalRelay& _relay)
    : EventRelayTarget(_relay)
    { } // ctor
    ~WebServiceEventListener() {
      while(!m_Subscriptions.empty()) {
        DSS::getInstance()->getEventInterpreter().unsubscribe(m_Subscriptions.front()->getID());
        m_Subscriptions.erase(m_Subscriptions.begin());
      }
    } // dtor

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
    bool waitForEvent(const int _timeoutMS);
    Event popEvent();
    bool hasEvent() { return !m_PendingEvents.empty(); }

    virtual std::string subscribeTo(const std::string& _eventName);
  private:
    SyncEvent m_EventArrived;
    Mutex m_PendingEventsMutex;
    std::vector<Event> m_PendingEvents;
    std::vector<boost::shared_ptr<EventSubscription> > m_Subscriptions;
  }; // WebServiceEventListener

  void WebServiceEventListener::handleEvent(Event& _event, const EventSubscription& _subscription) {
    m_PendingEventsMutex.lock();
    m_PendingEvents.push_back(_event);
    m_PendingEventsMutex.unlock();
    m_EventArrived.signal();
  } // handleEvent

  bool WebServiceEventListener::waitForEvent(const int _timeoutMS) {
    if(m_PendingEvents.empty()) {
      if(_timeoutMS == 0) {
        m_EventArrived.waitFor();
      } else if(_timeoutMS > 0) {
        m_EventArrived.waitFor(_timeoutMS);
      }
    }
    return !m_PendingEvents.empty();
  } // waitForEvent

  Event WebServiceEventListener::popEvent() {
    Event result;
    m_PendingEventsMutex.lock();
    result = m_PendingEvents.front();
    m_PendingEvents.erase(m_PendingEvents.begin());
    m_PendingEventsMutex.unlock();
    return result;
  } // popEvent

  std::string WebServiceEventListener::subscribeTo(const std::string& _eventName) {
    boost::shared_ptr<EventSubscription> subscription(
      new dss::EventSubscription(
            _eventName,
            EventInterpreterInternalRelay::getPluginName(),
            DSS::getInstance()->getEventInterpreter(),
            boost::shared_ptr<dss::SubscriptionOptions>()
      )
    );

    EventRelayTarget::subscribeTo(subscription);
    m_Subscriptions.push_back(subscription);
    DSS::getInstance()->getEventInterpreter().subscribe(subscription);

    return subscription->getID();
  } // subscribeTo


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

  void WebServiceSession::createListener() {
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
      m_pEventListener.reset(new WebServiceEventListener(*pPlugin));
    }
    assert(m_pEventListener != NULL);
  } // createListener

  bool WebServiceSession::waitForEvent(const int _timeoutMS) {
    createListener();
    return m_pEventListener->waitForEvent(_timeoutMS);
  } // waitForEvent

  Event WebServiceSession::popEvent() {
    createListener();
    return m_pEventListener->popEvent();
  } // popEvent

  std::string WebServiceSession::subscribeTo(const std::string& _eventName) {
    createListener();
    return m_pEventListener->subscribeTo(_eventName);
  } // subscribeTo

  bool WebServiceSession::hasEvent() {
    createListener();
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
          Logger::getInstance()->log(string("WebserviceWorker::Execute: Caught exception:")
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

#include "heatingregistering.h"
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
#include "foreach.h"

namespace {
  static const int REGISTERING_TIMEOUT_MINUTE = 5;
  static const std::string EV_RETRY_REGISTRATION("retry_registration");
}

namespace dss {

/*----------------------------------------------------------------------------*/
RegisteringLog::RegisteringLog(HeatingRegisteringItf* pItf) :
  m_registration(false),
  m_HeatingRegistering(pItf)
{ } //ctor

RegisteringLog::~RegisteringLog()
{ } //dtor

void RegisteringLog::done(RestTransferStatus_t status, WebserviceReply reply)
{
  if ((REST_OK == status) && (0 == reply.code )) {
    boost::mutex::scoped_lock lock(m_lock);
    m_registration = true;

    if (m_HeatingRegistering) {
      m_HeatingRegistering->stopTimeout();
    }
  }
}

bool RegisteringLog::isRegistered() const
{
  boost::mutex::scoped_lock lock(m_lock);
  return m_registration;
}

void RegisteringLog::resetRegistration()
{
  boost::mutex::scoped_lock lock(m_lock);
  m_registration = false;
}

__DEFINE_LOG_CHANNEL__(HeatingRegisteringPlugin, lsInfo);

/*----------------------------------------------------------------------------*/
HeatingRegisteringPlugin::HeatingRegisteringPlugin(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("heating_registering_upload", _pInterpreter),
    m_callback(boost::make_shared<RegisteringLog>(this))
{
}  //ctor

HeatingRegisteringPlugin::~HeatingRegisteringPlugin()
{ } // dtor

void HeatingRegisteringPlugin::startTimeout() {
  log(std::string(__func__) + " start cloud response timeout", lsInfo);

  DateTime start;
  start = start.addMinute(REGISTERING_TIMEOUT_MINUTE);

  boost::shared_ptr<Event> pEvent(new Event(EV_RETRY_REGISTRATION));
  pEvent->setProperty(EventProperty::ICalStartTime, start.toRFC2445IcalDataTime());
  pEvent->setProperty(EventProperty::ICalRRule, "FREQ=MINUTELY");
  DSS::getInstance()->getEventQueue().pushEvent(pEvent);
}

void HeatingRegisteringPlugin::stopTimeout()
{
  DSS::getInstance()->getEventRunner().removeEventByName(EV_RETRY_REGISTRATION);
}

void HeatingRegisteringPlugin::subscribe() {
  boost::shared_ptr<EventSubscription> subscription;

  subscription.reset(new EventSubscription("model_ready",
                                           getName(),
                                           getEventInterpreter(),
                                           boost::shared_ptr<SubscriptionOptions>()));
  getEventInterpreter().subscribe(subscription);

  subscription.reset(new EventSubscription(EV_RETRY_REGISTRATION,
                                           getName(),
                                           getEventInterpreter(),
                                           boost::shared_ptr<SubscriptionOptions>()));
  getEventInterpreter().subscribe(subscription);
}

void HeatingRegisteringPlugin::handleEvent(Event& _event,
                                           const EventSubscription& _subscription) {
  if ( (_event.getName() == "model_ready") ||
       (_event.getName() == EV_RETRY_REGISTRATION)) {
    sendRegisterMessage();
  }
}

void HeatingRegisteringPlugin::sendRegisterMessage() {
  if (m_callback->isRegistered()) {
    stopTimeout();
  } else {
    bool webServiceEnabled = DSS::getInstance()->getPropertySystem().getBoolValue(pp_websvc_enabled);
    if (webServiceEnabled) {
      std::string parameters;
      // AppToken is piggy backed with websvc_connection::request(.., authenticated=true)
      parameters += "&dssid=" + DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid)->getStringValue();
      boost::shared_ptr<StatusReplyChecker> mcb(new StatusReplyChecker(m_callback));
      WebserviceConnection::getInstance()->request("internal/dss/v1_0/DSSApartment/DSSBackAgain", parameters, POST, mcb, true);
      m_callback->resetRegistration();
      startTimeout();
    }
  }
}

}

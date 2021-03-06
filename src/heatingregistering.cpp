#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "heatingregistering.h"
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
#include "foreach.h"

namespace {
  static const std::string EV_RETRY_REGISTRATION("retry_registration");
}

namespace dss {

/*----------------------------------------------------------------------------*/
RegisteringLog::RegisteringLog(HeatingRegisteringItf* pItf) :
  m_HeatingRegistering(pItf)
{ } //ctor

RegisteringLog::~RegisteringLog()
{ } //dtor

void RegisteringLog::done(RestTransferStatus_t status, WebserviceReply reply)
{
  if (!((REST_OK == status) &&
        ((MsHub::OK == reply.code) || (MsHub::RC_NOT_ENABLED == reply.code)))) {
    m_HeatingRegistering->startTimeout(500);
  }
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

void HeatingRegisteringPlugin::startTimeout(int _delay) {
  log(std::string(__func__) + " start cloud registration delay", lsInfo);
  boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EV_RETRY_REGISTRATION);
  pEvent->setProperty("time", "+" + intToString(_delay));
  pEvent->setProperty(EventProperty::Unique, "Yes");
  DSS::getInstance()->getEventQueue().pushEvent(pEvent);
}

void HeatingRegisteringPlugin::subscribe() {
  auto&& interpreter = getEventInterpreter();
  interpreter.subscribe(boost::make_shared<EventSubscription>(*this, EventName::DSMeterReady));
  interpreter.subscribe(boost::make_shared<EventSubscription>(*this, EV_RETRY_REGISTRATION));
}

void HeatingRegisteringPlugin::handleEvent(Event& _event,
                                           const EventSubscription& _subscription) {
  if (_event.getName() == EventName::DSMeterReady) {
    startTimeout(120);
  } else if (_event.getName() == EV_RETRY_REGISTRATION) {
    sendRegisterMessage();
  }
}

void HeatingRegisteringPlugin::sendRegisterMessage() {
  WebserviceMsHub::doDssBackAgain(m_callback);
}

}

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
  if (!((REST_OK == status) && (0 == reply.code))) {
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
  boost::shared_ptr<Event> pEvent(new Event(EV_RETRY_REGISTRATION));
  pEvent->setProperty("time", "+" + intToString(_delay));
  pEvent->setProperty(EventProperty::Unique, "Yes");
  DSS::getInstance()->getEventQueue().pushEvent(pEvent);
}

void HeatingRegisteringPlugin::subscribe() {
  boost::shared_ptr<EventSubscription> subscription;

  subscription.reset(new EventSubscription("dsMeter_ready",
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
  if (_event.getName() == "dsMeter_ready") {
    startTimeout(120);
  } else if (_event.getName() == EV_RETRY_REGISTRATION) {
    sendRegisterMessage();
  }
}

void HeatingRegisteringPlugin::sendRegisterMessage() {
  WebserviceMsHub::doDssBackAgain(m_callback);
}

}

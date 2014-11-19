#ifndef __HEATINGREGISTERING_H__
#define __HEATINGREGISTERING_H__

#include "event.h"
#include "logger.h"

#include "src/webservice_api.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace dss {

  // interface class for callback
  class HeatingRegisteringItf {
  public:
    virtual ~HeatingRegisteringItf(){};
    virtual void startTimeout(int _delay) = 0;
  };

  class RegisteringLog : public WebserviceCallDone,
                         public boost::enable_shared_from_this<RegisteringLog> {
  public:
    RegisteringLog(HeatingRegisteringItf* pItf);
    virtual ~RegisteringLog();
    virtual void done(RestTransferStatus_t status, WebserviceReply reply);
  private:
    HeatingRegisteringItf *m_HeatingRegistering;
  };

  class HeatingRegisteringPlugin : public EventInterpreterPlugin,
                                   public HeatingRegisteringItf {
  private:
    __DECL_LOG_CHANNEL__
  public:
    HeatingRegisteringPlugin(EventInterpreter* _pInterpreter);
    virtual ~HeatingRegisteringPlugin();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
    virtual void subscribe();

  private:
    void startTimeout(int _delay);

    void sendRegisterMessage();
    boost::shared_ptr<RegisteringLog> m_callback;
  };
}
#endif // __HEATINGREGISTERING_H__

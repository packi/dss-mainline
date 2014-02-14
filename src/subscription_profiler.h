#ifndef _TIMER_TOOLS_H_
#define _TIMER_TOOLS_H_

#include "event.h"
#include "logger.h"

namespace dss {

  class BenchmarkPublisherPlugin : public EventInterpreterPlugin {
    __DECL_LOG_CHANNEL__
  public:
    BenchmarkPublisherPlugin(EventInterpreter* _pInterpreter);
    virtual ~BenchmarkPublisherPlugin();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };
}

#endif

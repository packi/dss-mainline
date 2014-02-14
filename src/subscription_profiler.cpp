#include "subscription_profiler.h"

#include <time.h>
#include <assert.h>
#include <libical/ical.h>

#include "dss.h"

namespace dss {

  //================================================== BenchmarkPublisherPlugin
  __DEFINE_LOG_CHANNEL__(BenchmarkPublisherPlugin, lsInfo)

  BenchmarkPublisherPlugin::BenchmarkPublisherPlugin(EventInterpreter* _pInterpreter)
    : EventInterpreterPlugin("subscription_benchmark_uploader", _pInterpreter) {
    log(std::string(__func__), lsDebug);
  }

  BenchmarkPublisherPlugin::~BenchmarkPublisherPlugin() {
  }

  void BenchmarkPublisherPlugin::handleEvent(Event& _event, const
                                             EventSubscription& _subscription) {
    if (_event.getName() == "running") {
      boost::shared_ptr<Event> pEvent;
      DateTime now;

      log(std::string(__func__) + " install ical rules", lsInfo);

      /*
       * -- credits or complaints: rk
       * http://www.kanzaki.com/docs/ical/rrule.html
       */
      pEvent = boost::shared_ptr<Event>(new Event("reexport_timings"));
      pEvent->setProperty(EventProperty::ICalStartTime,
                          now.addMinute(2).toRFC2445IcalDataTime());
      pEvent->setProperty(EventProperty::ICalRRule, "FREQ=MINUTELY;COUNT=27");
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);

      pEvent = boost::shared_ptr<Event>(new Event("reexport_timings"));
      pEvent->setProperty(EventProperty::ICalStartTime,
                          now.addMinute(30).toRFC2445IcalDataTime());
      pEvent->setProperty(EventProperty::ICalRRule, "FREQ=MINUTELY;INTERVAL=10;COUNT=8");
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);

      pEvent = boost::shared_ptr<Event>(new Event("reexport_timings"));
      pEvent->setProperty(EventProperty::ICalStartTime,
                          now.addHour(2).toRFC2445IcalDataTime());
      pEvent->setProperty(EventProperty::ICalRRule, "FREQ=HOURLY");
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);

      return;
    }

    static int ct = 0;

    ct++;
    log(std::string(__func__) + " " + _event.getName() + " " + intToString(ct), lsDebug);
  }


} // namespace

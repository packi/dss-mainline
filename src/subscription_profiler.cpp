#include "subscription_profiler.h"

#include <assert.h>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <libical/ical.h>
#include <time.h>

#include "dss.h"
#include "foreach.h"
#include "base.h"
#include "datetools.h"

#include "dss.h"

namespace dss {

  /*
   * TODO cleanup this mess
   * - move macros into operator functions
   * - use Templates: typedef TimeStampImpl<timerspec> TimeStamp
   *   so we can switch between __APPLE__ and/or float
   */

  /*
   * taken from sys/time.h addapted for struct timespec
   */
  /* Convenience macros for operations on timevals.
   * NOTE: `timercmp' does not work for >= or <=.  */
# define timespec_cmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_nsec CMP (b)->tv_nsec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))
# define timespec_add(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;			      \
    if ((result)->tv_nsec >= 1000000000)					      \
    {									      \
      ++(result)->tv_sec;						      \
      (result)->tv_nsec -= 1000000000;					      \
    }									      \
  } while (0)
# define timespec_sub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;			      \
    if ((result)->tv_nsec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_nsec += 1000000000;					      \
    }									      \
  } while (0)

#define SEC_TO_NSEC(s) ((s) * 1000 * 1000 * 1000)
# define timespec_2_ns(a) \
  (SEC_TO_NSEC(a.tv_sec) + a.tv_nsec) \

#define SEC_TO_MSEC(s) ((s) * 1000 * 1000)
# define timespec_2_us(a) \
  (SEC_TO_MSEC(a.tv_sec) + a.tv_nsec / 1000)

  unsigned TimeStamp::toMicroSec() const {
    return timespec_2_us(m_stamp);
  }

  TimeStamp TimeStamp::operator+(const TimeStamp &o) const {
    TimeStamp tmp;
    timespec_add(&m_stamp, &o.m_stamp, &tmp.m_stamp);
    return tmp;
  }

  TimeStamp TimeStamp::operator-(const TimeStamp &o) const {
    TimeStamp tmp;
    timespec_sub(&m_stamp, &o.m_stamp, &tmp.m_stamp);
    return tmp;
  }

  TimeStamp TimeStamp::operator+=(const TimeStamp &o) {
    timespec_add(&m_stamp, &o.m_stamp, &m_stamp);
    return *this;
  }

  TimeStamp TimeStamp::operator-=(const TimeStamp &o) {
    timespec_sub(&m_stamp, &o.m_stamp, &m_stamp);
    return *this;
  }

  bool TimeStamp::operator<(const TimeStamp &o) const {
    return timespec_cmp(&m_stamp, &o.m_stamp, <);
  }

  //================================================== SubscriptionTime

  bool SubscriptionTime::operator==(const SubscriptionTime &other) const {
    return ((m_subscriptionName == other.m_subscriptionName) &&
            (m_eventName == other.m_eventName));
  }

  bool SubscriptionTime::operator<(const SubscriptionTime &other) const {
    return (total() < other.total());
  }

  std::ostream& SubscriptionTime::operator<<(std::ostream &strm) const {
    return strm << m_subscriptionName + "@" + m_eventName;
  }

  std::string SubscriptionTime::toString() const {
    return m_subscriptionName + "@" + m_eventName;
  }

  TimeStamp SubscriptionTime::total() const {
    TimeStamp total = m_init + m_end;
    foreach(ScriptTime script, scripts) {
      total += script.totalTime;
    }
    return total;
  }

  //================================================== StopWatch

  __DEFINE_LOG_CHANNEL__(StopWatch, lsInfo)

  StopWatch::StopWatch(const std::string &eventName) : m_cancelled(false) {
    m_thisRun.m_eventName = eventName;
  }

  void StopWatch::startSubscription() {
    m_tic.timestamp();
  }

  void StopWatch::setSubscriptionName(const std::string &subscription) {
    m_thisRun.m_subscriptionName = subscription;
  }

  TimeStamp StopWatch::calcInterval() {
    struct TimeStamp prev;
    prev = m_tic;
    m_tic.timestamp();
    return (m_tic - prev);
  }

  void StopWatch::startScript() {
    m_thisRun.m_init = calcInterval();
    m_script = ScriptTime();
  }

  void StopWatch::setScriptName(const std::string &name) {
    m_script.name = name;
  }

  void StopWatch::stopScript() {
    m_script.totalTime = calcInterval();
    m_thisRun.scripts.push_back(m_script);
  }

  void StopWatch::stopSubscription() {
    if (m_cancelled) {
      return;
    }
    m_thisRun.m_end = calcInterval();
    BenchmarkAccumulator::instance()->addRun(m_thisRun);
  }


  //================================================== BenchmarkAccumulator

  __DEFINE_LOG_CHANNEL__(BenchmarkAccumulator, lsInfo)

  BenchmarkAccumulator *BenchmarkAccumulator::instance() {
    static BenchmarkAccumulator s_instance;
    return &s_instance;
  }

  BenchmarkAccumulator::BenchmarkAccumulator() {
  }

  void BenchmarkAccumulator::addRun(const SubscriptionTime &newTiming) {
    boost::lock_guard<boost::mutex> lock(m_mutex);

    /* lookup struct with accumulated values of the script */
    subscriptionTimes_t::iterator elt;
    elt = std::find_if(m_subscriptionTimes.begin(), m_subscriptionTimes.end(),
                       bind(&SubscriptionTime::operator==, newTiming, ::_1));

    if (elt == m_subscriptionTimes.end()) {
      log(std::string("Create new entry for ") + newTiming.toString(), lsDebug);
      m_subscriptionTimes.push_back(newTiming);
      m_subscriptionTimes.back().count = 1;
      return;
    }

    /* add init/exit overhead values */
    elt->m_init += newTiming.m_init;
    elt->m_end += newTiming.m_end;
    elt->count++;

    if (elt->scripts.size() != newTiming.scripts.size()) {
      log(std::string("Number of sub-scripts do not match ") + newTiming.m_subscriptionName +
          " " + intToString(newTiming.scripts.size()) + "/" +
          intToString(elt->scripts.size()) + " skipping",
          lsWarning);
      foreach(ScriptTime foo, elt->scripts) {
        log(std::string(": ") + foo.name, lsWarning);
      }
      foreach(ScriptTime foo, newTiming.scripts) {
        log(std::string("; ") + foo.name, lsWarning);
      }
      return;
    }

    /* iterate over sub scripts of the subscription */
    SubscriptionTime::scripts_t::const_iterator cur_script = newTiming.scripts.begin();
    foreach(ScriptTime accum, elt->scripts) {
      if (accum.name != cur_script->name) {
        log(std::string("Script name mismatch ") + accum.name + " " + cur_script->name,
            lsWarning);
        break;
      }
      accum.totalTime += cur_script->totalTime;
      cur_script++;
    }
  }

  void BenchmarkAccumulator::upload() {
    log(__func__, lsDebug);
    boost::lock_guard<boost::mutex> lock(m_mutex);

    /* we populate folder '/system/js/timings' */

    PropertyNodePtr propFolder =
      DSS::getInstance()->getPropertySystem().createProperty("/system/js");

    PropertyNodePtr timingTree = propFolder->getProperty("timings");
    if (timingTree) {
      /* flush the whole tree, so we can add a newly sorted one */
      propFolder->removeChild(timingTree);
    }

    /* sorted by total(-time) which is computed each time, TODO */
    std::sort(m_subscriptionTimes.begin(), m_subscriptionTimes.end());
    std::reverse(m_subscriptionTimes.begin(), m_subscriptionTimes.end());

    timingTree = propFolder->createProperty("timings");
    timingTree->createProperty("updated")->setStringValue(DateTime().toString());

    foreach(const SubscriptionTime &subscription, m_subscriptionTimes) {
      uploadSubscriptionTime(timingTree, subscription);
    }
  }

  void BenchmarkAccumulator::uploadSubscriptionTime(PropertyNodePtr timingTree, const
                                                 SubscriptionTime subs) {
    std::string expSubsName = subs.m_subscriptionName + "@" + subs.m_eventName;
    dss::replaceAll(expSubsName, "/", "_");

    PropertyNodePtr myProp = timingTree->createProperty(expSubsName);

    try {
        myProp->createProperty("count")->setIntegerValue(subs.count);
        myProp->createProperty("total")->setIntegerValue(subs.total().toMicroSec());
        myProp->createProperty("pre")->setIntegerValue(subs.m_init.toMicroSec());
        myProp->createProperty("post")->setIntegerValue(subs.m_end.toMicroSec());
    } catch(PropertyTypeMismatch& ex) {
      log(std::string("Subscription: ") + expSubsName + " Datatype error " +
          ex.what(), lsWarning);
      return;
    } catch(std::runtime_error& ex) {
      log(std::string("Subscription: ") + expSubsName + " General error " +
          ex.what(), lsError);
      return;
    }

    foreach(const ScriptTime &script, subs.scripts) {
      std::string expScriptName = script.name;
      dss::replaceAll(expScriptName, "/", "_");
      PropertyNodePtr myScriptProp = myProp->createProperty(expScriptName);

      try {
        myScriptProp->createProperty("totalTime")->setIntegerValue(script.totalTime.toMicroSec());
      } catch(PropertyTypeMismatch& ex) {
        log(std::string("Script : ") + expSubsName + "/" + expScriptName +
            " Datatype error " + ex.what(), lsWarning);
        continue;
      } catch(std::runtime_error& ex) {
        log(std::string("Script : ") + expSubsName+ "/" + expScriptName +
            " General error " + ex.what(), lsWarning);
        continue;
      }
    }
  }

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
    log(std::string(__func__) + " " + _event.getName() + " " + intToString(ct), lsInfo);
    BenchmarkAccumulator::instance()->upload();
  }

} // namespace

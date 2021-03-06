#ifndef _TIMER_TOOLS_H_
#define _TIMER_TOOLS_H_

#include <time.h>
#include <iostream>
#include <vector>

#include "event.h"
#include "logger.h"
#include "logger.h"
#include "propertysystem.h"

namespace dss {
  /*
   * plan is  to extend benchmarking for all types of subscriptions,
   * even to the BenchmarkPublisherPlugin itself
   * TODO use base class of this type, and use that in SubscriptionTime.
   *      Simple subscription will not fine grain timestamps, other subscriptions
   *      types might use other sub timestamps
   */
  struct ScriptTime {
    std::string name;
#if 0
    TimeStamp compileTime;
    TimeStamp executionTime;
#endif
    TimeStamp totalTime;
  };

  struct SubscriptionTime {
    std::string m_eventName;
    std::string m_subscriptionName;
    TimeStamp m_init, m_end;
    unsigned count;

    bool operator==(const SubscriptionTime &other) const;
    bool operator<(const SubscriptionTime &other) const;
    std::ostream& operator<<(std::ostream &strm) const;
    std::string toString() const;

    TimeStamp total() const;

    typedef std::vector<ScriptTime> scripts_t;
    std::vector<ScriptTime> scripts;
  };

  /**
   * Convenience wrapper for profiling. Create this type at the beginning of
   * the section you want to measure, when done call stopEvent() and the
   * timestamp will be added to the previous runs of this same section.
   * Measurement runs are identified by event and script_id. Script_id is used
   * instead of handler name since javascript scripts all use the same
   * (javascript-)handler
   */
  class StopWatch {
    __DECL_LOG_CHANNEL__
  public:
    StopWatch(const std::string &eventName);
    void startEvent();
    void startSubscription();
    void setSubscriptionName(const std::string &subscription);
    void startScript();
    void setScriptName(const std::string &name);
    void stopScript();
    void stopSubscription();
    void stopEvent();

    /**
     * usually you want to take the first timestamp immediately, before checking
     * that you actually are doing benchmarking or not, because you want to measure
     * that part as well.
     */
    void cancel() { m_cancelled = true; }
  private:
    TimeStamp calcInterval();
    std::string m_eventName;
    TimeStamp m_tic;
    SubscriptionTime m_thisRun;
    ScriptTime m_script;
    bool  m_cancelled;
  };

  class BenchmarkAccumulator {
    __DECL_LOG_CHANNEL__
  public:
    void upload();
    void addRun(const SubscriptionTime &subsTime);
    static BenchmarkAccumulator* instance();

  private:
    BenchmarkAccumulator();
    void uploadSubscriptionTime(PropertyNodePtr timingTree, const
                                SubscriptionTime subs);
    boost::mutex m_mutex;
    typedef std::vector<SubscriptionTime> subscriptionTimes_t;
    std::vector<SubscriptionTime> m_subscriptionTimes;
    DateTime m_startTime;
  };

  class BenchmarkPublisherPlugin : public EventInterpreterPlugin {
    __DECL_LOG_CHANNEL__
  public:
    BenchmarkPublisherPlugin(EventInterpreter* _pInterpreter);
    virtual ~BenchmarkPublisherPlugin();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

}

#endif

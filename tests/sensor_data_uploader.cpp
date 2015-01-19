#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <set>

#include "sensor_data_uploader.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(SensorDataUploaderTest)

struct EventFactory {
  void emit_events(boost::shared_ptr<SensorLog> s, int count,
                   bool auto_flush = true)
  {
    while (count--) {
      boost::shared_ptr<Event> event =
        boost::make_shared<Event>(EventName::DeviceSensorValue);
      event->setProperty("index", intToString(m_events.size()));

      m_events.push_back(event);
      s->append(event, false);

      if (auto_flush && (m_events.size() % SensorLog::max_elements == 0)) {
        s->triggerUpload();
      }
    }

    if (auto_flush) {
      s->triggerUpload();
    }
  }

  std::vector<boost::shared_ptr<Event> > m_events;
};

struct MockUploader : public SensorLog::Uploader {
  MockUploader(RestTransferStatus_t restcode = REST_OK,
               int wscode = 0)
    : m_restcode(restcode), m_wscode(wscode) {}

  void install_upload_action(boost::function<void()> action)
  {
    m_upload_action = action;
  }

  void upload(SensorLog::It it, SensorLog::It end,
              WebserviceCallDone_t callback)
  {
    if (m_upload_action) {
      m_upload_action();
    }

    m_events.insert(m_events.end(), it, end);
    WebserviceReply ws_reply = { m_wscode, "fake-reply" };
    callback->done(m_restcode, ws_reply);
  }

  RestTransferStatus_t m_restcode;
  int m_wscode;
  std::vector<boost::shared_ptr<Event> > m_events;
  boost::function<void()> m_upload_action;
};

std::set<int> filter_indices(const std::vector<boost::shared_ptr<Event> > &recv_events,
                             const std::vector<boost::shared_ptr<Event> > &sent_events)
{
  std::set<int> indices;
  BOOST_FOREACH (boost::shared_ptr<Event> evt, recv_events) {
    int index = strtol(evt->getPropertyByName("index").c_str(), NULL, 10);
    indices.insert(index);

    // did we create an event with such index
    BOOST_CHECK_EQUAL(evt, sent_events[index]);
  }
  return indices;
}

BOOST_AUTO_TEST_CASE(test_upload_10k_events) {
  MockUploader mu(REST_OK);
  EventFactory f;

  boost::shared_ptr<SensorLog> s =
    boost::make_shared<SensorLog>("unit-test", &mu);

  f.emit_events(s, 10 * 1000);

  // received all events
  BOOST_CHECK_EQUAL(mu.m_events.size(), f.m_events.size());

  // all events had a different index?
  std::set<int> indices = filter_indices(mu.m_events, f.m_events);
  BOOST_CHECK_EQUAL(mu.m_events.size(), indices.size());
}

BOOST_AUTO_TEST_CASE(test_upload_network_failure) {
  //
  // will collect all the events to be uploaded but always tell
  // Sensor Log it faild due to network error
  //
  MockUploader mu(NETWORK_ERROR);
  EventFactory f;

  boost::shared_ptr<SensorLog> s =
    boost::make_shared<SensorLog>("unit-test", &mu);

  f.emit_events(s, SensorLog::max_elements, false);
  s->triggerUpload();

  //
  // SensorLog will not retry after an error occured, so only first
  // batch of date was sent, then transmission stops
  //
  BOOST_CHECK_EQUAL(mu.m_events.size(),
                    static_cast<size_t>(SensorLog::max_post_events));
  std::set<int> indices1 = filter_indices(mu.m_events, f.m_events);

  // SensorLog will retry uploading same events
  s->triggerUpload();  // retry
  BOOST_CHECK_EQUAL(mu.m_events.size(),
                    2 * static_cast<size_t>(SensorLog::max_post_events));
  std::set<int> indices2 = filter_indices(mu.m_events, f.m_events);
  BOOST_CHECK(indices1 == indices2); // no new index seen
}

BOOST_AUTO_TEST_CASE(test_upload_reply_parse_error) {
  //
  // will collect all the events to be uploaded but always tell
  // Sensor Log it faild due to json error
  //
  MockUploader mu(JSON_ERROR);
  EventFactory f;

  boost::shared_ptr<SensorLog> s =
    boost::make_shared<SensorLog>("unit-test", &mu);

  f.emit_events(s, 3 * SensorLog::max_post_events, false);
  s->triggerUpload();

  //
  // SensorLog will drop the events without retry, so all
  // events will be transferred
  //
  BOOST_CHECK_EQUAL(mu.m_events.size(),
                    3 * static_cast<size_t>(SensorLog::max_post_events));
  std::set<int> indices = filter_indices(mu.m_events, f.m_events);
  BOOST_CHECK_EQUAL(mu.m_events.size(), indices.size());
}

BOOST_AUTO_TEST_CASE(test_upload_ws_complains) {
  //
  // will collect all the events to be uploaded but always tell
  // Sensor Log it faild due to json error
  //
  MockUploader mu(REST_OK, 0x3f);
  EventFactory f;

  boost::shared_ptr<SensorLog> s =
    boost::make_shared<SensorLog>("unit-test", &mu);

  f.emit_events(s, 3 * SensorLog::max_post_events, false);
  s->triggerUpload();

  //
  // Webservice complains about something, but we will continue
  //
  BOOST_CHECK_EQUAL(mu.m_events.size(),
                    3 * static_cast<size_t>(SensorLog::max_post_events));
  std::set<int> indices = filter_indices(mu.m_events, f.m_events);
  BOOST_CHECK_EQUAL(mu.m_events.size(), indices.size());
}

BOOST_AUTO_TEST_CASE(test_sensor_data_trickle) {
  EventFactory f;
  MockUploader mu;

  boost::shared_ptr<SensorLog> s =
    boost::make_shared<SensorLog>("unit-test", &mu);

  // whenever a packet is sent, a new sensor value arrives
  mu.install_upload_action(boost::bind(&EventFactory::emit_events, &f, s, 1,
                                       false));
  f.emit_events(s, SensorLog::max_post_events, true);

  //
  // uploaded a full packet and one packet containing only 1 event
  // another event is queued
  //
  BOOST_CHECK_EQUAL(mu.m_events.size(),
                    static_cast<size_t>(SensorLog::max_post_events) + 1);
  BOOST_CHECK_EQUAL(mu.m_events.size(), f.m_events.size() - 1);
}

BOOST_AUTO_TEST_CASE(test_sensor_data_trickle2) {
  EventFactory f;
  MockUploader mu;

  boost::shared_ptr<SensorLog> s =
    boost::make_shared<SensorLog>("unit-test", &mu);

  // whenever a packet is sent, a new sensor value arrives
  mu.install_upload_action(boost::bind(&EventFactory::emit_events, &f, s,
                                       SensorLog::max_post_events / 2,
                                       false));
  f.emit_events(s, 2 * SensorLog::max_post_events, true);

  //
  // while the first 2 full packet are uploaded, another packet trickles in
  // while uploading that, another 1/2 packet worth of data trickles in
  // while uploading that, another 1/2 packet worth of data trickles in
  // that packet is not uploaded since the packet before was not full
  //
  BOOST_CHECK_EQUAL(mu.m_events.size(),
                    static_cast<size_t>(SensorLog::max_post_events) * (2 + 1 + 0.5));
  BOOST_CHECK_EQUAL(mu.m_events.size() + SensorLog::max_post_events / 2,
                    f.m_events.size());
}

BOOST_AUTO_TEST_SUITE_END()

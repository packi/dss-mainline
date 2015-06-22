#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <curl/curl.h>
#include <iostream>
#include "foreach.h"

#include "event.h"
#include "event/event_create.h"
#include "eventinterpreterplugins.h"
#include "http_client.h"
#include "model/apartment.h"
#include "model/devicereference.h"
#include "model/zone.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "sessionmanager.h"
#include "unix/systeminfo.h"
#include "webservice_api.h"
#include "src/web/webrequests.h"
#include "tests/util/dss_instance_fixture.h"

using namespace dss;

static const char *websvc_url_authority_test = "https://testdsservices.aizo.com/";
static const char *websvc_osp_token_test = "76sdf9786f7d6fsd78f6asd7f6sd78f6sdf896as";

BOOST_AUTO_TEST_SUITE(WebserviceTest)

BOOST_AUTO_TEST_CASE(parseModelChangeTest) {

  /* test sample response */
  WebserviceReply resp;
  {
    const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":\"The dSS was not found in the database\"}";
    BOOST_CHECK_NO_THROW(resp = WebserviceMsHub::parseReply(buf));
    BOOST_CHECK_EQUAL(resp.code, 2);
    BOOST_CHECK_EQUAL(resp.desc, "The dSS was not found in the database");
  }
  {
    const char *buf =  "{\"ReturnCode\":\"2\",\"ReturnMessage\":\"The dSS was not found in the database\"}";
    BOOST_CHECK_THROW(resp = WebserviceMsHub::parseReply(buf), ParseError);
  }
  {
    const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":42}";
    BOOST_CHECK_THROW(resp = WebserviceMsHub::parseReply(buf), ParseError);
  }
  {
    const char *buf =  "";
    BOOST_CHECK_THROW(resp = WebserviceMsHub::parseReply(buf), ParseError);
  }
  {
    const char *buf =  "{128471049asdfhla}";
    BOOST_CHECK_THROW(resp = WebserviceMsHub::parseReply(buf), ParseError);
  }
  {
    const char *buf =  NULL;
    BOOST_CHECK_THROW(resp = WebserviceMsHub::parseReply(buf), ParseError);
  }
}

class WebserviceFixture {
public:
  WebserviceFixture(const std::string &url, const std::string &token) {

    /* TODO clean this up */
    SystemInfo info;
    info.collect();
    DSS::getInstance()->publishDSID();

    /* switch from production to test webservice */
    PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
    propSystem.getProperty(pp_websvc_url_authority)->setStringValue(url);
    propSystem.createProperty(pp_websvc_rc_osptoken)->setStringValue(token);

    // TODO: webservice connection fetched original authority, restart it
    WebserviceConnection::shutdown();
    WebserviceConnection::getInstanceMsHub();

    propSystem.createProperty(pp_websvc_enabled)->setBooleanValue(true);
    propSystem.createProperty(pp_websvc_mshub_active)->setBooleanValue(true);
  }

  DSSInstanceFixture m_dssInstanceCtrl;
};


class WebserviceFixtureReal : public WebserviceFixture {
public:
  /* test instance but still real implementation */
  WebserviceFixtureReal()
    : WebserviceFixture(websvc_url_authority_test,
                        websvc_osp_token_test) {}
};

class NotifyDone : public WebserviceCallDone {
public:
  NotifyDone(boost::mutex &mutex,
             boost::condition_variable &completion)
    : m_mutex(mutex)
    , m_completion(completion)
  {
  }

  void done(RestTransferStatus_t status, WebserviceReply reply) {
    boost::mutex::scoped_lock lock(m_mutex);
    this->status = status;
    this->reply = reply;
    m_completion.notify_one();
  }

  WebserviceReply reply;
  RestTransferStatus_t status;
private:
  boost::mutex &m_mutex;
  boost::condition_variable &m_completion;
};

BOOST_FIXTURE_TEST_CASE(test_notifyApartmentChange, WebserviceFixtureReal) {
  boost::mutex mutex;
  boost::condition_variable completion;

  WebserviceCallDone_t cont(new NotifyDone(mutex, completion));
  NotifyDone *notifyDone = static_cast<NotifyDone*>(cont.get());

  boost::mutex::scoped_lock lock(mutex);
  WebserviceMsHub::doModelChanged(WebserviceMsHub::ApartmentChange,
                                  cont);
  completion.wait(lock);

  BOOST_CHECK_EQUAL(notifyDone->status, REST_OK);
  BOOST_CHECK_EQUAL(notifyDone->reply.code, 98); /* no access rights for token */
}

/* Access Management */
BOOST_FIXTURE_TEST_CASE(test_revokeToken, WebserviceFixtureReal) {
  boost::mutex mutex;
  boost::condition_variable completion;

  WebserviceCallDone_t cont(new NotifyDone(mutex, completion));
  NotifyDone *notifyDone = static_cast<NotifyDone*>(cont.get());

  boost::mutex::scoped_lock lock(mutex);
  WebserviceMsHub::doNotifyTokenDeleted(SessionTokenGenerator::generate(),
                                        cont);
  completion.wait(lock);

  BOOST_CHECK_EQUAL(notifyDone->status, REST_OK);
  BOOST_CHECK_EQUAL(notifyDone->reply.code, 98); /* no access rights for token */
}

BOOST_FIXTURE_TEST_CASE(test_WebscvEnableDisablePlugin, WebserviceFixtureReal) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();

  // also starts event runner et. al
  m_dssInstanceCtrl.initPlugins();

  // check event subscriptions when webservice is enabled:
  // (ms-hub keepalive, event uploder mshub + dshub, weather downloader)
  propSystem.createProperty(pp_websvc_enabled)->setBooleanValue(true);
  BOOST_CHECK_EQUAL(DSS::getInstance()->getEventRunner().getSize(), 2);
  propSystem.createProperty(pp_websvc_enabled)->setBooleanValue(false);
  BOOST_CHECK_EQUAL(DSS::getInstance()->getEventRunner().getSize(), 0);
}

BOOST_AUTO_TEST_CASE(webservice_ms_json) {
  boost::shared_ptr<Event> pEvent;

  pEvent = boost::make_shared<Event>("aaaahhh");
  {
    JSONWriter json(JSONWriter::jsonNoneResult);
    BOOST_CHECK_THROW(MsHub::toJson(pEvent, json), DSSException);
  }

  {
    JSONWriter json(JSONWriter::jsonNoneResult);
    pEvent = boost::make_shared<Event>(EventName::DeviceSensorValue);
    BOOST_CHECK_THROW(MsHub::toJson(pEvent, json), DSSException);
  }

  pEvent = boost::make_shared<Event>(EventName::HeatingControllerSetup);
  JSONWriter json(JSONWriter::jsonNoneResult);
  BOOST_CHECK_NO_THROW(MsHub::toJson(pEvent, json));
  BOOST_CHECK_EQUAL(json.successJSON().substr(0, 90), std::string("{\"EventGroup\":\"ApartmentAndDevice\",\"EventCategory\":\"HeatingControllerSetup\",\"Timestamp\":\"2015-02-19T16:42:42.298Z\"}").substr(0, 90));
}

BOOST_FIXTURE_TEST_CASE(webservice_ds_json, DSSInstanceFixture) {
  /* DsHub::createHeader, requires instance */
  boost::shared_ptr<Event> pEvent;

  {
    JSONWriter json(JSONWriter::jsonNoneResult);
    pEvent = boost::make_shared<Event>("aaaahhh");
    BOOST_CHECK_THROW(DsHub::toJson(pEvent, json), DSSException);
  }

  {
    JSONWriter json(JSONWriter::jsonNoneResult);
    pEvent = boost::make_shared<Event>(EventName::DeviceSensorValue);
    BOOST_CHECK_THROW(DsHub::toJson(pEvent, json), DSSException);
  }

  pEvent = boost::make_shared<Event>(EventName::HeatingControllerSetup);
  JSONWriter json(JSONWriter::jsonNoneResult);
  BOOST_CHECK_NO_THROW(DsHub::toJson(pEvent, json));
  BOOST_CHECK_EQUAL(json.successJSON().substr(0, 126), std::string("{\"EventHeader\":{\"EventCategory\":\"HeatingControllerSetup\",\"EventGroup\":\"ApartmentAndDevice\",\"SequenceID\":0,\"SequenceIDSince\":\"2015-02-19T16:42:42Z\",\"Timestamp\":\"2015-02-19T16:42:42Z\",\"EventID\":\"a6f31327-098b-4c7e-bd50-bb8fb7e8b98b\",\"Source\":true},\"EventBody\":{}}").substr(0, 126));
}

class EventFactory {
public:
  EventFactory() {}
  boost::shared_ptr<Event> createEvent(const std::string& eventName);

  boost::shared_ptr<DeviceReference> createDevRef() {
    Apartment &apartment(DSS::getInstance()->getApartment());
    boost::shared_ptr<Device> dev = apartment.allocateDevice(dsuid_t());
    return boost::make_shared<DeviceReference>(dev, &apartment);
  }

  boost::shared_ptr<Group> createGroup(int id) {
    Apartment &apartment(DSS::getInstance()->getApartment());
    boost::shared_ptr<Zone> zone = apartment.getZone(0);
    return zone->getGroup(id);
  }

  boost::shared_ptr<State> createState(eStateType type) {
    return boost::make_shared<State>(type, "dummy-state", "unit-test");
  }

private:
  DSSInstanceFixture m_dss_guard;
};

boost::shared_ptr<Event> EventFactory::createEvent(const std::string& eventName)
{
  boost::shared_ptr<Event> pEvent;

  if (eventName == EventName::DeviceBinaryInputEvent) {
    pEvent = createDeviceBinaryInputEvent(createDevRef(), 0, 1, 7);
  } else if (eventName == EventName::DeviceSensorValue) {
    pEvent = createDeviceSensorValueEvent(createDevRef(), 0, 1, 7);
  } else if (eventName == EventName::DeviceStatus) {
    pEvent = createDeviceStatusEvent(createDevRef(), 0, 1);
  } else if (eventName == EventName::DeviceInvalidSensor) {
    pEvent = createDeviceInvalidSensorEvent(createDevRef(), 0, 1, DateTime());
  } else if (eventName == EventName::ZoneSensorValue) {
    pEvent = createZoneSensorValueEvent(createGroup(1), 0, 1, DSUID_NULL);
  } else if (eventName == EventName::ZoneSensorError) {
    pEvent = createZoneSensorErrorEvent(createGroup(1), 0, DateTime());
  } else if (eventName == EventName::CallScene) {
    pEvent = createGroupCallSceneEvent(createGroup(1), 1, 1, 1,
                                       callOrigin_t(2), dsuid_t(),
                                       "fake-token", false);
  } else if (eventName == EventName::UndoScene) {
    pEvent = createGroupUndoSceneEvent(createGroup(1), 1, 1, 1,
                                       callOrigin_t(2), dsuid_t(),
                                       "fake-token");
  } else if (eventName == EventName::AddonStateChange) {
    pEvent = createStateChangeEvent(createState(StateType_Script), State_Unknown, coTest);
  } else if (eventName == EventName::StateChange) {
    pEvent = createStateChangeEvent(createState(StateType_Device), State_Unknown, coTest);
  } else if (eventName == EventName::ExecutionDenied) {
    pEvent = createActionDenied("device-scene", "action-node", "unit-test", "testcase");
  } else if (eventName == EventName::HeatingEnabled) {
    // TODO created by javascript, sync paramter manually
    pEvent = createHeatingEnabled(1, true);
  } else if (eventName == EventName::HeatingControllerSetup) {
    ZoneHeatingConfigSpec_t spec;
    memset(&spec, 0x7f, sizeof(ZoneHeatingConfigSpec_t));
    pEvent = createHeatingControllerConfig(1, DSUID_BROADCAST, spec);
  } else if (eventName == EventName::HeatingControllerValue) {
    ZoneHeatingProperties_t props;
    ZoneHeatingOperationModeSpec_t mode;
    memset(&props, 0x5f, sizeof(props));
    props.m_HeatingControlMode = HeatingControlModeIDPID;
    memset(&mode, 0xf7, sizeof(mode));
    pEvent = createHeatingControllerValue(1, DSUID_BROADCAST, props, mode);
  } else if (eventName == EventName::HeatingControllerValueDsHub) {
    ZoneHeatingProperties_t props;
    ZoneHeatingStatus_t stat;
    memset(&props, 0x5f, sizeof(props));
    props.m_HeatingControlMode = HeatingControlModeIDPID;
    stat.m_NominalValue = stat.m_ControlValue = 22;
    pEvent = createHeatingControllerValueDsHub(1, 3, props, stat);
  } else if (eventName == EventName::HeatingControllerState) {
    pEvent = createHeatingControllerState(1, DSUID_BROADCAST, 0x7f);
  } else if (eventName == EventName::OldStateChange) {
    pEvent = createOldStateChange("UT_ScriptID", "UT_Name", "UT_Value", coTest);
  } else {
    // enable with '-l warning'
    BOOST_WARN_MESSAGE(pEvent, "Failed to create event <" + eventName + ">");
    return pEvent;
  }

  BOOST_CHECK_EQUAL(pEvent->getName(), eventName);
  return pEvent;
}

BOOST_FIXTURE_TEST_CASE(test_mshub_tojson, EventFactory) {
  boost::shared_ptr<Event> pEvent;

  foreach (std::string event, MsHub::uploadEvents()) {
    pEvent = createEvent(event);
    if (!pEvent) {
      continue;
    }

    JSONWriter json;
    BOOST_CHECK_NO_THROW(MsHub::toJson(pEvent, json));
  }
}

BOOST_FIXTURE_TEST_CASE(test_dshub_tojson, EventFactory) {
  boost::shared_ptr<Event> pEvent;

  foreach (std::string event, DsHub::uploadEvents()) {
    pEvent = createEvent(event);
    if (!pEvent) {
      continue;
    }

    JSONWriter json;
    BOOST_CHECK_NO_THROW(DsHub::toJson(pEvent, json));
  }
}

BOOST_AUTO_TEST_SUITE_END()

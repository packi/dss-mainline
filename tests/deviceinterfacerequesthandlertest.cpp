/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "webfixture.h"

#include "core/model/deviceinterface.h"
#include "core/web/handler/deviceinterfacerequesthandler.h"
#include "core/web/json.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebDeviceInterface)

  class DeviceInterfaceDummy : public IDeviceInterface {
  public:

    DeviceInterfaceDummy()
    : m_NumberOfCalls(0)
    {
    }
    virtual void turnOn() {
      functionCalled("turnOn");
    }
    virtual void turnOff() {
      functionCalled("turnOff");
    }
    virtual void increaseValue()  {
      functionCalled("increaseValue");
    }
    virtual void decreaseValue()  {
      functionCalled("decreaseValue");
    }
    virtual void setValue(const uint8_t _value)  {
      functionCalled("setValue(" + intToString(_value) + ")");
    }
    virtual void callScene(const int _sceneNr, const bool _force)  {
      functionCalled("callScene(" + intToString(_sceneNr) + ")");
    }
    virtual void saveScene(const int _sceneNr)  {
      functionCalled("saveScene(" + intToString(_sceneNr) + ")");
    }
    virtual void undoScene(const int _sceneNr)  {
      functionCalled("undoScene(" + intToString(_sceneNr) + ")");
    }
    virtual void undoSceneLast()  {
      functionCalled("undoSceneLast");
    }
    virtual unsigned long getPowerConsumption()  {
      functionCalled("getConsumption");
      return 50;
    }
    virtual void nextScene()  {
      functionCalled("nextScene");
    }
    virtual void previousScene()  {
      functionCalled("previousScene");
    }
    virtual void blink()  {
      functionCalled("blink");
    }

    int getNumberOfCalls() const { return m_NumberOfCalls; }
    const std::string& getLastFunction() const { return m_LastFunction; }

  private:
    void functionCalled(const std::string& _name) {
      m_NumberOfCalls++;
      m_LastFunction = _name;
    }
  private:
    int m_NumberOfCalls;
    std::string m_LastFunction;
  }; // IDeviceInterface


class DeviceInterfaceRequestHandlerValid : public DeviceInterfaceRequestHandler {
public:
  virtual WebServerResponse jsonHandleRequest(
    const RestfulRequest& _request,
    boost::shared_ptr<Session> _session) {
    boost::shared_ptr<JSONObject> result;
    return result;
  }
};

class Fixture : public WebFixture {
public:

  void testFunction(const std::string& _functionName) {
    HashMapConstStringString empty;
    testFunction(_functionName, _functionName, empty);
  }

  void testFunction(const std::string& _functionName, const std::string& _paramName, const std::string& _paramValue) {
    HashMapConstStringString params;
    params[_paramName] = _paramValue;
    testFunction(_functionName, _functionName + "(" + _paramValue + ")", params);
  }

  void testFunction(const std::string& _functionName, const std::string& _paramName, const std::string& _paramValue, const std::string& _resultingFunctionName) {
    HashMapConstStringString params;
    params[_paramName] = _paramValue;
    testFunction(_functionName, _resultingFunctionName, params);
  }
private:
  void testFunction(const std::string& _functionName, const std::string& _functionNameWithParams, const HashMapConstStringString& _params) {
    boost::shared_ptr<DeviceInterfaceDummy> dummy(new DeviceInterfaceDummy);
    RestfulRequest req("bla/" + _functionName, _params);
    WebServerResponse response = m_RequestHandler.handleDeviceInterfaceRequest(req, dummy);
    BOOST_CHECK_EQUAL(dummy->getNumberOfCalls(), 1);
    BOOST_CHECK_EQUAL(dummy->getLastFunction(), _functionNameWithParams);
    testOkIs(response, true);
  }

  DeviceInterfaceRequestHandlerValid m_RequestHandler;
};

BOOST_FIXTURE_TEST_CASE(testTurnOff, Fixture) {
  testFunction("turnOff");
}

BOOST_FIXTURE_TEST_CASE(testTurnOn, Fixture) {
  testFunction("turnOn");
}

BOOST_FIXTURE_TEST_CASE(testIncreaseValue, Fixture) {
  testFunction("increaseValue");
}

BOOST_FIXTURE_TEST_CASE(testDecreaseValue, Fixture) {
  testFunction("decreaseValue");
}

BOOST_FIXTURE_TEST_CASE(testSetValue, Fixture) {
  testFunction("setValue", "value", "5");
}

BOOST_FIXTURE_TEST_CASE(testCallScene, Fixture) {
  testFunction("callScene", "sceneNumber", "5");
}

BOOST_FIXTURE_TEST_CASE(testSaveScene, Fixture) {
  testFunction("saveScene", "sceneNumber", "5");
}

BOOST_FIXTURE_TEST_CASE(testUndoScene, Fixture) {
  testFunction("undoScene", "sceneNumber", "5");
}

BOOST_FIXTURE_TEST_CASE(testGetPowerConsumption, Fixture) {
  testFunction("getConsumption");
}

BOOST_FIXTURE_TEST_CASE(testBlink, Fixture) {
  testFunction("blink");
}

BOOST_AUTO_TEST_SUITE_END()

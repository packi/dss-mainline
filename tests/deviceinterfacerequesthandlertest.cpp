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

#include "src/model/deviceinterface.h"
#include "src/web/handler/deviceinterfacerequesthandler.h"
#include "src/web/json.h"
#include "src/session.h"
#include "src/model/scenehelper.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebDeviceInterface)

  class DeviceInterfaceDummy : public IDeviceInterface {
  public:

    DeviceInterfaceDummy()
    : m_NumberOfCalls(0)
    {
    }
    virtual void turnOn(const callOrigin_t _origin, const SceneAccessCategory _category) {
      functionCalled("turnOn");
    }
    virtual void turnOff(const callOrigin_t _origin, const SceneAccessCategory _category) {
      functionCalled("turnOff");
    }
    virtual void increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category)  {
      functionCalled("increaseValue");
    }
    virtual void decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category)  {
      functionCalled("decreaseValue");
    }
    virtual void setValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token)  {
      functionCalled("setValue(" + intToString(_value) + ")");
    }
    virtual void callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token, const bool _force)  {
      functionCalled("callScene(" + intToString(_sceneNr) + ")");
    }
    virtual void saveScene(const callOrigin_t _origin, const int _sceneNr, const std::string _token)  {
      functionCalled("saveScene(" + intToString(_sceneNr) + ")");
    }
    virtual void undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token)  {
      functionCalled("undoScene(" + intToString(_sceneNr) + ")");
    }
    virtual void undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token)  {
      functionCalled("undoSceneLast");
    }
    virtual unsigned long getPowerConsumption()  {
      functionCalled("getConsumption");
      return 50;
    }
    virtual void nextScene(const callOrigin_t _origin, const SceneAccessCategory _category)  {
      functionCalled("nextScene");
    }
    virtual void previousScene(const callOrigin_t _origin, const SceneAccessCategory _category)  {
      functionCalled("previousScene");
    }
    virtual void blink(const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token)  {
      functionCalled("blink");
    }
    virtual void increaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _channel, const std::string _token)  {
      functionCalled("increaseOutputChannelValue(" + intToString(_channel) + ")");
    }
    virtual void decreaseOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _channel, const std::string _token)  {
      functionCalled("decreaseOutputChannelValue(" + intToString(_channel) + ")");
    }
    virtual void stopOutputChannelValue(const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _channel, const std::string _token)  {
      functionCalled("stopOutputChannelValue(" + intToString(_channel) + ")");
    }
    virtual void pushSensor(const callOrigin_t _origin, const SceneAccessCategory _category, dsuid_t _sourceID, uint8_t _sensorType, float _sensorValueFloat, const std::string _token) {
      (void)SceneHelper::sensorToSystem(_sensorType, _sensorValueFloat);
      functionCalled("pushSensorValue");
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
    testFunctionDo(_functionName, _functionName, "");
  }

  void testFunctionOK(const std::string& _functionName, const std::string& _params) {
    testFunctionDo(_functionName, _functionName, _params);
  }

  void testFunctionThrow(const std::string& _functionName, const std::string& _params) {
    testFunctionThrow(_functionName, _functionName, _params);
  }

  void testFunction(const std::string& _functionName, const std::string& _paramName, const std::string& _paramValue) {
    std::string params = urlEncode(_paramName) + "=" + urlEncode(_paramValue);
    testFunctionDo(_functionName, _functionName + "(" + _paramValue + ")", params);
  }

  void testFunction(const std::string& _functionName,
                    const std::string& _paramName,
                    const std::string& _paramValue,
                    const std::string& _resultingFunctionName) {
    std::string params = urlEncode(_paramName) + "=" + urlEncode(_paramValue);
    testFunctionDo(_functionName, _resultingFunctionName, params);
  }
private:
  void testFunctionDo(const std::string& _functionName,
                      const std::string& _functionNameWithParams,
                      const std::string& _params) {
    boost::shared_ptr<DeviceInterfaceDummy> dummy = boost::make_shared<DeviceInterfaceDummy>();
    boost::shared_ptr<Session> dummySession = boost::make_shared<Session>("dummy");
    RestfulRequest req("bla/" + _functionName, _params);
    boost::shared_ptr<JSONObject> temp;
    BOOST_CHECK_NO_THROW(temp =
      m_RequestHandler.handleDeviceInterfaceRequest(req, dummy, dummySession));
    WebServerResponse response = temp;
    BOOST_CHECK_EQUAL(dummy->getNumberOfCalls(), 1);
    BOOST_CHECK_EQUAL(dummy->getLastFunction(), _functionNameWithParams);
    testOkIs(response, true);
  }
  void testFunctionThrow(const std::string& _functionName,
                         const std::string& _functionNameWithParams,
                         const std::string& _params) {
    boost::shared_ptr<DeviceInterfaceDummy> dummy = boost::make_shared<DeviceInterfaceDummy>();
    boost::shared_ptr<Session> dummySession = boost::make_shared<Session>("dummy");
    RestfulRequest req("bla/" + _functionName, _params);
    boost::shared_ptr<JSONObject> temp;
    BOOST_CHECK_THROW(temp =
      m_RequestHandler.handleDeviceInterfaceRequest(req, dummy, dummySession), SensorOutOfRangeException);
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

BOOST_FIXTURE_TEST_CASE(testPushSensorIDRoomTemperatureSetpoint, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=50&sensorValue=25.5");
  testFunctionThrow("pushSensorValue", "sensorType=50&sensorValue=-50");
  testFunctionThrow("pushSensorValue", "sensorType=50&sensorValue=150.25");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDActivePower, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=4&sensorValue=548");
  testFunctionThrow("pushSensorValue", "sensorType=4&sensorValue=-1");
  testFunctionThrow("pushSensorValue", "sensorType=4&sensorValue=4096");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDOutputCurrent, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=5&sensorValue=658");
  testFunctionThrow("pushSensorValue", "sensorType=5&sensorValue=-1");
  testFunctionThrow("pushSensorValue", "sensorType=5&sensorValue=4096.32");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDElectricMeter, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=6&sensorValue=25.36");
  testFunctionThrow("pushSensorValue", "sensorType=6&sensorValue=-0.25");
  testFunctionThrow("pushSensorValue", "sensorType=6&sensorValue=41.265");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDOutputCurrentEx, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=64&sensorValue=15487");
  testFunctionThrow("pushSensorValue", "sensorType=64&sensorValue=-15");
  testFunctionThrow("pushSensorValue", "sensorType=64&sensorValue=16381");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDPowerConsumptionVA, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=65&sensorValue=659");
  testFunctionThrow("pushSensorValue", "sensorType=65&sensorValue=-1");
  testFunctionThrow("pushSensorValue", "sensorType=65&sensorValue=4096");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDTemperatureIndoors, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=9&sensorValue=25.251");
  testFunctionThrow("pushSensorValue", "sensorType=9&sensorValue=-44");
  testFunctionThrow("pushSensorValue", "sensorType=9&sensorValue=60");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDTemperatureOutdoors, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=10&sensorValue=-15.36");
  testFunctionThrow("pushSensorValue", "sensorType=10&sensorValue=-44");
  testFunctionThrow("pushSensorValue", "sensorType=10&sensorValue=59.524");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDBrightnessIndoors, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=11&sensorValue=45874.254");
  testFunctionThrow("pushSensorValue", "sensorType=11&sensorValue=0");
  testFunctionThrow("pushSensorValue", "sensorType=11&sensorValue=132000.254");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDBrightnessOutdoors, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=12&sensorValue=25");
  testFunctionThrow("pushSensorValue", "sensorType=12&sensorValue=-5698");
  testFunctionThrow("pushSensorValue", "sensorType=12&sensorValue=200000.326598");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDHumidityIndoors, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=13&sensorValue=0");
  testFunctionThrow("pushSensorValue", "sensorType=13&sensorValue=-1");
  testFunctionThrow("pushSensorValue", "sensorType=13&sensorValue=103.264");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDHumidityOutdoors, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=14&sensorValue=102.37");
  testFunctionThrow("pushSensorValue", "sensorType=14&sensorValue=-4654");
  testFunctionThrow("pushSensorValue", "sensorType=14&sensorValue=102.4");
}

//BOOST_FIXTURE_TEST_CASE(testPushSensorIDAirPressure, Fixture) {
//  testFunctionOK("pushSensorValue", "sensorType=15&sensorValue=1000");
//  testFunctionThrow("pushSensorValue", "sensorType=15&sensorValue=199");
//  testFunctionThrow("pushSensorValue", "sensorType=15&sensorValue=1224");
//}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDWindSpeed, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=18&sensorValue=35");
  testFunctionThrow("pushSensorValue", "sensorType=18&sensorValue=-4");
  testFunctionThrow("pushSensorValue", "sensorType=18&sensorValue=105");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDWindDirection, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=19&sensorValue=180");
  testFunctionThrow("pushSensorValue", "sensorType=19&sensorValue=-1");
  testFunctionThrow("pushSensorValue", "sensorType=19&sensorValue=512");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDPrecipitation, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=20&sensorValue=45");
  testFunctionThrow("pushSensorValue", "sensorType=20&sensorValue=-1");
  testFunctionThrow("pushSensorValue", "sensorType=20&sensorValue=102.5");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDCO2Concentration, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=21&sensorValue=50000");
  testFunctionThrow("pushSensorValue", "sensorType=21&sensorValue=0");
  testFunctionThrow("pushSensorValue", "sensorType=21&sensorValue=132000");
}

BOOST_FIXTURE_TEST_CASE(testPushSensorIDRoomTemperatureControlVariable, Fixture) {
  testFunctionOK("pushSensorValue", "sensorType=51&sensorValue=51.25");
  testFunctionThrow("pushSensorValue", "sensorType=51&sensorValue=-101");
  testFunctionThrow("pushSensorValue", "sensorType=51&sensorValue=104");
}

BOOST_AUTO_TEST_SUITE_END()

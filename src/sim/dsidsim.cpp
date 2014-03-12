/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "dsidsim.h"
#include "src/model/modelconst.h"

namespace dss {
  //================================================== DSIDSim

  DSIDSim::DSIDSim(const DSMeterSim& _simulator, const dss_dsid_t _dsid, const devid_t _shortAddress)
  : DSIDInterface(_simulator, _dsid, _shortAddress),
    m_Enabled(true),
    m_CurrentValue(0),
    m_SimpleConsumption(0)
  {
    m_ValuesForScene.resize(255);
    m_ValuesForScene[SceneOff] = 0;
    m_ValuesForScene[SceneMax] = 255;
    m_ValuesForScene[SceneMin] = 0;
    m_ValuesForScene[Scene1] = 255;
    m_ValuesForScene[Scene2] = 255;
    m_ValuesForScene[Scene3] = 255;
    m_ValuesForScene[Scene4] = 255;
    m_ValuesForScene[SceneStandBy] = 0;
    m_ValuesForScene[SceneDeepOff] = 0;
  } // ctor

  uint32_t DSIDSim::getPowerConsumption() {
	int _simpleConsumption=0;
	int _minConsumption=0;
	int _maxConsumption=0;
	int _jitterConsumption=0;
	// simple consumption calculation: just provide an max value, interpolated by currentvalue and some jitter
	// only for backward compatiliby
	if (getConfigParameter("SimpleConsumption")!="")
		_simpleConsumption= (int) ((m_CurrentValue / 255.0) * strToInt(getConfigParameter("SimpleConsumption"))) + (rand() % 100);

	if (getConfigParameter("MinConsumption")!="")
		_minConsumption=strToInt(getConfigParameter("MinConsumption"));

	if (getConfigParameter("MaxConsumption")!="")
		_maxConsumption= (int) ((m_CurrentValue / 255.0) * (strToInt(getConfigParameter("MaxConsumption")) - _minConsumption) );

	if (getConfigParameter("JitterConsumption")!="")
		_jitterConsumption= (int) (rand() % strToInt(getConfigParameter("JitterConsumption")));
	_jitterConsumption =_jitterConsumption - (_jitterConsumption/2);

    return (uint32_t) _simpleConsumption + _minConsumption + _maxConsumption + _jitterConsumption; //((m_CurrentValue / 255.0) * m_SimpleConsumption) + (rand() % 100);
  }

  uint32_t DSIDSim::getEnergyMeterValue() {
    return 0;
  }

  void DSIDSim::callScene(const int _sceneNr) {
    if(m_Enabled) {
      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // callScene

  void DSIDSim::saveScene(const int _sceneNr) {
    if(m_Enabled) {
      m_ValuesForScene[_sceneNr] = m_CurrentValue;
    }
  } // saveScene

  void DSIDSim::undoScene(const int _sceneNr) {
    if(m_Enabled) {
      // TODO: handle scene history properly
      //if(m_SceneHistory[0] == _sceneNr) {
      //  m_CurrentValue = m_ValuesForScene.at(m_SceneHistory[1]);
      //}
    }
  } // undoScene

  void DSIDSim::undoSceneLast() {
    if(m_Enabled) {
      // TODO: implement
//      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // undoSceneLast

  void DSIDSim::enable() {
    m_Enabled = true;
  } // enable

  void DSIDSim::disable() {
    m_Enabled = false;
  } // disable

  void DSIDSim::setDeviceConfig(const uint8_t _configClass,
                                const uint8_t _configIndex,
                                const uint8_t _value) {
    if(m_Enabled) {
      m_DeviceConfig[_configClass][_configIndex] = int(_value);
    }
  } // setDeviceConfig

  uint8_t DSIDSim::getDeviceConfig(const uint8_t _configClass,
                                   const uint8_t _configIndex) {
    return static_cast<uint8_t>(m_DeviceConfig[_configClass][_configIndex]);
  } // getValue

  uint16_t DSIDSim::getDeviceConfigWord(const uint8_t _configClass,
                                   const uint8_t _configIndex) {
    return static_cast<uint16_t>(m_DeviceConfig[_configClass][_configIndex]);
  } // getValue

  void DSIDSim::setDeviceButtonActiveGroup(const uint8_t _groupID) {
  } //setDeviceButtonActiveGroup

  void DSIDSim::setDeviceProgMode(const uint8_t _modeId) {
  } // setDeviceProgMode

  uint32_t DSIDSim::getDeviceSensorValue(const uint8_t _sensorIndex) {
    return 0;
  } // getDeviceSensorValue

  uint8_t DSIDSim::getDeviceSensorType(const uint8_t _sensorIndex) {
    return 255;
  } // getDeviceSensorType

  std::pair<uint8_t, uint16_t> DSIDSim::getTransmissionQuality() {
    uint8_t down = rand() % 255;
    uint8_t up = rand() % 255;
    return std::make_pair(down, up);
  }

  void DSIDSim::increaseDeviceOutputChannelValue(const uint8_t _channel) {
  } // increaseDeviceOutputChannelValue

  void DSIDSim::decreaseDeviceOutputChannelValue(const uint8_t _channel) {
  } // decreaseDeviceOutputChannelValue

  void DSIDSim::stopDeviceOutputChannelValue(const uint8_t _channel) {
  } // stopDeviceOutputChannelValue

  void DSIDSim::setValue(const uint8_t _value) {
    if(m_Enabled) {
      m_CurrentValue = int(_value);
    }
  } // setValue

  void DSIDSim::sensorPush(const uint8_t _sensorType, const uint16_t _sensorValue) {
  } // sensorPush

  uint16_t DSIDSim::getFunctionID() {
    if(m_ConfigParameter.has("functionID")) {
      int result = strToIntDef(m_ConfigParameter.get("functionID"), -1);
      if(result != -1) {
        return result;
      }
    }
    return 0;
  } // getFunctionID

  uint16_t DSIDSim::getProductID() {
    if(m_ConfigParameter.has("productID")) {
      int result = strToIntDef(m_ConfigParameter.get("productID"), -1);
      if(result != -1) {
        return result;
      }
    }
    return 0;
  } // getProductID

  uint16_t DSIDSim::getProductRevision() {
    if(m_ConfigParameter.has("productRevision")) {
      int result = strToIntDef(m_ConfigParameter.get("productRevision"), -1);
      if(result != -1) {
        return result;
      }
    }
    return 0;
  } // getProductRevision

  void DSIDSim::setConfigParameter(const std::string& _name, const std::string& _value) {
    m_ConfigParameter.set(_name, _value);
  } // setConfigParameter

  std::string DSIDSim::getConfigParameter(const std::string& _name) const {
    return m_ConfigParameter.get(_name, "");
  } // getConfigParameter


}

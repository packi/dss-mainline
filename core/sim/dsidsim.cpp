/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

namespace dss {
  //================================================== DSIDSim

  DSIDSim::DSIDSim(const DSModulatorSim& _simulator, const dsid_t _dsid, const devid_t _shortAddress)
  : DSIDInterface(_simulator, _dsid, _shortAddress),
    m_Enabled(true),
    m_CurrentValue(0),
    m_SimpleConsumption(25 * 1000)
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

  int DSIDSim::getConsumption() {
    return (int)((m_CurrentValue / 255.0) * m_SimpleConsumption) + (rand() % 100);
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
      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // undoScene

  void DSIDSim::increaseValue(const int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue += 10;
      m_CurrentValue = std::min((uint8_t)0xff, m_CurrentValue);
    }
  } // increaseValue

  void DSIDSim::decreaseValue(const int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue -= 10;
      m_CurrentValue = std::max((uint8_t)0, m_CurrentValue);
    }
  } // decreaseValue

  void DSIDSim::enable() {
    m_Enabled = true;
  } // enable

  void DSIDSim::disable() {
    m_Enabled = false;
  } // disable

  void DSIDSim::startDim(bool _directionUp, const int _parameterNr) {
    if(m_Enabled) {
      m_DimmingUp = _directionUp;
      m_Dimming = true;
      time(&m_DimmStartTime);
    }
  } // startDim

  void DSIDSim::endDim(const int _parameterNr) {
    if(m_Enabled) {
      time_t now;
      time(&now);
      if(m_DimmingUp) {
        m_CurrentValue = int(std::max(m_CurrentValue + difftime(m_DimmStartTime, now) * 5, 255.0));
      } else {
        m_CurrentValue = int(std::min(m_CurrentValue - difftime(m_DimmStartTime, now) * 5, 255.0));
      }
    }
  } // endDim

  void DSIDSim::setValue(const double _value, int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue = int(_value);
    }
  } // setValue

  double DSIDSim::getValue(int _parameterNr) const {
    return static_cast<double>(m_CurrentValue);
  } // getValue

  uint16_t DSIDSim::getFunctionID() {
    return FunctionIDDevice;
  } // getFunctionID

  void DSIDSim::setConfigParameter(const std::string& _name, const std::string& _value) {
    m_ConfigParameter.set(_name, _value);
  } // setConfigParameter

  std::string DSIDSim::getConfigParameter(const std::string& _name) const {
    return m_ConfigParameter.get(_name, "");
  } // getConfigParameter


}

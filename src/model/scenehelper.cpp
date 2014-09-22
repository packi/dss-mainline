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


#include "scenehelper.h"

#include <cassert>
#include <math.h>

namespace dss {

  //================================================== Utils

  unsigned int SceneHelper::getNextScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case SceneOff:
      return Scene1;
    case Scene1:
      return Scene2;
    case Scene2:
      return Scene3;
    case Scene3:
      return Scene4;
    case Scene4:
      return Scene2;
    case SceneOnE1:
      return Scene12;
    case Scene12:
      return Scene13;
    case Scene13:
      return Scene14;
    case Scene14:
      return Scene12;
    case SceneOnE2:
      return Scene22;
    case Scene22:
      return Scene23;
    case Scene23:
      return Scene24;
    case Scene24:
      return Scene22;
    case SceneOnE3:
      return Scene32;
    case Scene32:
      return Scene33;
    case Scene33:
      return Scene34;
    case Scene34:
      return Scene32;
    case SceneOnE4:
      return Scene42;
    case Scene42:
      return Scene43;
    case Scene43:
      return Scene44;
    case Scene44:
      return Scene42;

    default:
      return SceneOff;
    }
  } // getNextScene

  unsigned int SceneHelper::getPreviousScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case Scene1:
      return Scene4;
    case Scene2:
      return Scene1;
    case Scene3:
      return Scene2;
    case Scene4:
      return Scene3;
    case SceneOnE1:
      return Scene14;
    case Scene12:
      return SceneOnE1;
    case Scene13:
      return Scene12;
    case Scene14:
      return Scene13;
    case SceneOnE2:
      return Scene24;
    case Scene22:
      return SceneOnE2;
    case Scene23:
      return Scene22;
    case Scene24:
      return Scene23;
    case SceneOnE3:
      return Scene34;
    case Scene32:
      return SceneOnE3;
    case Scene33:
      return Scene32;
    case Scene34:
      return Scene33;
    case SceneOnE4:
      return Scene44;
    case Scene42:
      return SceneOnE4;
    case Scene43:
      return Scene42;
    case Scene44:
      return Scene43;

    default:
      return SceneOff;
    }
  } // getPreviousScene

  bool SceneHelper::m_Initialized = false;
  std::bitset<MaxSceneNumber + 1> SceneHelper::m_ZonesToIgnore;

  void SceneHelper::initialize() {
    m_Initialized = true;
    m_ZonesToIgnore.reset();
    m_ZonesToIgnore.set(SceneInc);
    m_ZonesToIgnore.set(SceneIncA1);
    m_ZonesToIgnore.set(SceneIncA2);
    m_ZonesToIgnore.set(SceneIncA3);
    m_ZonesToIgnore.set(SceneIncA4);
    m_ZonesToIgnore.set(SceneDec);
    m_ZonesToIgnore.set(SceneDecA1);
    m_ZonesToIgnore.set(SceneDecA2);
    m_ZonesToIgnore.set(SceneDecA3);
    m_ZonesToIgnore.set(SceneDecA4);
    m_ZonesToIgnore.set(SceneDimArea);
    m_ZonesToIgnore.set(SceneStop);
    m_ZonesToIgnore.set(SceneStopA1);
    m_ZonesToIgnore.set(SceneStopA2);
    m_ZonesToIgnore.set(SceneStopA3);
    m_ZonesToIgnore.set(SceneStopA4);
    m_ZonesToIgnore.set(SceneBell);
    m_ZonesToIgnore.set(SceneEnergyOverload);
    m_ZonesToIgnore.set(SceneLocalOff);
    m_ZonesToIgnore.set(SceneLocalOn);
  }

  bool SceneHelper::rememberScene(const unsigned int _scene) {
    if(!m_Initialized) {
      initialize();
      assert(m_Initialized);
    }
    if(_scene <= MaxSceneNumber) {
      return !m_ZonesToIgnore.test(_scene);
    }
    return false;
  } // rememberScene

  bool SceneHelper::isMultiTipSequence(const unsigned int _scene) {
    switch(_scene) {
    case Scene1:
    case SceneA11:
    case SceneA21:
    case SceneA31:
    case SceneA41:
    case Scene2:
    case Scene3:
    case Scene4:
    case SceneOnE1:
    case Scene12:
    case Scene13:
    case Scene14:
    case SceneOnE2:
    case Scene22:
    case Scene23:
    case Scene24:
    case SceneOnE3:
    case Scene32:
    case Scene33:
    case Scene34:
    case SceneOnE4:
    case Scene42:
    case Scene43:
    case Scene44:
      return true;
    case SceneOff:
    case SceneOffA1:
    case SceneOffA2:
    case SceneOffA3:
    case SceneOffA4:
    case SceneOffE1:
    case SceneOffE2:
    case SceneOffE3:
    case SceneOffE4:
      return true;
    }
    return false;
  } // isMultiTipSequence

  bool SceneHelper::isDimSequence(const unsigned int _scene) {
    switch(_scene) {
    case SceneInc:
    case SceneDec:
    case SceneDimArea:
    case SceneDecA1:
    case SceneIncA1:
    case SceneDecA2:
    case SceneIncA2:
    case SceneDecA3:
    case SceneIncA3:
    case SceneDecA4:
    case SceneIncA4:
      return true;
    }
    return false;
  }

  bool SceneHelper::isInRange(const int _sceneNumber, const int _zoneNumber) {
    bool aboveZero = (_sceneNumber >= 0);
    bool validForZone;
    if(_zoneNumber == 0) {
      validForZone = (_sceneNumber <= MaxSceneNumber);
    } else {
      validForZone = (_sceneNumber <= MaxSceneNumberOutsideZoneZero);
    }
    return aboveZero && validForZone;
  }

  uint64_t SceneHelper::getReachableScenesBitmapForButtonID(const int _buttonID) {
    const uint64_t Scene0Bitset = (1uLL << 0);
    const uint64_t Scene1Bitset = (1uLL << 1);
    const uint64_t Scene2Bitset = (1uLL << 2);
    const uint64_t Scene3Bitset = (1uLL << 3);
    const uint64_t Scene4Bitset = (1uLL << 4);
    const uint64_t Scene5Bitset = (1uLL << 5);
    const uint64_t Scene6Bitset = (1uLL << 6);
    const uint64_t Scene7Bitset = (1uLL << 7);
    const uint64_t Scene8Bitset = (1uLL << 8);
    const uint64_t Scene9Bitset = (1uLL << 9);
    const uint64_t Scene17Bitset = (1uLL << 17);
    const uint64_t Scene18Bitset = (1uLL << 18);
    const uint64_t Scene19Bitset = (1uLL << 19);
    const uint64_t Scene20Bitset = (1uLL << 20);
    const uint64_t Scene21Bitset = (1uLL << 21);
    const uint64_t Scene22Bitset = (1uLL << 22);
    const uint64_t Scene23Bitset = (1uLL << 23);
    const uint64_t Scene24Bitset = (1uLL << 24);
    const uint64_t Scene25Bitset = (1uLL << 25);
    const uint64_t Scene26Bitset = (1uLL << 26);
    const uint64_t Scene27Bitset = (1uLL << 27);
    const uint64_t Scene28Bitset = (1uLL << 28);
    const uint64_t Scene29Bitset = (1uLL << 29);
    const uint64_t Scene30Bitset = (1uLL << 30);
    const uint64_t Scene31Bitset = (1uLL << 31);
    const uint64_t Scene32Bitset = (1uLL << 32);
    const uint64_t Scene33Bitset = (1uLL << 33);
    const uint64_t Scene34Bitset = (1uLL << 34);
    const uint64_t Scene35Bitset = (1uLL << 35);
    const uint64_t Scene36Bitset = (1uLL << 36);
    const uint64_t Scene37Bitset = (1uLL << 37);
    const uint64_t Scene38Bitset = (1uLL << 38);
    const uint64_t Scene39Bitset = (1uLL << 39);

    const uint64_t reachableZonesPerButtonID[] = {
      Scene0Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 0
      Scene0Bitset | Scene1Bitset | Scene6Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 1
      Scene0Bitset | Scene2Bitset | Scene7Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 2
      Scene0Bitset | Scene3Bitset | Scene8Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 3
      Scene0Bitset | Scene4Bitset | Scene9Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 4
      Scene0Bitset | Scene5Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 5
      Scene0Bitset | Scene32Bitset | Scene33Bitset | Scene20Bitset | Scene21Bitset | Scene22Bitset, // 6
      Scene0Bitset | Scene34Bitset | Scene35Bitset | Scene23Bitset | Scene24Bitset | Scene25Bitset, // 7
      Scene0Bitset | Scene36Bitset | Scene37Bitset | Scene26Bitset | Scene27Bitset | Scene28Bitset, // 8
      Scene0Bitset | Scene38Bitset | Scene39Bitset | Scene29Bitset | Scene30Bitset | Scene31Bitset, // 9
      Scene0Bitset | Scene1Bitset | Scene6Bitset | Scene20Bitset | Scene21Bitset | Scene22Bitset, // 10
      Scene0Bitset | Scene2Bitset | Scene7Bitset | Scene23Bitset | Scene24Bitset | Scene25Bitset, // 11
      Scene0Bitset | Scene3Bitset | Scene8Bitset | Scene26Bitset | Scene27Bitset | Scene28Bitset, // 12
      Scene0Bitset | Scene4Bitset | Scene9Bitset | Scene29Bitset | Scene30Bitset | Scene31Bitset, // 13
    };
    if((_buttonID >= 0) && (_buttonID <= 13)) {
      return reachableZonesPerButtonID[_buttonID];
    }
    return 0uLL;
  } // getReachableScenesBitmapForButtonID

  SceneHelper::SceneOnState SceneHelper::isOnScene(const int _groupID, const unsigned int _scene) {
      SceneHelper::SceneOnState ret = SceneHelper::DontCare;

    // other groups not yet supported
    if ((_groupID != GroupIDYellow) && (_groupID != GroupIDHeating)) {
      return ret;
    }

    switch (_scene) {
      case SceneOff:
      case SceneDeepOff:
      case SceneSleeping:
      case SceneAutoOff:
      case SceneAutoStandBy:
      case SceneStandBy:
      case SceneAbsent:
      case SceneOffE1:
      case SceneOffE2:
      case SceneOffE3:
      case SceneOffE4:
          ret = SceneHelper::False;
          break; // false
      case Scene1:
      case Scene2:
      case Scene3:
      case Scene4:
      case SceneOnE1:
      case SceneOnE2:
      case SceneOnE3:
      case SceneOnE4:
      case Scene12:
      case Scene13:
      case Scene14:
      case Scene22:
      case Scene23:
      case Scene24:
      case Scene32:
      case Scene33:
      case Scene34:
      case Scene42:
      case Scene43:
      case Scene44:
      case ScenePanic:
      case SceneFire:
          ret = SceneHelper::True;
          break;
      case SceneAlarm:
      case SceneAlarm2:
      case SceneAlarm3:
      case SceneAlarm4:
      case SceneWindActive:
      case SceneWindInactive:
      case SceneRainActive:
      case SceneRainInactive:
      case SceneHailActive:
      case SceneHailInactive:
      case SceneRoomActivate:
      case ScenePresent:
      case SceneWakeUp:
      case SceneBell:
      case SceneBell2:
      case SceneBell3:
      case SceneBell4:
      default: // don't care
          break;
    }

    return ret;
  }

  double SceneHelper::sensorToFloat12(int _sensorType, int _sensorValue) {
    double convertedSensorValue;
    switch (_sensorType) {
    case SensorIDActivePower:
    case SensorIDOutputCurrent:
    case SensorIDPowerConsumptionVA:
      convertedSensorValue = (double) _sensorValue;
      break;
    case SensorIDElectricMeter:
      convertedSensorValue = (double) (_sensorValue * 0.01);
      break;
    case SensorIDOutputCurrentEx:
      convertedSensorValue = (double) (_sensorValue * 4);
      break;
    case SensorIDTemperatureIndoors:
    case SensorIDTemperatureOutdoors:
    case SensorIDRoomTemperatureSetpoint:
      convertedSensorValue = (double) ((_sensorValue * 0.1 / 4) - 273.15 + 230.0);
      break;
    case SensorIDBrightnessIndoors:
    case SensorIDBrightnessOutdoors:
    case SensorIDCO2Concentration:
      convertedSensorValue = (double) (pow(10, ((double) _sensorValue) / 800));
      break;
    case SensorIDHumidityIndoors:
    case SensorIDHumidityOutdoors:
      convertedSensorValue = (double) (_sensorValue * 0.1 / 4);
      break;
    case SensorIDWindSpeed:
      convertedSensorValue = (double) (_sensorValue * 0.1 / 4);
      break;
    case SensorIDRoomTemperatureControlVariable:
      convertedSensorValue = (double) (_sensorValue - 100);
      break;
    case SensorIDWindDirection:
    case SensorIDPrecipitation:
    default:
      convertedSensorValue = (double) _sensorValue;
      break;
    }
    return convertedSensorValue;
  } // sensorToFloat12

  int SceneHelper::sensorToSystem(int _sensorType, double _sensorValue) {
    int convertedSensorValue;
    switch (_sensorType) {
    case SensorIDActivePower:
    case SensorIDOutputCurrent:
    case SensorIDPowerConsumptionVA:
      convertedSensorValue = (int) _sensorValue;
      break;
    case SensorIDElectricMeter:
      convertedSensorValue = (int) (_sensorValue / 0.01);
      break;
    case SensorIDOutputCurrentEx:
      convertedSensorValue = (int) (_sensorValue / 4);
      break;
    case SensorIDTemperatureIndoors:
    case SensorIDTemperatureOutdoors:
    case SensorIDRoomTemperatureSetpoint:
      convertedSensorValue = (int) ((_sensorValue + 273.15 - 230.0) * 4 / 0.1);
      break;
    case SensorIDBrightnessIndoors:
    case SensorIDBrightnessOutdoors:
    case SensorIDCO2Concentration:
      convertedSensorValue = (int) (800 * log10(_sensorValue));
      break;
    case SensorIDHumidityIndoors:
    case SensorIDHumidityOutdoors:
      convertedSensorValue = (int) (_sensorValue * 4 / 0.1);
      break;
    case SensorIDWindSpeed:
      convertedSensorValue = (int) (_sensorValue * 4 / 0.1);
      break;
    case SensorIDRoomTemperatureControlVariable:
      convertedSensorValue = (int) (_sensorValue + 100) * 4;
      break;
    case SensorIDWindDirection:
    case SensorIDPrecipitation:
    default:
      convertedSensorValue = (int) _sensorValue;
      break;
    }
    return convertedSensorValue;
  } // sensorToSystem

  std::string SceneHelper::sensorName(const int _sensorType) {
    std::string _name;
    switch(_sensorType) {
      case SensorIDActivePower:
        _name = "Active Power"; break;
      case SensorIDOutputCurrent:
        _name = "Output Current"; break;
      case SensorIDElectricMeter:
        _name = "Electric Meter"; break;
      case SensorIDOutputCurrentEx:
        _name = "Output Current Ex"; break;
      case SensorIDPowerConsumptionVA:
        _name = "Active Power Ex"; break;
      case SensorIDTemperatureIndoors:
        _name = "Temperature Indoors"; break;
      case SensorIDTemperatureOutdoors:
        _name = "Temperature Outdoors"; break;
      case SensorIDBrightnessIndoors:
        _name = "Brightness Indoors"; break;
      case SensorIDBrightnessOutdoors:
        _name = "Brightness Outdoors"; break;
      case SensorIDHumidityIndoors:
        _name = "Humidity Indoors"; break;
      case SensorIDHumidityOutdoors:
        _name = "Humidity Outdoors"; break;
      case SensorIDAirPressure:
        _name = "Air Pressure"; break;
      case SensorIDWindSpeed:
        _name = "Wind Speed"; break;
      case SensorIDWindDirection:
        _name = "Wind Direction"; break;
      case SensorIDPrecipitation:
        _name = "Precipitation"; break;
      case SensorIDRoomTemperatureSetpoint:
        _name = "Nominal Temperature"; break;
      case SensorIDRoomTemperatureControlVariable:
        _name = "Temperature Control Value"; break;
      case SensorIDCO2Concentration:
        _name = "Carbon Dioxide Concentration"; break;
      case 255:
        _name = "Unknown Type"; break;
    default:
      break;
    }
    return _name;
  } // sensorName

}

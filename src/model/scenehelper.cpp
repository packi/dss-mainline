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


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


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
    m_ZonesToIgnore.set(SceneImpulse);
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
    // assume that no default zone/group state tracked
    SceneHelper::SceneOnState ret = SceneHelper::DontCare;

    // light has a zone/group state
    if (_groupID == GroupIDYellow) {
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
        default: // don't care
          break;
      }
    }

    // heating has a zone/group state
    if (_groupID == GroupIDHeating) {
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
          ret = SceneHelper::True;
          break;
        default: // don't care
          break;
      }
    }

    return ret;
  }

  double sensorToFloat12(SensorType _sensorType, int _sensorValue) {
    double convertedSensorValue;
    switch (_sensorType) {
    case SensorType::ActivePower:
    case SensorType::OutputCurrent:
    case SensorType::ActivePowerVA:
      convertedSensorValue = (double) _sensorValue;
      break;
    case SensorType::ElectricMeter:
      convertedSensorValue = (double) (_sensorValue * 0.01);
      convertedSensorValue= roundDigits(convertedSensorValue, 2);
      break;
    case SensorType::OutputCurrent16A:
      convertedSensorValue = (double) (_sensorValue * 4);
      break;
    case SensorType::TemperatureIndoors:
    case SensorType::TemperatureOutdoors:
    case SensorType::RoomTemperatureSetpoint:
      convertedSensorValue = (double) ((_sensorValue * 0.1 / 4) - 273.15 + 230.0);
      convertedSensorValue = roundDigits(convertedSensorValue, 3);
      break;
    case SensorType::BrightnessIndoors:
    case SensorType::BrightnessOutdoors:
    case SensorType::CO2Concentration:
    case SensorType::COConcentration:
      convertedSensorValue = (double) (pow(10, ((double) _sensorValue) / 800));
      convertedSensorValue = roundDigits(convertedSensorValue, 4);
      break;
    case SensorType::HumidityIndoors:
    case SensorType::HumidityOutdoors:
      convertedSensorValue = (double) (_sensorValue * 0.1 / 4);
      convertedSensorValue= roundDigits(convertedSensorValue, 3);
      break;
    case SensorType::WindSpeed:
    case SensorType::GustSpeed:
      convertedSensorValue = (double) (_sensorValue * 0.1 / 4);
      convertedSensorValue= roundDigits(convertedSensorValue, 3);
      break;
    case SensorType::SoundPressureLevel:
      convertedSensorValue = (double) (_sensorValue * 0.25 / 4);
      convertedSensorValue= roundDigits(convertedSensorValue, 3);
      break;
    case SensorType::RoomTemperatureControlVariable:
      convertedSensorValue = (double) (_sensorValue - 100);
      break;
    case SensorType::WindDirection:
    case SensorType::GustDirection:
      convertedSensorValue = (double) (_sensorValue * 0.5 / 4);
      convertedSensorValue= roundDigits(convertedSensorValue, 3);
      break;
    case SensorType::Precipitation:
      convertedSensorValue = (double) (_sensorValue * 0.1 / 4);
      convertedSensorValue= roundDigits(convertedSensorValue, 3);
      break;
    case SensorType::AirPressure:
      convertedSensorValue = (double) ((_sensorValue / 4) + 200);
      convertedSensorValue= roundDigits(convertedSensorValue, 3);
      break;
    default:
      convertedSensorValue = (double) _sensorValue;
      break;
    }
    return convertedSensorValue;
  } // sensorToFloat12

  int sensorToSystem(SensorType _sensorType, double _sensorValue) {
    int convertedSensorValue;
    switch (_sensorType) {
    case SensorType::ActivePower:
    case SensorType::OutputCurrent:
    case SensorType::ActivePowerVA:
      if (_sensorValue < 0 || _sensorValue > 4095) {
        throw SensorOutOfRangeException("Value must be in range [0..4095]");
      }
      convertedSensorValue = (int) (_sensorValue + 0.5);
      break;
    case SensorType::ElectricMeter:
      if (_sensorValue < 0 || _sensorValue > 40.95) {
        throw SensorOutOfRangeException("Value must be in range [0..40.95]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.005) / 0.01);
      break;
    case SensorType::OutputCurrent16A:
      if (_sensorValue < 0 || _sensorValue > 16380) {
        throw SensorOutOfRangeException("Value must be in range [0..16380]");
      }
      convertedSensorValue = (int) ((_sensorValue + 2) / 4);
      break;
    case SensorType::TemperatureIndoors:
    case SensorType::TemperatureOutdoors:
    case SensorType::RoomTemperatureSetpoint:
      if (_sensorValue < -43.15 || _sensorValue > 59.225) {
        throw SensorOutOfRangeException("Value must be in range [-43.15..59.225]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125 + 273.15 - 230.0) * 4 / 0.1);
      break;
    case SensorType::BrightnessIndoors:
    case SensorType::BrightnessOutdoors:
    case SensorType::CO2Concentration:
    case SensorType::COConcentration:
      if (_sensorValue < 1 || _sensorValue > 131446.795) {
        throw SensorOutOfRangeException("Value must be in range [1..131446.795]");
      }
      convertedSensorValue = (int) (800 * log10(_sensorValue));
      break;
    case SensorType::HumidityIndoors:
    case SensorType::HumidityOutdoors:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw SensorOutOfRangeException("Value must be in range [0..102.375]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.1);
      break;
    case SensorType::WindSpeed:
    case SensorType::GustSpeed:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw SensorOutOfRangeException("Value must be in range [0..102.375]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.1);
      break;
    case SensorType::SoundPressureLevel:
      if (_sensorValue < 0 || _sensorValue > 255.938) {
        throw SensorOutOfRangeException("Value must be in range [0..255.938]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.25);
      break;
    case SensorType::RoomTemperatureControlVariable:
      if (_sensorValue < -100 || _sensorValue > 100) {
        throw SensorOutOfRangeException("Value must be in range [-100..100]");
      }
      convertedSensorValue = (int) (_sensorValue + 0.5 + 100);
      break;
    case SensorType::WindDirection:
    case SensorType::GustDirection:
      if (_sensorValue < 0 || _sensorValue > 511.875) {
        throw SensorOutOfRangeException("Value must be in range [0..511.875]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0625) * 8);
      break;
    case SensorType::Precipitation:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw SensorOutOfRangeException("Value must be in range [0..102.375]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.1);
      break;
    case SensorType::AirPressure:
      if (_sensorValue < 200 || _sensorValue > 1223.75) {
        throw SensorOutOfRangeException("Value must be in range [200..1223.75]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.125 - 200) * 4);
      break;
    default:
      convertedSensorValue = (int) (_sensorValue + 0.5);
      break;
    }
    return convertedSensorValue;
  } // sensorToSystem

  uint8_t sensorToPrecision(SensorType _sensorType) {
    switch (_sensorType) {
      case SensorType::RoomTemperatureControlVariable:
        return 1;
      default:
        return 0;
    }
    return 0;
  }

  std::string sensorName(SensorType _sensorType) {
    std::string _name;
    switch(_sensorType) {
      case SensorType::ActivePower:
        _name = "Active Power"; break;
      case SensorType::OutputCurrent:
        _name = "Output Current"; break;
      case SensorType::ElectricMeter:
        _name = "Electric Meter"; break;
      case SensorType::OutputCurrent16A:
        _name = "Output Current Ex"; break;
      case SensorType::ActivePowerVA:
        _name = "Active Power Ex"; break;
      case SensorType::TemperatureIndoors:
        _name = "Temperature Indoors"; break;
      case SensorType::TemperatureOutdoors:
        _name = "Temperature Outdoors"; break;
      case SensorType::BrightnessIndoors:
        _name = "Brightness Indoors"; break;
      case SensorType::BrightnessOutdoors:
        _name = "Brightness Outdoors"; break;
      case SensorType::HumidityIndoors:
        _name = "Humidity Indoors"; break;
      case SensorType::HumidityOutdoors:
        _name = "Humidity Outdoors"; break;
      case SensorType::AirPressure:
        _name = "Air Pressure"; break;
      case SensorType::GustSpeed:
        _name = "Gust Speed"; break;
      case SensorType::GustDirection:
        _name = "Gust Direction"; break;
      case SensorType::WindSpeed:
        _name = "Wind Speed"; break;
      case SensorType::WindDirection:
        _name = "Wind Direction"; break;
      case SensorType::SoundPressureLevel:
        _name = "Sound Pressure Level"; break;
      case SensorType::Precipitation:
        _name = "Precipitation"; break;
      case SensorType::RoomTemperatureSetpoint:
        _name = "Nominal Temperature"; break;
      case SensorType::RoomTemperatureControlVariable:
        _name = "Temperature Control Value"; break;
      case SensorType::CO2Concentration:
        _name = "Carbon Dioxide Concentration"; break;
      case SensorType::COConcentration:
        _name = "Carbon Monoxide Concentration"; break;
      case SensorType::UnknownType:
        _name = "Unknown Type"; break;
      default:
        _name = "Invalid Type: " +  intToString(static_cast<int>(_sensorType)); break;
    }
    return _name;
  } // sensorName

}

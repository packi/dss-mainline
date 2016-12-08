/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#include "modelconst.h"

#include <math.h>

#include "base.h"

namespace dss {
  double sensorValueToDouble(SensorType _sensorType, int _sensorValue) {
    switch (_sensorType) {
    case SensorType::ActivePower:
    case SensorType::OutputCurrent:
    case SensorType::ActivePowerVA:
      return static_cast<double>(_sensorValue);
    case SensorType::ElectricMeter:
      return roundDigits(_sensorValue * 0.01, 2);
    case SensorType::OutputCurrent16A:
      return static_cast<double>(_sensorValue * 4);
    case SensorType::TemperatureIndoors:
    case SensorType::TemperatureOutdoors:
    case SensorType::RoomTemperatureSetpoint:
      return roundDigits(_sensorValue * 0.1 / 4 - 273.15 + 230.0, 3);
    case SensorType::BrightnessIndoors:
    case SensorType::BrightnessOutdoors:
    case SensorType::CO2Concentration:
    case SensorType::COConcentration:
      return roundDigits(pow(10, _sensorValue / 800.0), 4);
    case SensorType::HumidityIndoors:
    case SensorType::HumidityOutdoors:
      return roundDigits(_sensorValue * 0.1 / 4 , 3);
    case SensorType::WindSpeed:
    case SensorType::GustSpeed:
      return roundDigits(_sensorValue * 0.1 / 4, 3);
    case SensorType::SoundPressureLevel:
      return roundDigits(_sensorValue * 0.25 / 4, 3);
    case SensorType::RoomTemperatureControlVariable:
      return static_cast<double>(_sensorValue - 100);
    case SensorType::WindDirection:
    case SensorType::GustDirection:
      return roundDigits(_sensorValue * 0.5 / 4, 3);
    case SensorType::Precipitation:
      return roundDigits(_sensorValue * 0.1 / 4, 3);
    case SensorType::AirPressure:
      return roundDigits(_sensorValue / 4.0 + 200, 3);
    default:
      return static_cast<double>(_sensorValue);
    }
    return static_cast<double>(_sensorValue);
  } // sensorValueToDouble

  int doubleToSensorValue(SensorType _sensorType, double _sensorValue) {
    switch (_sensorType) {
    case SensorType::ActivePower:
    case SensorType::OutputCurrent:
    case SensorType::ActivePowerVA:
      if (_sensorValue < 0 || _sensorValue > 4095) {
        throw DSSException("Value must be in range [0..4095]");
      }
      return static_cast<int>(_sensorValue + 0.5);
    case SensorType::ElectricMeter:
      if (_sensorValue < 0 || _sensorValue > 40.95) {
        throw DSSException("Value must be in range [0..40.95]");
      }
      return static_cast<int>((_sensorValue + 0.005) / 0.01);
    case SensorType::OutputCurrent16A:
      if (_sensorValue < 0 || _sensorValue > 16380) {
        throw DSSException("Value must be in range [0..16380]");
      }
      return static_cast<int>((_sensorValue + 2) / 4);
    case SensorType::TemperatureIndoors:
    case SensorType::TemperatureOutdoors:
    case SensorType::RoomTemperatureSetpoint:
      if (_sensorValue < -43.15 || _sensorValue > 59.225) {
        throw DSSException("Value must be in range [-43.15..59.225]");
      }
      return static_cast<int>((_sensorValue + 0.0125 + 273.15 - 230.0) * 4 / 0.1);
    case SensorType::BrightnessIndoors:
    case SensorType::BrightnessOutdoors:
    case SensorType::CO2Concentration:
    case SensorType::COConcentration:
      if (_sensorValue < 1 || _sensorValue > 131446.795) {
        throw DSSException("Value must be in range [1..131446.795]");
      }
      return static_cast<int>(800 * log10(_sensorValue));
    case SensorType::HumidityIndoors:
    case SensorType::HumidityOutdoors:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw DSSException("Value must be in range [0..102.375]");
      }
      return static_cast<int>((_sensorValue + 0.0125) * 4 / 0.1);
    case SensorType::WindSpeed:
    case SensorType::GustSpeed:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw DSSException("Value must be in range [0..102.375]");
      }
      return static_cast<int>((_sensorValue + 0.0125) * 4 / 0.1);
    case SensorType::SoundPressureLevel:
      if (_sensorValue < 0 || _sensorValue > 255.938) {
        throw DSSException("Value must be in range [0..255.938]");
      }
      return static_cast<int>((_sensorValue + 0.0125) * 4 / 0.25);
    case SensorType::RoomTemperatureControlVariable:
      if (_sensorValue < -100 || _sensorValue > 100) {
        throw DSSException("Value must be in range [-100..100]");
      }
      return static_cast<int>(_sensorValue + 0.5 + 100);
    case SensorType::WindDirection:
    case SensorType::GustDirection:
      if (_sensorValue < 0 || _sensorValue > 511.875) {
        throw DSSException("Value must be in range [0..511.875]");
      }
      return static_cast<int>((_sensorValue + 0.0625) * 8);
    case SensorType::Precipitation:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw DSSException("Value must be in range [0..102.375]");
      }
      return static_cast<int>((_sensorValue + 0.0125) * 4 / 0.1);
    case SensorType::AirPressure:
      if (_sensorValue < 200 || _sensorValue > 1223.75) {
        throw DSSException("Value must be in range [200..1223.75]");
      }
      return static_cast<int>((_sensorValue + 0.125 - 200) * 4);
    default:
      return static_cast<int>(_sensorValue + 0.5);
    }
    return static_cast<int>(_sensorValue + 0.5);
  } // doubleToSensorValue

  uint8_t sensorTypeToPrecision(SensorType _sensorType) {
    switch (_sensorType) {
      case SensorType::RoomTemperatureControlVariable:
        return 1;
      default:
        return 0;
    }
    return 0;
  }

  std::string sensorTypeName(SensorType _sensorType) {
    switch(_sensorType) {
      case SensorType::ActivePower:
        return "Active Power";
      case SensorType::OutputCurrent:
        return "Output Current";
      case SensorType::ElectricMeter:
        return "Electric Meter";
      case SensorType::OutputCurrent16A:
        return "Output Current Ex";
      case SensorType::ActivePowerVA:
        return "Active Power Ex";
      case SensorType::TemperatureIndoors:
        return "Temperature Indoors";
      case SensorType::TemperatureOutdoors:
        return "Temperature Outdoors";
      case SensorType::BrightnessIndoors:
        return "Brightness Indoors";
      case SensorType::BrightnessOutdoors:
        return "Brightness Outdoors";
      case SensorType::HumidityIndoors:
        return "Humidity Indoors";
      case SensorType::HumidityOutdoors:
        return "Humidity Outdoors";
      case SensorType::AirPressure:
        return "Air Pressure";
      case SensorType::GustSpeed:
        return "Gust Speed";
      case SensorType::GustDirection:
        return "Gust Direction";
      case SensorType::WindSpeed:
        return "Wind Speed";
      case SensorType::WindDirection:
        return "Wind Direction";
      case SensorType::SoundPressureLevel:
        return "Sound Pressure Level";
      case SensorType::Precipitation:
        return "Precipitation";
      case SensorType::RoomTemperatureSetpoint:
        return "Nominal Temperature";
      case SensorType::RoomTemperatureControlVariable:
        return "Temperature Control Value";
      case SensorType::CO2Concentration:
        return "Carbon Dioxide Concentration";
      case SensorType::COConcentration:
        return "Carbon Monoxide Concentration";
      case SensorType::UnknownType:
        return "Unknown Type";
      default:
        return "Invalid Type: " +  intToString(static_cast<int>(_sensorType));
    }
    return "Invalid Type: " +  intToString(static_cast<int>(_sensorType));
  } // sensorTypeName

  std::ostream& operator<<(std::ostream& stream, SensorType type) {
    stream << dss::sensorTypeName(type);
    return stream;
  }
}

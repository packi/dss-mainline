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
  } // sensorValueToDouble

  int doubleToSensorValue(SensorType _sensorType, double _sensorValue) {
    int convertedSensorValue;
    switch (_sensorType) {
    case SensorType::ActivePower:
    case SensorType::OutputCurrent:
    case SensorType::ActivePowerVA:
      if (_sensorValue < 0 || _sensorValue > 4095) {
        throw DSSException("Value must be in range [0..4095]");
      }
      convertedSensorValue = (int) (_sensorValue + 0.5);
      break;
    case SensorType::ElectricMeter:
      if (_sensorValue < 0 || _sensorValue > 40.95) {
        throw DSSException("Value must be in range [0..40.95]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.005) / 0.01);
      break;
    case SensorType::OutputCurrent16A:
      if (_sensorValue < 0 || _sensorValue > 16380) {
        throw DSSException("Value must be in range [0..16380]");
      }
      convertedSensorValue = (int) ((_sensorValue + 2) / 4);
      break;
    case SensorType::TemperatureIndoors:
    case SensorType::TemperatureOutdoors:
    case SensorType::RoomTemperatureSetpoint:
      if (_sensorValue < -43.15 || _sensorValue > 59.225) {
        throw DSSException("Value must be in range [-43.15..59.225]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125 + 273.15 - 230.0) * 4 / 0.1);
      break;
    case SensorType::BrightnessIndoors:
    case SensorType::BrightnessOutdoors:
    case SensorType::CO2Concentration:
    case SensorType::COConcentration:
      if (_sensorValue < 1 || _sensorValue > 131446.795) {
        throw DSSException("Value must be in range [1..131446.795]");
      }
      convertedSensorValue = (int) (800 * log10(_sensorValue));
      break;
    case SensorType::HumidityIndoors:
    case SensorType::HumidityOutdoors:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw DSSException("Value must be in range [0..102.375]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.1);
      break;
    case SensorType::WindSpeed:
    case SensorType::GustSpeed:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw DSSException("Value must be in range [0..102.375]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.1);
      break;
    case SensorType::SoundPressureLevel:
      if (_sensorValue < 0 || _sensorValue > 255.938) {
        throw DSSException("Value must be in range [0..255.938]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.25);
      break;
    case SensorType::RoomTemperatureControlVariable:
      if (_sensorValue < -100 || _sensorValue > 100) {
        throw DSSException("Value must be in range [-100..100]");
      }
      convertedSensorValue = (int) (_sensorValue + 0.5 + 100);
      break;
    case SensorType::WindDirection:
    case SensorType::GustDirection:
      if (_sensorValue < 0 || _sensorValue > 511.875) {
        throw DSSException("Value must be in range [0..511.875]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0625) * 8);
      break;
    case SensorType::Precipitation:
      if (_sensorValue < 0 || _sensorValue > 102.375) {
        throw DSSException("Value must be in range [0..102.375]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.0125) * 4 / 0.1);
      break;
    case SensorType::AirPressure:
      if (_sensorValue < 200 || _sensorValue > 1223.75) {
        throw DSSException("Value must be in range [200..1223.75]");
      }
      convertedSensorValue = (int) ((_sensorValue + 0.125 - 200) * 4);
      break;
    default:
      convertedSensorValue = (int) (_sensorValue + 0.5);
      break;
    }
    return convertedSensorValue;
  } // doubleToSensorValue

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

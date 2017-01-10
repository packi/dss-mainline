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
#include <ds/str.h>

#include "base.h"
#include "exception.h"

namespace dss {
  double sensorValueToDouble(SensorType _sensorType, int _sensorValue) {
    switch (_sensorType) {
    case SensorType::Status:
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
    case SensorType::Reserved1:
    case SensorType::Reserved2:
    case SensorType::NotUsed:
    case SensorType::UnknownType:
      return static_cast<double>(_sensorValue);
    }
    return static_cast<double>(_sensorValue);
  } // sensorValueToDouble

  int doubleToSensorValue(SensorType _sensorType, double _sensorValue) {
    switch (_sensorType) {
    case SensorType::Status:
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
    case SensorType::Reserved1:
    case SensorType::Reserved2:
    case SensorType::NotUsed:
    case SensorType::UnknownType:
      return static_cast<int>(_sensorValue + 0.5);
    }
    return static_cast<int>(_sensorValue + 0.5);
  } // doubleToSensorValue

  uint8_t sensorTypeToPrecision(SensorType _sensorType) {
    switch (_sensorType) {
      case SensorType::Status:
      case SensorType::RoomTemperatureControlVariable:
        return 1;
      case SensorType::ActivePower:
      case SensorType::OutputCurrent:
      case SensorType::ElectricMeter:
      case SensorType::TemperatureIndoors:
      case SensorType::TemperatureOutdoors:
      case SensorType::BrightnessIndoors:
      case SensorType::BrightnessOutdoors:
      case SensorType::HumidityIndoors:
      case SensorType::HumidityOutdoors:
      case SensorType::AirPressure:
      case SensorType::GustSpeed:
      case SensorType::GustDirection:
      case SensorType::WindSpeed:
      case SensorType::WindDirection:
      case SensorType::Precipitation:
      case SensorType::CO2Concentration:
      case SensorType::COConcentration:
      case SensorType::SoundPressureLevel:
      case SensorType::RoomTemperatureSetpoint:
      case SensorType::Reserved1:
      case SensorType::Reserved2:
      case SensorType::OutputCurrent16A:
      case SensorType::ActivePowerVA:
      case SensorType::NotUsed:
      case SensorType::UnknownType:
        return 0;
    }
    return 0;
  }

  std::string sensorTypeName(SensorType _sensorType) {
    switch(_sensorType) {
      case SensorType::Status:
        return "Status";
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
      case SensorType::Reserved1:
        return "Reserved1";
      case SensorType::Reserved2:
        return "Reserved2";
      case SensorType::NotUsed:
        return "NotUsed";
    }
    return "Invalid Type: " +  intToString(static_cast<int>(_sensorType));
  } // sensorTypeName

  std::ostream& operator<<(std::ostream& stream, SensorType type) {
    stream << dss::sensorTypeName(type);
    return stream;
  }

  std::ostream& operator<<(std::ostream& stream, StatusBitType x) {
    return [&]()-> std::ostream& {
      switch (x) {
        case StatusBitType::MALFUNCTION: return stream << "malfunction";
        case StatusBitType::SERVICE: return stream << "service";
      }
      return stream << "unknown";
    }() << '(' << static_cast<int>(x) << ')';
  }

  std::ostream& operator<<(std::ostream& stream, BinaryInputType x) {
    switch (x) {
      case BinaryInputType::Presence: return stream << "presence";
      case BinaryInputType::RoomBrightness: return stream << "roomBrightness";
      case BinaryInputType::PresenceInDarkness: return stream << "presenceInDarkness";
      case BinaryInputType::TwilightExternal: return stream << "twilightExternal";
      case BinaryInputType::Movement: return stream << "movement";
      case BinaryInputType::MovementInDarkness: return stream << "movementInDarkness";
      case BinaryInputType::SmokeDetector: return stream << "smokeDetector";
      case BinaryInputType::WindDetector: return stream << "windDetector";
      case BinaryInputType::RainDetector: return stream << "rainDetector";
      case BinaryInputType::SunRadiation: return stream << "sunRadiation";
      case BinaryInputType::RoomThermostat: return stream << "roomThermostat";
      case BinaryInputType::BatteryLow: return stream << "batteryLow";
      case BinaryInputType::WindowContact: return stream << "windowContact";
      case BinaryInputType::DoorContact: return stream << "doorContact";
      case BinaryInputType::WindowTilt: return stream << "windowTilt";
      case BinaryInputType::GarageDoorContact: return stream << "garageDoorContact";
      case BinaryInputType::SunProtection: return stream << "sunProtection";
      case BinaryInputType::FrostDetector: return stream << "frostDetector";
      case BinaryInputType::HeatingSystem: return stream << "heatingSystem";
      case BinaryInputType::HeatingSystemMode: return stream << "heatingSystemMode";
      case BinaryInputType::PowerUp: return stream << "powerUp";
      case BinaryInputType::Malfunction: return stream << "malfunction";
      case BinaryInputType::Service: return stream << "service";
    }
    return stream << "unknown" << static_cast<int>(x);
  }

  boost::optional<StatusBitType> statusBitTypeForBinaryInputType(BinaryInputType x) {
    switch (x) {
      case BinaryInputType::Presence:
      case BinaryInputType::RoomBrightness:
      case BinaryInputType::PresenceInDarkness:
      case BinaryInputType::TwilightExternal:
      case BinaryInputType::Movement:
      case BinaryInputType::MovementInDarkness:
      case BinaryInputType::SmokeDetector:
      case BinaryInputType::WindDetector:
      case BinaryInputType::RainDetector:
      case BinaryInputType::SunRadiation:
      case BinaryInputType::RoomThermostat:
      case BinaryInputType::BatteryLow:
      case BinaryInputType::WindowContact:
      case BinaryInputType::DoorContact:
      case BinaryInputType::WindowTilt:
      case BinaryInputType::GarageDoorContact:
      case BinaryInputType::SunProtection:
      case BinaryInputType::FrostDetector:
      case BinaryInputType::HeatingSystem:
      case BinaryInputType::HeatingSystemMode:
      case BinaryInputType::PowerUp:
        return boost::none;
      case BinaryInputType::Malfunction:
        return StatusBitType::MALFUNCTION;
      case BinaryInputType::Service:
        return StatusBitType::SERVICE;
    }
    return boost::none;
  }

  std::string applicationTypeToString(ApplicationType type) {
    switch (type) {
      case ApplicationType::None:
        return "None";
      case ApplicationType::Lights:
        return "Lights";
      case ApplicationType::Blinds:
        return "Blinds";
      case ApplicationType::Heating:
        return "Heating";
      case ApplicationType::Audio:
        return "Audio";
      case ApplicationType::Video:
        return "Video";
      case ApplicationType::Cooling:
        return "Cooling";
      case ApplicationType::Ventilation:
        return "Ventilation";
      case ApplicationType::Window:
        return "Window";
      case ApplicationType::Curtains:
        return "Curtains";
      case ApplicationType::Temperature:
        return "Temperature";
      case ApplicationType::ApartmentVentilation:
        return "ApartmentVentilation";
      default:
        return "Not valid ApplicationType (" + intToString(static_cast<int>(type)) + ")";
    }
  }

  std::ostream& operator<<(std::ostream& stream, ApplicationType x) { return stream << applicationTypeToString(x); }

  int getApplicationTypeColor(ApplicationType x) {
    switch (x) {
      case ApplicationType::Lights:
        return ColorIDYellow;
      case ApplicationType::Blinds:
        return ColorIDGray;
      case ApplicationType::Heating:
      case ApplicationType::Cooling:
      case ApplicationType::Ventilation:
      case ApplicationType::Window:
      case ApplicationType::Temperature:
      case ApplicationType::ApartmentVentilation:
        return ColorIDBlue;
      case ApplicationType::Audio:
        return ColorIDCyan;
      case ApplicationType::Video:
        return ColorIDViolet;
      case ApplicationType::None:
      case ApplicationType::Curtains:
        return 0;
    }
    return 0;
  }

} // namespace dss

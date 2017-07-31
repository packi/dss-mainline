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
#include <ds/log.h>

#include "base.h"
#include "exception.h"
#include "model-features.h"

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

  boost::optional<const char*> statusFieldTypeName(StatusFieldType x) {
    switch (x) {
      case StatusFieldType::MALFUNCTION:
        return "malfunction";
      case StatusFieldType::SERVICE:
        return "service";
    }
    return boost::none;
  }

  boost::optional<StatusFieldType> statusFieldTypeFromName(const std::string& x) {
    if (x == "malfunction") {
      return StatusFieldType::MALFUNCTION;
    } else if (x == "service") {
      return StatusFieldType::SERVICE;
    }
    return boost::none;
  }

  std::ostream& operator<<(std::ostream& stream, StatusFieldType x) {
    return [&]()-> std::ostream& {
      if (auto&& name = statusFieldTypeName(x)) {
        return stream << *name;
      } else {
        return stream << "unknown";
      }
    }() << '(' << static_cast<int>(x) << ')';
  }

  boost::optional<const char*> statusFieldValueName(StatusFieldValue x) {
    switch (x) {
      case StatusFieldValue::INACTIVE:
        return "inactive";
      case StatusFieldValue::ACTIVE:
        return "active";
    }
    return boost::none;
  }

  boost::optional<StatusFieldValue> statusFieldValueFromName(const std::string& x) {
    if (x == "inactive") {
      return StatusFieldValue::INACTIVE;
    } else if (x == "active") {
      return StatusFieldValue::ACTIVE;
    }
    return boost::none;
  }

  std::ostream& operator<<(std::ostream& stream, StatusFieldValue x) {
    return [&]()-> std::ostream& {
      if (auto&& name = statusFieldValueName(x)) {
        return stream << *name;
      } else {
        return stream << "unknown";
      }
    }() << '(' << static_cast<int>(x) << ')';
  }

  std::ostream& operator<<(std::ostream& stream, BinaryInputType x) {
    return [&]()-> std::ostream& {
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
      return stream << "unknown";
    }() << '(' << static_cast<int>(x) << ')';
  }

  boost::optional<StatusFieldType> statusFieldTypeForBinaryInputType(BinaryInputType x) {
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
        return StatusFieldType::MALFUNCTION;
      case BinaryInputType::Service:
        return StatusFieldType::SERVICE;
    }
    return boost::none;
  }

  std::ostream& operator<<(std::ostream& stream, ApplicationType x) {
    return [&]()-> std::ostream& {
      switch (x) {
        case ApplicationType::None: return stream << "none";
        case ApplicationType::Lights: return stream << "lights";
        case ApplicationType::Blinds: return stream << "blinds";
        case ApplicationType::Heating: return stream << "heating";
        case ApplicationType::Audio: return stream << "audio";
        case ApplicationType::Video: return stream << "video";
        case ApplicationType::Joker: return stream << "joker";
        case ApplicationType::Cooling: return stream << "cooling";
        case ApplicationType::Ventilation: return stream << "ventilation";
        case ApplicationType::Window: return stream << "window";
        case ApplicationType::Recirculation: return stream << "recirculation";
        case ApplicationType::ControlTemperature: return stream << "controlTemperature";
        case ApplicationType::ApartmentVentilation: return stream << "apartmentVentilation";
        case ApplicationType::ApartmentRecirculation: return stream << "apartmentRecirculation";
      }
      return stream << "unknown";
    }() << '(' << static_cast<int>(x) << ')';
  }

  int getApplicationTypeColor(ApplicationType x) {
    switch (x) {
      case ApplicationType::Lights:
        return ColorIDYellow;
      case ApplicationType::Blinds:
        return ColorIDGray;
      case ApplicationType::Heating:
      case ApplicationType::Cooling:
      case ApplicationType::Ventilation:
      case ApplicationType::Recirculation:
      case ApplicationType::Window:
      case ApplicationType::ControlTemperature:
      case ApplicationType::ApartmentVentilation:
      case ApplicationType::ApartmentRecirculation:
        return ColorIDBlue;
      case ApplicationType::Audio:
        return ColorIDCyan;
      case ApplicationType::Video:
        return ColorIDViolet;
      case ApplicationType::Joker:
        return ColorIDBlack;
      case ApplicationType::None:
        return 0;
    }
    return 0;
  }

  RgbBitmask getApplicationTypeRgbBitmask(ApplicationType type) {
    switch (getApplicationTypeColor(type)) {
      case ColorIDYellow:
        return RgbBitmask::yellow;
      case ColorIDGray:
        return RgbBitmask::gray;
      case ColorIDBlue:
        return RgbBitmask::blue;
      case ColorIDCyan:
        return RgbBitmask::cyan;
      case ColorIDViolet:
        return RgbBitmask::magenta;
      case ColorIDRed:
        return RgbBitmask::red;
      case ColorIDGreen:
        return RgbBitmask::green;
      case ColorIDBlack:
        return RgbBitmask::gray;
      case ColorIDWhite:
        return RgbBitmask::gray;
    }
    return RgbBitmask::gray;
  }

  boost::optional<const char*> heatingControlModeName(HeatingControlMode x) {
    switch (x) {
      case HeatingControlMode::OFF:
        return "off";
      case HeatingControlMode::PID:
        return "control";
      case HeatingControlMode::ZONE_FOLLOWER:
        return "zoneFollower";
      case HeatingControlMode::FIXED:
        return "fixed";
      case HeatingControlMode::MANUAL:
        return "manual";
    }
    return boost::none;
  }

  boost::optional<HeatingControlMode> heatingControlModeFromName(const std::string& x) {
    if (x == "off") {
      return HeatingControlMode::OFF;
    } else if (x == "control") {
      return HeatingControlMode::PID;
    } else if (x == "zoneFollower") {
      return HeatingControlMode::ZONE_FOLLOWER;
    } else if (x == "fixed") {
      return HeatingControlMode::FIXED;
    } else if (x == "manual") {
      return HeatingControlMode::MANUAL;
    }

    return boost::none;
  }

  std::ostream& operator<<(std::ostream& stream, HeatingControlMode x) {
    return [&]()-> std::ostream& {
      if (auto&& name = heatingControlModeName(x)) {
        return stream << *name;
      } else {
        return stream << "unknown";
      }
    }() << '(' << static_cast<int>(x) << ')';
  }

  std::ostream& operator<<(std::ostream& stream, ButtonInputMode x) {
    return stream <<
           [&] {
             switch (x) {
               case ButtonInputMode::STANDARD:
                 return "standard";
               case ButtonInputMode::TURBO:
                 return "turbo";
               case ButtonInputMode::SWITCHED:
                 return "switched";
               case ButtonInputMode::TWO_WAY_DW_WITH_INPUT1:
                 return "twoWayDwWithInput1";
               case ButtonInputMode::TWO_WAY_DW_WITH_INPUT2:
                 return "twoWayDwWithInput2";
               case ButtonInputMode::TWO_WAY_DW_WITH_INPUT3:
                 return "twoWayDwWithInput3";
               case ButtonInputMode::TWO_WAY_DW_WITH_INPUT4:
                 return "twoWayDwWithInput4";
               case ButtonInputMode::TWO_WAY_UP_WITH_INPUT1:
                 return "twoWayUpWithInput1";
               case ButtonInputMode::TWO_WAY_UP_WITH_INPUT2:
                 return "twoWayUpWithInput2";
               case ButtonInputMode::TWO_WAY_UP_WITH_INPUT3:
                 return "twoWayUpWithInput3";
               case ButtonInputMode::TWO_WAY_UP_WITH_INPUT4:
                 return "twoWayUpWithInput4";
               case ButtonInputMode::TWO_WAY:
                 return "twoWay";
               case ButtonInputMode::ONE_WAY:
                 return "oneWay";
               case ButtonInputMode::AKM_STANDARD:
                 return "akmStandard";
               case ButtonInputMode::AKM_INVERTED:
                 return "akmInverted";
               case ButtonInputMode::AKM_ON_RISING_EDGE:
                 return "akmOnRisingEdge";
               case ButtonInputMode::AKM_ON_FALLING_EDGE:
                 return "akmOnFallingEdge";
               case ButtonInputMode::AKM_OFF_RISING_EDGE:
                 return "akmOffRisingEdge";
               case ButtonInputMode::AKM_OFF_FALLING_EDGE:
                 return "akmOffFallingEdge";
               case ButtonInputMode::AKM_RISING_EDGE:
                 return "akmRisingEdge";
               case ButtonInputMode::AKM_FALLING_EDGE:
                 return "akmFallingEdge";
               case ButtonInputMode::SDS_SLAVE_M1_M2:
                 return "sdsSlaveM1M2";
             }
             return "unknown";
           }() << '('
                  << static_cast<int>(x) << ')';
  }

  boost::optional<const char*> modelFeatureName(ModelFeatureId x) {
    switch (x) {
      case ModelFeatureId::dontcare:
        return "dontcare";
      case ModelFeatureId::blink:
        return "blink";
      case ModelFeatureId::ledauto:
        return "ledauto";
      case ModelFeatureId::leddark:
        return "leddark";
      case ModelFeatureId::transt:
        return "transt";
      case ModelFeatureId::outmode:
        return "outmode";
      case ModelFeatureId::outmodeswitch:
        return "outmodeswitch";
      case ModelFeatureId::outvalue8:
        return "outvalue8";
      case ModelFeatureId::pushbutton:
        return "pushbutton";
      case ModelFeatureId::pushbdevice:
        return "pushbdevice";
      case ModelFeatureId::pushbarea:
        return "pushbarea";
      case ModelFeatureId::pushbadvanced:
        return "pushbadvanced";
      case ModelFeatureId::pushbcombined:
        return "pushbcombined";
      case ModelFeatureId::shadeprops:
        return "shadeprops";
      case ModelFeatureId::shadeposition:
        return "shadeposition";
      case ModelFeatureId::motiontimefins:
        return "motiontimefins";
      case ModelFeatureId::optypeconfig:
        return "optypeconfig";
      case ModelFeatureId::shadebladeang:
        return "shadebladeang";
      case ModelFeatureId::highlevel:
        return "highlevel";
      case ModelFeatureId::consumption:
        return "consumption";
      case ModelFeatureId::jokerconfig:
        return "jokerconfig";
      case ModelFeatureId::akmsensor:
        return "akmsensor";
      case ModelFeatureId::akminput:
        return "akminput";
      case ModelFeatureId::akmdelay:
        return "akmdelay";
      case ModelFeatureId::twowayconfig:
        return "twowayconfig";
      case ModelFeatureId::outputchannels:
        return "outputchannels";
      case ModelFeatureId::heatinggroup:
        return "heatinggroup";
      case ModelFeatureId::heatingoutmode:
        return "heatingoutmode";
      case ModelFeatureId::heatingprops:
        return "heatingprops";
      case ModelFeatureId::pwmvalue:
        return "pwmvalue";
      case ModelFeatureId::valvetype:
        return "valvetype";
      case ModelFeatureId::extradimmer:
        return "extradimmer";
      case ModelFeatureId::umvrelay:
        return "umvrelay";
      case ModelFeatureId::blinkconfig:
        return "blinkconfig";
      case ModelFeatureId::impulseconfig:
        return "impulseconfig";
      case ModelFeatureId::umroutmode:
        return "umroutmode";
      case ModelFeatureId::pushbsensor:
        return "pushbsensor";
      case ModelFeatureId::locationconfig:
        return "locationconfig";
      case ModelFeatureId::windprotectionconfigawning:
        return "windprotectionconfigawning";
      case ModelFeatureId::windprotectionconfigblind:
        return "windprotectionconfigblind";
      case ModelFeatureId::outmodegeneric:
        return "outmodegeneric";
      case ModelFeatureId::outconfigswitch:
        return "outconfigswitch";
      case ModelFeatureId::temperatureoffset:
        return "temperatureoffset";
      case ModelFeatureId::apartmentapplication:
        return "apartmentapplication";
      case ModelFeatureId::ftwtempcontrolventilationselect:
        return "ftwtempcontrolventilationselect";
      case ModelFeatureId::ftwdisplaysettings:
        return "ftwdisplaysettings";
      case ModelFeatureId::ftwbacklighttimeout:
        return "ftwbacklighttimeout";
      case ModelFeatureId::ventconfig:
        return "ventconfig";
      case ModelFeatureId::fcu:
        return "fcu";
      case ModelFeatureId::pushbdisabled:
        return "pushbdisabled";
      case ModelFeatureId::consumptioneventled:
        return "consumptioneventled";
      case ModelFeatureId::consumptiontimer:
        return "consumptiontimer";
      case ModelFeatureId::jokertempcontrol:
        return "jokertempcontrol";
    }
    return boost::none;
  }

  boost::optional<ModelFeatureId> modelFeatureFromName(const std::string& x) {
    if (x == "dontcare") {
      return ModelFeatureId::dontcare;
    } else if (x == "blink") {
      return ModelFeatureId::blink;
    } else if (x == "ledauto") {
      return ModelFeatureId::ledauto;
    } else if (x == "leddark") {
      return ModelFeatureId::leddark;
    } else if (x == "transt") {
      return ModelFeatureId::transt;
    } else if (x == "outmode") {
      return ModelFeatureId::outmode;
    } else if (x == "outmodeswitch") {
      return ModelFeatureId::outmodeswitch;
    } else if (x == "outvalue8") {
      return ModelFeatureId::outvalue8;
    } else if (x == "pushbutton") {
      return ModelFeatureId::pushbutton;
    } else if (x == "pushbdevice") {
      return ModelFeatureId::pushbdevice;
    } else if (x == "pushbarea") {
      return ModelFeatureId::pushbarea;
    } else if (x == "pushbadvanced") {
      return ModelFeatureId::pushbadvanced;
    } else if (x == "pushbcombined") {
      return ModelFeatureId::pushbcombined;
    } else if (x == "shadeprops") {
      return ModelFeatureId::shadeprops;
    } else if (x == "shadeposition") {
      return ModelFeatureId::shadeposition;
    } else if (x == "motiontimefins") {
      return ModelFeatureId::motiontimefins;
    } else if (x == "optypeconfig") {
      return ModelFeatureId::optypeconfig;
    } else if (x == "shadebladeang") {
      return ModelFeatureId::shadebladeang;
    } else if (x == "highlevel") {
      return ModelFeatureId::highlevel;
    } else if (x == "consumption") {
      return ModelFeatureId::consumption;
    } else if (x == "jokerconfig") {
      return ModelFeatureId::jokerconfig;
    } else if (x == "akmsensor") {
      return ModelFeatureId::akmsensor;
    } else if (x == "akminput") {
      return ModelFeatureId::akminput;
    } else if (x == "akmdelay") {
      return ModelFeatureId::akmdelay;
    } else if (x == "twowayconfig") {
      return ModelFeatureId::twowayconfig;
    } else if (x == "outputchannels") {
      return ModelFeatureId::outputchannels;
    } else if (x == "heatinggroup") {
      return ModelFeatureId::heatinggroup;
    } else if (x == "heatingoutmode") {
      return ModelFeatureId::heatingoutmode;
    } else if (x == "heatingprops") {
      return ModelFeatureId::heatingprops;
    } else if (x == "pwmvalue") {
      return ModelFeatureId::pwmvalue;
    } else if (x == "valvetype") {
      return ModelFeatureId::valvetype;
    } else if (x == "extradimmer") {
      return ModelFeatureId::extradimmer;
    } else if (x == "umvrelay") {
      return ModelFeatureId::umvrelay;
    } else if (x == "blinkconfig") {
      return ModelFeatureId::blinkconfig;
    } else if (x == "impulseconfig") {
      return ModelFeatureId::impulseconfig;
    } else if (x == "umroutmode") {
      return ModelFeatureId::umroutmode;
    } else if (x == "pushbsensor") {
      return ModelFeatureId::pushbsensor;
    } else if (x == "locationconfig") {
      return ModelFeatureId::locationconfig;
    } else if (x == "windprotectionconfigblind") {
      return ModelFeatureId::windprotectionconfigblind;
    } else if (x == "windprotectionconfigawning") {
      return ModelFeatureId::windprotectionconfigawning;
    } else if (x == "outmodegeneric") {
      return ModelFeatureId::outmodegeneric;
    } else if (x == "outconfigswitch") {
      return ModelFeatureId::outconfigswitch;
    } else if (x == "temperatureoffset") {
      return ModelFeatureId::temperatureoffset;
    } else if (x == "apartmentapplication") {
      return ModelFeatureId::apartmentapplication;
    } else if (x == "ftwtempcontrolventilationselect") {
      return ModelFeatureId::ftwtempcontrolventilationselect;
    } else if (x == "ftwdisplaysettings") {
      return ModelFeatureId::ftwdisplaysettings;
    } else if (x == "ftwbacklighttimeout") {
      return ModelFeatureId::ftwbacklighttimeout;
    } else if (x == "ventconfig") {
      return ModelFeatureId::ventconfig;
    } else if (x == "fcu") {
      return ModelFeatureId::fcu;
    } else if (x == "pushbdisabled") {
      return ModelFeatureId::pushbdisabled;
    } else if (x == "consumptioneventled") {
      return ModelFeatureId::consumptioneventled;
    } else if (x == "consumptiontimer") {
      return ModelFeatureId::consumptiontimer;
    } else if (x == "jokertempcontrol") {
      return ModelFeatureId::jokertempcontrol;
    }

    return boost::none;
  }


  std::ostream& operator<<(std::ostream& stream, ModelFeatureId x) {
    return [&]()-> std::ostream& {
      if (auto&& name = modelFeatureName(x)) {
        return stream << *name;
      } else {
        return stream << "unknown";
      }
    }() << '(' << static_cast<int>(x) << ')';
  }

} // namespace dss

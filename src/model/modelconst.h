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

#ifndef DS485GROUPSCENECONST_H_
#define DS485GROUPSCENECONST_H_

#include <stdint.h>

namespace dss {

  // Scene constants for devices
  const uint8_t SceneOff = 0x00;
  const uint8_t SceneOffA1 = 0x01;
  const uint8_t SceneOffA2 = 0x02;
  const uint8_t SceneOffA3 = 0x03;
  const uint8_t SceneOffA4 = 0x04;
  const uint8_t Scene1 = 0x05;
  const uint8_t SceneA11 = 0x06;
  const uint8_t SceneA21 = 0x07;
  const uint8_t SceneA31 = 0x08;
  const uint8_t SceneA41 = 0x09;
  const uint8_t SceneDimArea = 0x0A;
  const uint8_t SceneDec = 0x0B;
  const uint8_t SceneInc = 0x0C;
  const uint8_t SceneMin = 0x0D;
  const uint8_t SceneMax = 0x0E;
  const uint8_t SceneStop = 0x0F;
  const uint8_t Scene2 = 0x11;
  const uint8_t Scene3 = 0x12;
  const uint8_t Scene4 = 0x13;
  const uint8_t Scene12 = 0x14;
  const uint8_t Scene13 = 0x15;
  const uint8_t Scene14 = 0x16;
  const uint8_t Scene22 = 0x17;
  const uint8_t Scene23 = 0x18;
  const uint8_t Scene24 = 0x19;
  const uint8_t Scene32 = 0x1A;
  const uint8_t Scene33 = 0x1B;
  const uint8_t Scene34 = 0x1C;
  const uint8_t Scene42 = 0x1D;
  const uint8_t Scene43 = 0x1E;
  const uint8_t Scene44 = 0x1F;
  const uint8_t SceneOffE1 = 0x20;
  const uint8_t SceneOnE1 = 0x21;
  const uint8_t SceneOffE2 = 0x22;
  const uint8_t SceneOnE2 = 0x23;
  const uint8_t SceneOffE3 = 0x24;
  const uint8_t SceneOnE3 = 0x25;
  const uint8_t SceneOffE4 = 0x26;
  const uint8_t SceneOnE4 = 0x27;
  const uint8_t SceneAutoOff = 0x28;
  const uint8_t SceneDecA1 = 0x2A;
  const uint8_t SceneIncA1 = 0x2B;
  const uint8_t SceneDecA2 = 0x2C;
  const uint8_t SceneIncA2 = 0x2D;
  const uint8_t SceneDecA3 = 0x2E;
  const uint8_t SceneIncA3 = 0x2F;
  const uint8_t SceneDecA4 = 0x30;
  const uint8_t SceneIncA4 = 0x32;
  const uint8_t SceneLocalOff = 0x32;
  const uint8_t SceneLocalOn = 0x33;
  const uint8_t SceneStopA1 = 0x34;
  const uint8_t SceneStopA2 = 0x35;
  const uint8_t SceneStopA3 = 0x36;
  const uint8_t SceneStopA4 = 0x37;

  // Apartment Scenes
  const uint8_t SceneAutoStandBy = 0x40;
  const uint8_t ScenePanic = 0x41;
  const uint8_t SceneEnergyOverload = 0x42;
  const uint8_t SceneStandBy = 0x43;
  const uint8_t SceneDeepOff = 0x44;
  const uint8_t SceneSleeping = 0x45;
  const uint8_t SceneWakeUp = 0x46;
  const uint8_t ScenePresent = 0x47;
  const uint8_t SceneAbsent = 0x48;
  const uint8_t SceneBell = 0x49;
  const uint8_t SceneAlarm = 0x4A;
  const uint8_t SceneRoomActivate = 0x4B;
  const uint8_t SceneFire = 0x4C;
  const uint8_t SceneSmoke = 0x4D;
  const uint8_t SceneWater = 0x4E;
  const uint8_t SceneGas = 0x4F;
  const uint8_t SceneBell2 = 0x50;
  const uint8_t SceneBell3 = 0x51;
  const uint8_t SceneBell4 = 0x52;
  const uint8_t SceneAlarm2 = 0x53;
  const uint8_t SceneAlarm3 = 0x54;
  const uint8_t SceneAlarm4 = 0x55;
  const uint8_t SceneWindActive = 0x56;
  const uint8_t SceneWindInactive = 0x57;
  const uint8_t SceneRainActive = 0x58;
  const uint8_t SceneRainInactive = 0x59;
  const uint8_t SceneHailActive = 0x5a;
  const uint8_t SceneHailInactive = 0x5b;

  const uint8_t MaxSceneNumber = 127;
  const uint8_t MaxSceneNumberOutsideZoneZero = 63;

  const uint16_t SceneFlagLocalOff = 0x0100;

  // Button input id"s
  const uint8_t ButtonId_Device = 0;
  const uint8_t ButtonId_Area1 = 1;
  const uint8_t ButtonId_Area2 = 2;
  const uint8_t ButtonId_Area3 = 3;
  const uint8_t ButtonId_Area4 = 4;
  const uint8_t ButtonId_Zone = 5;
  const uint8_t ButtonId_Zone_Extended1 = 6;
  const uint8_t ButtonId_Zone_Extended2 = 7;
  const uint8_t ButtonId_Zone_Extended3 = 8;
  const uint8_t ButtonId_Zone_Extended4 = 9;
  const uint8_t ButtonId_Area1_Extended = 10;
  const uint8_t ButtonId_Area2_Extended = 11;
  const uint8_t ButtonId_Area3_Extended = 12;
  const uint8_t ButtonId_Area4_Extended = 13;
  const uint8_t ButtonId_Apartment = 14;
  const uint8_t ButtonId_Generic = 15;

  // Sensor functions for Automated Devices
  const uint8_t SensorFunction_Generic = 0;
  const uint8_t SensorFunction_Generic1 = 1;
  const uint8_t SensorFunction_Generic2 = 2;
  const uint8_t SensorFunction_Generic3 = 3;
  const uint8_t SensorFunction_Generic4 = 4;
  const uint8_t SensorFunction_Generic5 = 5;
  const uint8_t SensorFunction_Generic6 = 6;
  const uint8_t SensorFunction_Generic7 = 7;
  const uint8_t SensorFunction_Generic8 = 8;
  const uint8_t SensorFunction_Generic9 = 9;
  const uint8_t SensorFunction_Generic10 = 10;
  const uint8_t SensorFunction_Generic11 = 11;

  // Sensor Type ID's
  const int SensorIDActivePower = 4;
  const int SensorIDOutputCurrent = 5;
  const int SensorIDElectricMeter = 6;
  const int SensorIDOutputCurrentEx = 64;
  const int SensorIDPowerConsumptionVA = 65;
  const int SensorIDTemperatureIndoors = 9;
  const int SensorIDTemperatureOutdoors = 10;
  const int SensorIDBrightnessIndoors = 11;
  const int SensorIDBrightnessOutdoors = 12;
  const int SensorIDHumidityIndoors = 13;
  const int SensorIDHumidityOutdoors = 14;
  const int SensorIDAirPressure = 15;
  const int SensorIDWindSpeed = 18;
  const int SensorIDWindDirection = 19;
  const int SensorIDPrecipitation = 20;
  const int SensorIDCO2Concentration = 21;
  const int SensorIDRoomTemperatureSetpoint = 50;
  const int SensorIDRoomTemperatureControlVariable = 51;
  const int SensorIDReserved1 = 61;
  const int SensorIDReserved2 = 62;
  const int SensorIDOutputCurrent16A = 64;
  const int SensorIDActivePowerVA = 65;
  const int SensorIDNotUsed = 253;

  const int SensorMaxLifeTime = 3600; /* 1h */

  // BinaryInput Type IDs
  const int BinaryInputIDPresence = 1;
  const int BinaryInputIDRoomBrightness = 2;
  const int BinaryInputIDPresenceInDarkness = 3;
  const int BinaryInputIDTwilightExternal = 4;
  const int BinaryInputIDMovement = 5;
  const int BinaryInputIDMovementInDarkness = 6;
  const int BinaryInputIDSmokeDetector = 7;
  const int BinaryInputIDWindDetector = 8;
  const int BinaryInputIDRainDetector = 9;
  const int BinaryInputIDSun = 10;
  const int BinaryInputIDRoomThermostat = 11;
  const int BinaryInputIDBatteryLow = 12;
  const int BinaryInputIDWindowContact = 13;
  const int BinaryInputIDDoorContact = 14;
  const int BinaryInputIDWindowTilt = 15;

  // Click type constants for devices
  const uint8_t ClickType1T = 0x00;     // Tipp 1
  const uint8_t ClickType2T = 0x01;     // Tipp 2
  const uint8_t ClickType3T = 0x02;     // Tipp 3
  const uint8_t ClickType4T = 0x03;     // Tipp 4
  const uint8_t ClickTypeHS = 0x04;     // Hold Start
  const uint8_t ClickTypeHR = 0x05;     // Hold Repeat
  const uint8_t ClickTypeHE = 0x06;     // Hold End
  const uint8_t ClickType1C = 0x07;     // Single Click
  const uint8_t ClickType2C = 0x08;     // Double Click
  const uint8_t ClickType3C = 0x09;     // Triple Click
  const uint8_t ClickType1P = 0x0a;     // Short-Long Click
  const uint8_t ClickTypeL0 = 0x0b;     // Local-On Click
  const uint8_t ClickTypeL1 = 0x0c;     // Local-Off Click
  const uint8_t ClickType2P = 0x0d;     // Short-Short-Long Click
  const uint8_t ClickTypeLS = 0x0e;     // Local-Stop Click
  const uint8_t ClickTypeRES = 0x0f;

  // Heating Control Mode
  const int HeatingControlModeIDOff = 0;
  const int HeatingControlModeIDPID = 1;
  const int HeatingControlModeIDZoneFollower = 2;
  const int HeatingControlModeIDFixed = 3;
  const int HeatingControlModeIDManual = 4;

  // Heating Control States
  const int HeatingControlStateIDInternal = 0;
  const int HeatingControlStateIDExternal = 1;
  const int HeatingControlStateIDExBackup = 2;
  const int HeatingControlStateIDEmergency = 3;

  // Heating Operation Mode
  const int HeatingOperationModeIDOff = 0;
  const int HeatingOperationModeIDComfort = 1;
  const int HeatingOperationModeIDEconomy = 2;
  const int HeatingOperationModeIDNotUsed = 3;
  const int HeatingOperationModeIDNight = 4;
  const int HeatingOperationModeInvalid = 0xff;

  const int HeatingOperationModeIDValveProtection = 31;

  // Color ID"s
  const int ColorIDYellow = 1;
  const int ColorIDGray = 2;
  const int ColorIDBlue = 3;
  const int ColorIDCyan = 4;
  const int ColorIDViolet = 5;
  const int ColorIDRed = 6;
  const int ColorIDGreen = 7;
  const int ColorIDBlack = 8;

  // Group ID"s
  const int GroupIDBroadcast = 0;
  const int GroupIDYellow = 1;
  const int GroupIDGray = 2;
  const int GroupIDHeating = 3;
  const int GroupIDCyan = 4;
  const int GroupIDViolet = 5;
  const int GroupIDRed = 6;
  const int GroupIDGreen = 7;
  const int GroupIDBlack = 8;
  const int GroupIDCooling = 9;
  const int GroupIDVentilation = 10;
  const int GroupIDWindow = 11;
  const int GroupIDCurtain = 12;
  const int GroupIDReserved3 = 13;
  const int GroupIDReserved4 = 14;
  const int GroupIDReserved5 = 15;
  const int GroupIDStandardMax = 15;
  const int GroupIDAppUserMin = 16;
  const int GroupIDAppUserMax = 39;
  const int GroupIDUserGroupStart = GroupIDAppUserMax + 1;
  const int GroupIDUserGroupEnd = 47;
  const int GroupIDControlGroupMin = 48;
  const int GroupIDControlTemperature = 48;
  const int GroupIDControlGroupMax = 55;
  const int GroupIDMax = 63;

  inline bool isDefaultGroup(int groupId) {
    return ((groupId >= GroupIDYellow && groupId <= GroupIDStandardMax)) ||
        (groupId == GroupIDControlTemperature);
  }

  inline bool isAppUserGroup(int groupId) {
    return (groupId >= GroupIDAppUserMin && groupId <= GroupIDAppUserMax);
  }

  inline bool isZoneUserGroup(int groupId) {
    return (groupId >= GroupIDUserGroupStart && groupId <= GroupIDUserGroupEnd);
  }

  inline bool isControlGroup(int groupId) {
    return (groupId >= GroupIDControlGroupMin && groupId <= GroupIDControlGroupMax);
  }

  const uint64_t DSIDHeader = 0x3504175FE0000000ll;

  const uint8_t CfgClassComm = 0x00;
  const uint8_t CfgClassDevice = 0x01;
  const uint8_t CfgClassPlatform = 0x02;
  const uint8_t CfgClassFunction = 0x03;
  const uint8_t CfgClassSensorEvent = 0x06;
  const uint8_t CfgClassRuntime = 0x40;
  const uint8_t CfgClassScene = 0x80;
  const uint8_t CfgClassSceneExtention = 0x81;
  const uint8_t CfgClassSceneAngle = 0x82;

  const uint8_t CfgFunction_Mode = 0;
  const uint8_t CfgFunction_ButtonMode = 1;
  const uint8_t CfgFunction_DimTime0 = 0x06;
  const uint8_t CfgFunction_FOffTime1 = 0x15;
  const uint8_t CfgFunction_FOnTime1 = 0x16;
  const uint8_t CfgFunction_FCount1 = 0x17;
  const uint8_t CfgFunction_LedConfig0 = 0x18;
  const uint8_t CfgFunction_LTMode = 0x1E;
  const uint8_t CfgFunction_LTTimeoutOff = 0x22;
  const uint8_t CfgFunction_LTTimeoutOn = 0x24;

  const uint8_t CfgFunction_Valve_EmergencyValue = 0x45;
  const uint8_t CfgFunction_Valve_EmergencyTimer = 0x46;
  const uint8_t CfgFunction_Valve_PwmPeriod = 0x48;
  const uint8_t CfgFunction_Valve_PwmMinValue = 0x4a;
  const uint8_t CfgFunction_Valve_PwmMaxValue = 0x4b;
  const uint8_t CfgFunction_Valve_PwmMinY = 0x4c;
  const uint8_t CfgFunction_Valve_PwmMaxY = 0x4d;
  const uint8_t CfgFunction_Valve_PwmConfig = 0x4e;
  const uint8_t CfgFunction_Valve_PwmOffset = 0x4f;
  const uint8_t CfgFunction_Valve_ProtectionTimer = 0x52;

  const uint8_t CfgFunction_UMV_Relay_Config = 0x52;

  const uint8_t CfgRuntime_Shade_Position = 0x02;
  const uint8_t CfgRuntime_Shade_PositionAngle = 0x04;
  const uint8_t CfgRuntime_Shade_PositionCurrent = 0x06;
  const uint8_t CfgRuntime_Valve_PwmValue = 0x04;
  const uint8_t CfgRuntime_Valve_PwmPriorityMode = 0x05;

  const uint8_t CfgDevice_SensorParameter = 0x20;
  const uint8_t CfgFSensorEvent_TableSize = 6;

  const uint16_t ProductID_KM_200 = 0x00C8;
  const uint16_t ProductID_SDS_200 = 0x18C8;
  const uint16_t ProductID_KL_200 = 0x0CC8;
  const uint16_t ProductID_KL_201 = 0x0CC9;
  const uint16_t ProductID_KL_210 = 0x0CD2;
  const uint16_t ProductID_KL_220 = 0x0CDC;
  const uint16_t ProductID_KL_230 = 0x0CE6;
  const uint16_t ProductID_UMV_210 = 0x2CD2;

  const uint8_t MinimumOutputChannelID = 1;
  const uint8_t MaximumOutputChannelID = 10;

  // function id numbers
  #define Fid_105_Mask_NumButtons 0x03
  #define Fid_105_Mask_VariableStandardGroup 0x04
  #define Fid_105_Mask_ExtraHardware 0x08
  #define Fid_105_Mask_OutputPresent 0x10
  #define Fid_105_Mask_VariableRamptime 0x20

} // namespace dss
#endif

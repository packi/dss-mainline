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

#ifndef DS485CONST_H_
#define DS485CONST_H_

#include <stdint.h>

namespace dss {

  // DS485 comands
  const uint8_t CommandSolicitSuccessorRequestLong = 0x00;
  const uint8_t CommandSolicitSuccessorRequest = 0x01;
  const uint8_t CommandSolicitSuccessorResponse = 0x02;
  const uint8_t CommandGetAddressRequest = 0x03;
  const uint8_t CommandGetAddressResponse = 0x04;
  const uint8_t CommandSetDeviceAddressRequest = 0x05;
  const uint8_t CommandSetDeviceAddressResponse = 0x06;
  const uint8_t CommandSetSuccessorAddressRequest = 0x07;
  const uint8_t CommandSetSuccessorAddressResponse = 0x08;
  const uint8_t CommandRequest = 0x09;
  const uint8_t CommandResponse = 0x0a;
  const uint8_t CommandAck = 0x0b;
  const uint8_t CommandBusy = 0x0c;
  const uint8_t CommandEvent = 0x0f;

  // dSS => dSM function constants
  const uint8_t FunctionModulatorAddZone = 0x00;
  const uint8_t FunctionModulatorRemoveZone = 0x01;
  const uint8_t FunctionModulatorRemoveAllZones = 0x02;
  const uint8_t FunctionModulatorCountDevInZone = 0x03;
  const uint8_t FunctionModulatorDevKeyInZone = 0x04;
  const uint8_t FunctionModulatorGetGroupsSize = 0x05;
  const uint8_t FunctionModulatorGetZonesSize  = 0x06;
  const uint8_t FunctionModulatorGetZoneIdForInd = 0x07;
  const uint8_t FunctionModulatorAddToGroup = 0x08;
  const uint8_t FunctionModulatorRemoveFromGroup = 0x09;
  const uint8_t FunctionGroupAddDeviceToGroup = 0x10;
  const uint8_t FunctionGroupRemoveDeviceFromGroup = 0x11;
  const uint8_t FunctionGroupGetDeviceCount = 0x12;
  const uint8_t FunctionGroupGetDevKeyForInd = 0x13;

  const uint8_t FunctionZoneGetGroupIdForInd = 0x17;
  const uint8_t FunctionGroupGetLastCalledScene = 0x18;

  const uint8_t FunctionGroupIncreaseValue = 0x20;
  const uint8_t FunctionGroupDecreaseValue = 0x21;
  const uint8_t FunctionGroupStartDimInc = 0x22;
  const uint8_t FunctionGroupStartDimDec = 0x23;
  const uint8_t FunctionGroupEndDim = 0x24;
  const uint8_t FunctionGroupCallScene = 0x25;
  const uint8_t FunctionGroupSaveScene = 0x26;
  const uint8_t FunctionGroupUndoScene = 0x27;
  const uint8_t FunctionGroupSetValue = 0x28;

  const uint8_t FunctionZoneAddDevice = 0x30;
  const uint8_t FunctionZoneRemoveDevice = 0x31;
  const uint8_t FunctionDeviceAddToGroup = 0x34;
  const uint8_t FunctionZoneRemoveAllDevicesFromZone = 0x35;

  const uint8_t FunctionDeviceCallScene = 0x42;
  const uint8_t FunctionDeviceSaveScene = 0x43;
  const uint8_t FunctionDeviceUndoScene = 0x44;

  const uint8_t FunctionDeviceStartDimInc = 0x45;
  const uint8_t FunctionDeviceStartDimDec = 0x46;
  const uint8_t FunctionDeviceEndDim = 0x47;


  const uint8_t FunctionDeviceSetParameterValue = 0x4B;
  const uint8_t FunctionDeviceIncreaseValue  = 0x4C;
  const uint8_t FunctionDeviceDecreaseValue  = 0x4D;
  const uint8_t FunctionDeviceSetValue  = 0x50;

  const uint8_t FunctionDeviceSetZoneID = 0x4F;

  const uint8_t FunctionDeviceGetOnOff = 0x61;
  const uint8_t FunctionDeviceGetParameterValue = 0x62;
  const uint8_t FunctionDeviceGetDSID = 0x65;
  const uint8_t FunctionDeviceGetFunctionID = 0x66;
  const uint8_t FunctionDeviceGetGroups = 0x67;
  const uint8_t FunctionDeviceGetName = 0x68;
  const uint8_t FunctionDeviceGetVersion = 0x69;
  
  const uint8_t FunctionGetTypeRequest = 0x90;
  const uint8_t FunctionModulatorGetDSID = 0x91;
  const uint8_t FunctionModulatorGetPowerConsumption = 0x94;
  const uint8_t FunctionModulatorGetEnergyMeterValue = 0x95;
  const uint8_t FunctionModulatorSetEnergyLevel = 0x96;
  const uint8_t FunctionModulatorGetEnergyLevel = 0x97;

  const uint8_t FunctionDeviceGetTransmissionQuality = 0x9f;

  const uint8_t FunctionMeterSynchronisation = 0xa0;
  
  const uint8_t FunctionDSLinkConfigWrite = 0xc0;
  const uint8_t FunctionDSLinkConfigRead = 0xc1;
  const uint8_t FunctionDSLinkSendDevice = 0xc2;
  const uint8_t FunctionDSLinkReceive = 0xc3;
  const uint8_t FunctionDSLinkSendGroup = 0xc4;
  const uint8_t DSLinkSendLastByte = 0x01;
  const uint8_t DSLinkSendWriteOnly = 0x02;
  
  const uint8_t EventNewDS485Device = 0x80;
  const uint8_t EventLostDS485Device = 0x81;
  const uint8_t EventDeviceReady = 0x82;
  const uint8_t EventDSLinkInterrupt = 0x85;

  // Scene constants for devices
  const uint8_t SceneOff = 0x00;
  const uint8_t Scene1 = 0x05;
  const uint8_t Scene2 = 0x11;
  const uint8_t Scene3 = 0x12;
  const uint8_t Scene4 = 0x13;
  const uint8_t SceneStandBy = 0x30;
  const uint8_t SceneDeepOff = 0x31;
  const uint8_t SceneInc = 0x0B;
  const uint8_t SceneDec = 0x0C;
  const uint8_t SceneMin = 0x0D;
  const uint8_t SceneMax = 0x0E;
  const uint8_t SceneStop = 0x0F;
  const uint8_t SceneBell = 0x29;
  const uint8_t SceneAlarm = 0x2A;
  const uint8_t ScenePanic = 0x2B;
  const uint8_t SceneEnergyOverload = 0x2C;
  const uint8_t SceneEnergyHigh = 0x2D;
  const uint8_t SceneEnergyMiddle = 0x2E;
  const uint8_t SceneEnergyLow = 0x2F;
  const uint8_t SceneEnergyClassA = 0x32;
  const uint8_t SceneEnergyClassB = 0x33;
  const uint8_t SceneEnergyClassC = 0x34;
  const uint8_t SceneEnergyClassD = 0x35;
  const uint8_t SceneEnergyClassE = 0x36;
  const uint8_t SceneEnergyClassF = 0x37;
  const uint8_t SceneLocalOff = 0x3D;
  const uint8_t SceneLocalOn = 0x3E;
  
  const uint8_t MaxSceneNumber = 0x3F;

  const uint16_t SceneFlagLocalOff = 0x0100;

  const int GroupIDBroadcast = 0;
  const int GroupIDYellow = 1;
  const int GroupIDGray = 2;
  const int GroupIDBlue = 3;
  const int GroupIDCyan = 4;
  const int GroupIDViolet = 5;
  const int GroupIDRed = 6;
  const int GroupIDGreen = 7;
  const int GroupIDBlack = 8;
  const int GroupIDWhite = 9;
  const int GroupIDDisplay = 10;
  const int GroupIDMax = GroupIDDisplay;

  const int FunctionIDDevice = 0;
  const int FunctionIDSwitch = 1;

  const uint64_t DSIDHeader = 0x3504175FE0000000ll;

  // error codes as received by the modulator
  const int kDS485Ok = 1;
  const int kDS485NoIDForIndexFound = -1;
  const int kDS485ZoneNotFound = -2;
  const int kDS485IndexOutOfBounds = -3;
  const int kDS485GroupIDOutOfBounds = -4;
  const int kDS485ZoneCannotBeDeleted = -5;
  const int kDS485OutOfMemory = -6;
  const int kDS485RoomAlreadyExists = -7;
  const int kDS485InvalidDeviceID = -8;
  const int kDS485CannotRemoveFromStandardGroup = -9;
  const int kDS485CannotDeleteStandardGroup = -10;
  const int kDS485DSIDIsNull = -11;
  const int kDS485ReservedRoomNumber = -15;
  const int kDS485DeviceNotFound = -16;
  const int kDS485GroupNotFound = -18;

}

#endif /*DS485CONST_H_*/

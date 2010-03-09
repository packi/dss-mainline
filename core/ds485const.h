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
  const uint8_t FunctionDSMeterAddZone = 0x00;
  const uint8_t FunctionDSMeterRemoveZone = 0x01;
  const uint8_t FunctionDSMeterRemoveAllZones = 0x02;
  const uint8_t FunctionDSMeterCountDevInZone = 0x03;
  const uint8_t FunctionDSMeterDevKeyInZone = 0x04;
  const uint8_t FunctionDSMeterGetGroupsSize = 0x05;
  const uint8_t FunctionDSMeterGetZonesSize  = 0x06;
  const uint8_t FunctionDSMeterGetZoneIdForInd = 0x07;
  const uint8_t FunctionDSMeterAddToGroup = 0x08;
  const uint8_t FunctionDSMeterRemoveFromGroup = 0x09;
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
  const uint8_t FunctionDeviceDisable = 0x48;
  const uint8_t FunctionDeviceEnable = 0x49;

  const uint8_t FunctionDeviceSetParameterValue = 0x4B;
  const uint8_t FunctionDeviceIncreaseValue  = 0x4C;
  const uint8_t FunctionDeviceDecreaseValue  = 0x4D;
  const uint8_t FunctionDeviceSetValue  = 0x50;

  const uint8_t FunctionBlink = 0x52;

  const uint8_t FunctionDeviceSetZoneID = 0x4F;

  const uint8_t FunctionDeviceGetOnOff = 0x61;
  const uint8_t FunctionDeviceGetParameterValue = 0x62;
  const uint8_t FunctionDeviceGetDSID = 0x65;
  const uint8_t FunctionDeviceGetFunctionID = 0x66;
  const uint8_t FunctionDeviceGetGroups = 0x67;
  const uint8_t FunctionDeviceGetVersion = 0x68;
  const uint8_t FunctionDeviceGetSensorValue = 0x69;
  
  const uint8_t FunctionGetTypeRequest = 0x90;
  const uint8_t FunctionDSMeterGetDSID = 0x91;
  const uint8_t FunctionDSMeterGetPowerConsumption = 0x94;
  const uint8_t FunctionDSMeterGetEnergyMeterValue = 0x95;
  const uint8_t FunctionDSMeterSetEnergyLevel = 0x96;
  const uint8_t FunctionDSMeterGetEnergyLevel = 0x97;

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
  const uint8_t EventDeviceReceivedTelegramShort = 0x83;
  const uint8_t EventDeviceReceivedTelegramLong = 0x84;
  const uint8_t EventDSLinkInterrupt = 0x85;

  const int FunctionIDDevice = 0;
  const int FunctionIDSwitch = 1;

  const uint64_t DSIDHeader = 0x3504175FE0000000ll;

  // error codes as received by the dsMeter
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

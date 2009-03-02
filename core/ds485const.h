#ifndef DS485CONST_H_
#define DS485CONST_H_

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

  const uint8_t FunctionGroupIncreaseValue = 0x20;
  const uint8_t FunctionGroupDecreaseValue = 0x21;
  const uint8_t FunctionGroupStartDimInc = 0x22;
  const uint8_t FunctionGroupStartDimDec = 0x23;
  const uint8_t FunctionGroupEndDim = 0x24;
  const uint8_t FunctionGroupCallScene = 0x25;
  const uint8_t FunctionGroupSaveScene = 0x26;
  const uint8_t FunctionGroupUndoScene = 0x27;

  const uint8_t FunctionZoneAddDevice = 0x30;
  const uint8_t FunctionZoneRemoveDevice = 0x31;

  const uint8_t FunctionDeviceCallScene = 0x42;
  const uint8_t FunctionDeviceSaveScene = 0x43;
  const uint8_t FunctionDeviceUndoScene = 0x44;

  const uint8_t FunctionDeviceStartDimInc = 0x45;
  const uint8_t FunctionDeviceStartDimDec = 0x46;
  const uint8_t FunctionDeviceEndDim = 0x47;


  const uint8_t FunctionDeviceSetParameterValue = 0x4B;
  const uint8_t FunctionDeviceIncreaseValue  = 0x4C;
  const uint8_t FunctionDeviceDecreaseValue  = 0x4D;

  const uint8_t FunctionDeviceSetZoneID = 0x4F;

  const uint8_t FunctionDeviceGetOnOff = 0x61;
  const uint8_t FunctionDeviceGetParameterValue = 0x62;
  const uint8_t FunctionDeviceGetDSID = 0x65;

  const uint8_t FunctionKeyPressed = 0x80;

  const uint8_t FunctionModulatorGetDSID = 0x91;
  const uint8_t FunctionModulatorGetPowerConsumption = 0x94;
  const uint8_t FunctionModulatorGetEnergyMeterValue = 0x95;

  const uint8_t FunctionDeviceGetFunctionID = 0x66;

  const uint8_t FunctionDeviceSubscribe = 0xF3;

  const uint8_t FunctionGetTypeRequest = 0x90;

  // Scene constants for devices
  const uint8_t SceneOff = 0x00;
  const uint8_t Scene1 = 0x05;
  const uint8_t Scene2 = 0x02;
  const uint8_t Scene3 = 0x03;
  const uint8_t Scene4 = 0x04;
  const uint8_t SceneStandBy = 0x30;
  const uint8_t SceneDeepOff = 0x31;
  const uint8_t SceneInc = 0x0B;
  const uint8_t SchenDec = 0x0C;
  const uint8_t SceneMin = 0x0D;
  const uint8_t SceneMax = 0x0E;
  const uint8_t SceneStop = 0x0F;

  const int GroupIDBroadcast = 0;
  const int GroupIDYellow = 1;
  const int GroupIDGray = 2;
  const int GroupIDBlue = 3;
  const int GroupIDCyan = 4;
  const int GroupIDRed = 5;
  const int GroupIDViolet = 6;
  const int GroupIDGreen = 7;
  const int GroupIDBlack = 8;
  const int GroupIDWhite = 9;
  const int GroupIDMax = GroupIDWhite;

  const int FunctionIDDevice = 0;
  const int FunctionIDSwitch = 1;
}

#endif /*DS485CONST_H_*/

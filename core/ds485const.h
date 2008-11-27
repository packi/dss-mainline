#ifndef DS485CONST_H_
#define DS485CONST_H_

namespace dss {

  // DS485 comands
  const uint8 CommandSolicitSuccessorRequest = 0x01;
  const uint8 CommandSolicitSuccessorResponse = 0x02;
  const uint8 CommandGetAddressRequest = 0x03;
  const uint8 CommandGetAddressResponse = 0x04;
  const uint8 CommandSetDeviceAddressRequest = 0x05;
  const uint8 CommandSetDeviceAddressResponse = 0x06;
  const uint8 CommandSetSuccessorAddressRequest = 0x07;
  const uint8 CommandSetSuccessorAddressResponse = 0x08;
  const uint8 CommandRequest = 0x09;
  const uint8 CommandResponse = 0x0a;
  const uint8 CommandAck = 0x0b;
  const uint8 CommandBusy = 0x0c;

  // dSS => dSM function constants
  const uint8 FunctionModulatorAddZone = 0x00;
  const uint8 FunctionModulatorRemoveZone = 0x01;
  const uint8 FunctionModulatorRemoveAllZones = 0x02;
  const uint8 FunctionModulatorCountDevInZone = 0x03;
  const uint8 FunctionModulatorDevKeyInZone = 0x04;
  const uint8 FunctionModulatorGetGroupsSize = 0x05;
  const uint8 FunctionModulatorGetZonesSize  = 0x06;
  const uint8 FunctionModulatorGetZoneIdForInd = 0x07;
  const uint8 FunctionModulatorAddToGroup = 0x08;
  const uint8 FunctionModulatorRemoveFromGroup = 0x09;
  const uint8 FunctionGroupAddDeviceToGroup = 0x10;
  const uint8 FunctionGroupRemoveDeviceFromGroup = 0x11;
  const uint8 FunctionGroupGetDeviceCount = 0x12;
  const uint8 FunctionGroupGetDevKeyForInd = 0x13;

  const uint8 FunctionZoneGetGroupIdForInd = 0x17;

  const uint8 FunctionGroupIncreaseValue = 0x20;
  const uint8 FunctionGroupDecreaseValue = 0x21;
  const uint8 FunctionGroupStartDimInc = 0x22;
  const uint8 FunctionGroupStartDimDec = 0x23;
  const uint8 FunctionGroupEndDim = 0x24;
  const uint8 FunctionGroupCallScene = 0x25;
  const uint8 FunctionGroupSaveScene = 0x26;
  const uint8 FunctionGroupUndoScene = 0x27;

  const uint8 FunctionZoneAddDevice = 0x30;
  const uint8 FunctionZoneRemoveDevice = 0x31;
  const uint8 FunctionZoneRemoveAllDevices = 0x35;

  const uint8 FunctionDeviceCallScene = 0x42;
  const uint8 FunctionDeviceSaveScene = 0x43;
  const uint8 FunctionDeviceUndoScene = 0x44;

  const uint8 FunctionDeviceSetParameterValue = 0x4B;
  const uint8 FunctionDeviceIncValue  = 0x4C;
  const uint8 FunctionDeviceDecValue  = 0x4D;

  const uint8 FunctionDeviceGetOnOff = 0x61;
  const uint8 FunctionDeviceGetParameterValue = 0x62;
  const uint8 FunctionDeviceGetDSID = 0x65;

  const uint8 FunctionKeyPressed = 0x80;

  const uint8 FunctionModulatorGetDSID = 0xF1;
  const uint8 FunctionModulatorGetPowerConsumption = 0x52;

  const uint8 FunctionDeviceGetFunctionID = 0x66;

  const uint8 FunctionDeviceSubscribe = 0xF3;

  const uint8 FunctionGetTypeRequest = 0xF0;

  // Scene constants for devices
  const uint8 SceneOff = 0x00;
  const uint8 Scene1 = 0x01;
  const uint8 Scene2 = 0x02;
  const uint8 Scene3 = 0x03;
  const uint8 Scene4 = 0x04;
  const uint8 SceneStandBy = 0x05;
  const uint8 SceneDeepOff = 0x06;

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

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
  const uint8 FunctionModulatorAddRoom = 0x00;
  const uint8 FunctionModulatorRemoveRoom = 0x01;
  const uint8 FunctionModulatorRemoveAllRooms = 0x02;
  const uint8 FunctionModulatorCountDevInRoom = 0x03;
  const uint8 FunctionModulatorDevKeyInRoom = 0x04;
  const uint8 FunctionModulatorGetGroupsSize = 0x05;
  const uint8 FunctionModulatorGetRoomsSize  = 0x06;
  const uint8 FunctionModulatorGetRoomIdForInd = 0x07;
  const uint8 FunctionModulatorAddToGroup = 0x08;
  const uint8 FunctionModulatorRemoveFromGroup = 0x09;
  const uint8 FunctionGroupAddDeviceToGroup = 0x10;
  const uint8 FunctionGroupRemoveDeviceFromGroup = 0x11;
  const uint8 FunctionGroupGetDeviceCount = 0x12;
  const uint8 FunctionGroupGetDevKeyForInd = 0x13;

  const uint8 FunctionRoomGetGroupIdForInd = 0x17;

  const uint8 FunctionDeviceCallScene = 0x42;
  const uint8 FunctionDeviceIncValue  = 0x40;
  const uint8 FunctionDeviceDecValue  = 0x41;

  const uint8 FunctionDeviceGetOnOff = 0x61;
  const uint8 FunctionDeviceGetDSID = 0x65;

  const uint8 FunctionModulatorGetDSID = 0xF1;

  const uint8 FunctionGetTypeRequest = 0xF0;

  // Scene constants for devices
  const uint8 SceneOff = 0x00;
  const uint8 Scene1 = 0x01;
  const uint8 Scene2 = 0x02;
  const uint8 Scene3 = 0x03;
  const uint8 Scene4 = 0x04;
  const uint8 SceneStandBy = 0x05;
  const uint8 SceneDeepOff = 0x06;

}

#endif /*DS485CONST_H_*/

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

  const uint8_t MaxSceneNumber = 255;
  const uint8_t MaxSceneNumberOutsideZoneZero = 63;

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
  const int GroupIDStandardMax = 15;
  const int GroupIDUserGroupStart = GroupIDStandardMax + 1;
  const int GroupIDMax = 63;

  const uint64_t DSIDHeader = 0x3504175FE0000000ll;
  const uint32_t SimulationPrefix = 0xFFC00000;

  const uint8_t CfgClassComm = 0x00;
  const uint8_t CfgClassDevice = 0x01;
  const uint8_t CfgClassPlatform = 0x02;
  const uint8_t CfgClassFunction = 0x03;
  const uint8_t CfgClassRuntime = 0x40;
  const uint8_t CfgClassScene = 0x80;

  const uint8_t CfgFunction_Mode = 0;
  const uint8_t CfgFunction_ButtonMode = 1;
  const uint8_t CfgFunction_DimTime0 = 0x06;
  const uint8_t CfgFunction_LedConfig0 = 0x18;
  const uint8_t CfgFunction_LTMode = 0x1E;

} // namespace dss
#endif

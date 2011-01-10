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
  const uint8_t Scene1 = 0x05;
  const uint8_t Scene2 = 0x11;
  const uint8_t Scene3 = 0x12;
  const uint8_t Scene4 = 0x13;
  const uint8_t SceneDec = 0x0B;
  const uint8_t SceneInc = 0x0C;
  const uint8_t SceneMin = 0x0D;
  const uint8_t SceneMax = 0x0E;
  const uint8_t SceneStop = 0x0F;
  const uint8_t SceneLocalOff = 0x3D;
  const uint8_t SceneLocalOn = 0x3E;

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

  const uint8_t MaxSceneNumber = 0xFF;

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

} // namespace dss
#endif

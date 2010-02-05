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

namespace dss {

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
  const int GroupIDStandardMax = GroupIDDisplay;
  const int GroupIDMax = 64;

} // namespace dss
#endif

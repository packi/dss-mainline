/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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


#include "scenehelper.h"

#include <cassert>

namespace dss {

  //================================================== Utils

  unsigned int SceneHelper::getNextScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case ScenePanic:
    case SceneStandBy:
    case SceneDeepOff:
    case SceneOff:
    case Scene1:
      return Scene2;
    case Scene2:
      return Scene3;
    case Scene3:
      return Scene4;
    case Scene4:
      return Scene2;
    default:
      return Scene1;
    }
  } // getNextScene

  unsigned int SceneHelper::getPreviousScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case ScenePanic:
    case SceneStandBy:
    case SceneDeepOff:
    case SceneOff:
    case Scene1:
      return Scene4;
    case Scene2:
      return Scene1;
    case Scene3:
      return Scene2;
    case Scene4:
      return Scene3;
    default:
      return Scene1;
    }
  } // getPreviousScene

  bool SceneHelper::m_Initialized = false;
  std::bitset<MaxSceneNumber + 1> SceneHelper::m_ZonesToIgnore;

  void SceneHelper::initialize() {
    m_Initialized = true;
    m_ZonesToIgnore.reset();
    m_ZonesToIgnore.set(SceneInc);
    m_ZonesToIgnore.set(SceneDec);
    m_ZonesToIgnore.set(SceneStop);
    m_ZonesToIgnore.set(SceneBell);
    m_ZonesToIgnore.set(SceneEnergyOverload);
    m_ZonesToIgnore.set(SceneLocalOff);
    m_ZonesToIgnore.set(SceneLocalOn);
  }

  bool SceneHelper::rememberScene(const unsigned int _scene) {
    if(!m_Initialized) {
      initialize();
      assert(m_Initialized);
    }
    if(_scene <= MaxSceneNumber) {
      return !m_ZonesToIgnore.test(_scene);
    }
    return false;
  } // rememberScene

  bool SceneHelper::isInRange(const int _sceneNumber, const int _zoneNumber) {
    bool aboveZero = (_sceneNumber >= 0);
    bool validForZone;
    if(_zoneNumber == 0) {
      validForZone = (_sceneNumber <= MaxSceneNumber);
    } else {
      validForZone = (_sceneNumber <= MaxSceneNumberOutsideZoneZero);
    }
    return aboveZero && validForZone;
  }

  uint64_t SceneHelper::getReachableScenesBitmapForButtonID(const int _buttonID) {
    const uint64_t Scene0Bitset = (1uLL << 0);
    const uint64_t Scene1Bitset = (1uLL << 1);
    const uint64_t Scene2Bitset = (1uLL << 2);
    const uint64_t Scene3Bitset = (1uLL << 3);
    const uint64_t Scene4Bitset = (1uLL << 4);
    const uint64_t Scene5Bitset = (1uLL << 5);
    const uint64_t Scene6Bitset = (1uLL << 6);
    const uint64_t Scene7Bitset = (1uLL << 7);
    const uint64_t Scene8Bitset = (1uLL << 8);
    const uint64_t Scene9Bitset = (1uLL << 9);
    const uint64_t Scene17Bitset = (1uLL << 17);
    const uint64_t Scene18Bitset = (1uLL << 18);
    const uint64_t Scene19Bitset = (1uLL << 19);
    const uint64_t Scene20Bitset = (1uLL << 20);
    const uint64_t Scene21Bitset = (1uLL << 21);
    const uint64_t Scene22Bitset = (1uLL << 22);
    const uint64_t Scene23Bitset = (1uLL << 23);
    const uint64_t Scene24Bitset = (1uLL << 24);
    const uint64_t Scene25Bitset = (1uLL << 25);
    const uint64_t Scene26Bitset = (1uLL << 26);
    const uint64_t Scene27Bitset = (1uLL << 27);
    const uint64_t Scene28Bitset = (1uLL << 28);
    const uint64_t Scene29Bitset = (1uLL << 29);
    const uint64_t Scene30Bitset = (1uLL << 30);
    const uint64_t Scene31Bitset = (1uLL << 31);
    const uint64_t Scene32Bitset = (1uLL << 32);
    const uint64_t Scene33Bitset = (1uLL << 33);
    const uint64_t Scene34Bitset = (1uLL << 34);
    const uint64_t Scene35Bitset = (1uLL << 35);
    const uint64_t Scene36Bitset = (1uLL << 36);
    const uint64_t Scene37Bitset = (1uLL << 37);
    const uint64_t Scene38Bitset = (1uLL << 38);
    const uint64_t Scene39Bitset = (1uLL << 39);

    const uint64_t reachableZonesPerButtonID[] = {
      Scene0Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 0
      Scene0Bitset | Scene1Bitset | Scene6Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 1
      Scene0Bitset | Scene2Bitset | Scene7Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 2
      Scene0Bitset | Scene3Bitset | Scene8Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 3
      Scene0Bitset | Scene4Bitset | Scene9Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 4
      Scene0Bitset | Scene5Bitset | Scene17Bitset | Scene18Bitset | Scene19Bitset, // 5
      Scene0Bitset | Scene32Bitset | Scene33Bitset | Scene20Bitset | Scene21Bitset | Scene22Bitset, // 6
      Scene0Bitset | Scene34Bitset | Scene35Bitset | Scene23Bitset | Scene24Bitset | Scene25Bitset, // 7
      Scene0Bitset | Scene36Bitset | Scene37Bitset | Scene26Bitset | Scene27Bitset | Scene28Bitset, // 8
      Scene0Bitset | Scene38Bitset | Scene39Bitset | Scene29Bitset | Scene30Bitset | Scene31Bitset, // 9
      Scene0Bitset | Scene1Bitset | Scene6Bitset | Scene20Bitset | Scene21Bitset | Scene22Bitset, // 10
      Scene0Bitset | Scene2Bitset | Scene7Bitset | Scene23Bitset | Scene24Bitset | Scene25Bitset, // 11
      Scene0Bitset | Scene3Bitset | Scene8Bitset | Scene26Bitset | Scene27Bitset | Scene28Bitset, // 12
      Scene0Bitset | Scene4Bitset | Scene9Bitset | Scene29Bitset | Scene30Bitset | Scene31Bitset, // 13
    };
    if((_buttonID >= 0) && (_buttonID <= 13)) {
      return reachableZonesPerButtonID[_buttonID];
    }
    return 0uLL;
  } // getReachableScenesBitmapForButtonID

}

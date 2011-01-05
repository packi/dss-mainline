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

#include "core/model/modelconst.h"

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
      return Scene4;
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
    m_ZonesToIgnore.set(SceneEnergyHigh);
    m_ZonesToIgnore.set(SceneEnergyMiddle);
    m_ZonesToIgnore.set(SceneEnergyLow);
    m_ZonesToIgnore.set(SceneEnergyClassA);
    m_ZonesToIgnore.set(SceneEnergyClassB);
    m_ZonesToIgnore.set(SceneEnergyClassC);
    m_ZonesToIgnore.set(SceneEnergyClassD);
    m_ZonesToIgnore.set(SceneEnergyClassE);
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

}

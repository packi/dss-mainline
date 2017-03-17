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


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "scenehelper.h"
#include "src/ds/str.h"
#include <cassert>
#include <map>

namespace dss {

  //================================================== Utils

  unsigned int SceneHelper::getNextScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case SceneOff:
      return Scene1;
    case Scene1:
      return Scene2;
    case Scene2:
      return Scene3;
    case Scene3:
      return Scene4;
    case Scene4:
      return Scene2;
    case SceneOnE1:
      return Scene12;
    case Scene12:
      return Scene13;
    case Scene13:
      return Scene14;
    case Scene14:
      return Scene12;
    case SceneOnE2:
      return Scene22;
    case Scene22:
      return Scene23;
    case Scene23:
      return Scene24;
    case Scene24:
      return Scene22;
    case SceneOnE3:
      return Scene32;
    case Scene32:
      return Scene33;
    case Scene33:
      return Scene34;
    case Scene34:
      return Scene32;
    case SceneOnE4:
      return Scene42;
    case Scene42:
      return Scene43;
    case Scene43:
      return Scene44;
    case Scene44:
      return Scene42;

    default:
      return SceneOff;
    }
  } // getNextScene

  unsigned int SceneHelper::getPreviousScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case Scene1:
      return Scene4;
    case Scene2:
      return Scene1;
    case Scene3:
      return Scene2;
    case Scene4:
      return Scene3;
    case SceneOnE1:
      return Scene14;
    case Scene12:
      return SceneOnE1;
    case Scene13:
      return Scene12;
    case Scene14:
      return Scene13;
    case SceneOnE2:
      return Scene24;
    case Scene22:
      return SceneOnE2;
    case Scene23:
      return Scene22;
    case Scene24:
      return Scene23;
    case SceneOnE3:
      return Scene34;
    case Scene32:
      return SceneOnE3;
    case Scene33:
      return Scene32;
    case Scene34:
      return Scene33;
    case SceneOnE4:
      return Scene44;
    case Scene42:
      return SceneOnE4;
    case Scene43:
      return Scene42;
    case Scene44:
      return Scene43;

    default:
      return SceneOff;
    }
  } // getPreviousScene

  bool SceneHelper::m_Initialized = false;
  std::bitset<MaxSceneNumber + 1> SceneHelper::m_ZonesToIgnore;

  void SceneHelper::initialize() {
    m_Initialized = true;
    m_ZonesToIgnore.reset();
    m_ZonesToIgnore.set(SceneInc);
    m_ZonesToIgnore.set(SceneIncA1);
    m_ZonesToIgnore.set(SceneIncA2);
    m_ZonesToIgnore.set(SceneIncA3);
    m_ZonesToIgnore.set(SceneIncA4);
    m_ZonesToIgnore.set(SceneDec);
    m_ZonesToIgnore.set(SceneDecA1);
    m_ZonesToIgnore.set(SceneDecA2);
    m_ZonesToIgnore.set(SceneDecA3);
    m_ZonesToIgnore.set(SceneDecA4);
    m_ZonesToIgnore.set(SceneDimArea);
    m_ZonesToIgnore.set(SceneStop);
    m_ZonesToIgnore.set(SceneStopA1);
    m_ZonesToIgnore.set(SceneStopA2);
    m_ZonesToIgnore.set(SceneStopA3);
    m_ZonesToIgnore.set(SceneStopA4);
    m_ZonesToIgnore.set(SceneBell);
    m_ZonesToIgnore.set(SceneEnergyOverload);
    m_ZonesToIgnore.set(SceneLocalOff);
    m_ZonesToIgnore.set(SceneLocalOn);
    m_ZonesToIgnore.set(SceneImpulse);
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

  bool SceneHelper::isMultiTipSequence(const unsigned int _scene) {
    switch(_scene) {
    case Scene1:
    case SceneA11:
    case SceneA21:
    case SceneA31:
    case SceneA41:
    case Scene2:
    case Scene3:
    case Scene4:
    case SceneOnE1:
    case Scene12:
    case Scene13:
    case Scene14:
    case SceneOnE2:
    case Scene22:
    case Scene23:
    case Scene24:
    case SceneOnE3:
    case Scene32:
    case Scene33:
    case Scene34:
    case SceneOnE4:
    case Scene42:
    case Scene43:
    case Scene44:
      return true;
    case SceneOff:
    case SceneOffA1:
    case SceneOffA2:
    case SceneOffA3:
    case SceneOffA4:
    case SceneOffE1:
    case SceneOffE2:
    case SceneOffE3:
    case SceneOffE4:
      return true;
    }
    return false;
  } // isMultiTipSequence

  bool SceneHelper::isDimSequence(const unsigned int _scene) {
    switch(_scene) {
    case SceneInc:
    case SceneDec:
    case SceneDimArea:
    case SceneDecA1:
    case SceneIncA1:
    case SceneDecA2:
    case SceneIncA2:
    case SceneDecA3:
    case SceneIncA3:
    case SceneDecA4:
    case SceneIncA4:
      return true;
    }
    return false;
  }

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

  SceneHelper::SceneOnState SceneHelper::isOnScene(const int _groupID, const unsigned int _scene) {
    // assume that no default zone/group state tracked
    SceneHelper::SceneOnState ret = SceneHelper::DontCare;

    // light has a zone/group state
    if (_groupID == GroupIDYellow) {
      switch (_scene) {
        case SceneOff:
        case SceneDeepOff:
        case SceneSleeping:
        case SceneAutoOff:
        case SceneAutoStandBy:
        case SceneStandBy:
        case SceneAbsent:
        case SceneOffE1:
        case SceneOffE2:
        case SceneOffE3:
        case SceneOffE4:
          ret = SceneHelper::False;
          break; // false
        case Scene1:
        case Scene2:
        case Scene3:
        case Scene4:
        case SceneOnE1:
        case SceneOnE2:
        case SceneOnE3:
        case SceneOnE4:
        case Scene12:
        case Scene13:
        case Scene14:
        case Scene22:
        case Scene23:
        case Scene24:
        case Scene32:
        case Scene33:
        case Scene34:
        case Scene42:
        case Scene43:
        case Scene44:
        case ScenePanic:
        case SceneFire:
          ret = SceneHelper::True;
          break;
        default: // don't care
          break;
      }
    }

    // heating has a zone/group state
    if (_groupID == GroupIDHeating) {
      switch (_scene) {
        case SceneOff:
        case SceneDeepOff:
        case SceneSleeping:
        case SceneAutoOff:
        case SceneAutoStandBy:
        case SceneStandBy:
        case SceneAbsent:
        case SceneOffE1:
        case SceneOffE2:
        case SceneOffE3:
        case SceneOffE4:
          ret = SceneHelper::False;
          break; // false
        case Scene1:
        case Scene2:
        case Scene3:
        case Scene4:
        case SceneOnE1:
        case SceneOnE2:
        case SceneOnE3:
        case SceneOnE4:
        case Scene12:
        case Scene13:
        case Scene14:
        case Scene22:
        case Scene23:
        case Scene24:
        case Scene32:
        case Scene33:
        case Scene34:
        case Scene42:
        case Scene43:
        case Scene44:
          ret = SceneHelper::True;
          break;
        default: // don't care
          break;
      }
    }

    return ret;
  }

  std::string SceneHelper::getSceneName(int sceneId, int groupId) {
    static const std::map<int, const char*> defaultSceneNames = {
        {SceneOff, "Off"}, {SceneOffA1, "Off-Area1"}, {SceneOffA2, "Off-Area2"}, {SceneOffA3, "Off-Area3"},
        {SceneOffA4, "Off-Area4"}, {Scene1, "Preset1"}, {SceneA11, "On-Area1"}, {SceneA21, "On-Area2"},
        {SceneA31, "On-Area3"}, {SceneA41, "On-Area4"}, {SceneDimArea, "Dimm-Area"}, {SceneDec, "Dec"},
        {SceneInc, "Inc"}, {SceneMin, "Min"}, {SceneMax, "Max"}, {SceneStop, "Stop"}, {Scene2, "Preset2"},
        {Scene3, "Preset3"}, {Scene4, "Preset4"}, {Scene12, "Preset12"}, {Scene13, "Preset13"}, {Scene14, "Preset14"},
        {Scene22, "Preset22"}, {Scene23, "Preset23"}, {Scene24, "Preset24"}, {Scene32, "Preset32"},
        {Scene33, "Preset33"}, {Scene34, "Preset34"}, {Scene42, "Preset42"}, {Scene43, "Preset43"},
        {Scene44, "Preset44"}, {SceneOffE1, "Off-Preset10"}, {SceneOnE1, "On-Preset11"}, {SceneOffE2, "Off-Preset20"},
        {SceneOnE2, "On-Preset21"}, {SceneOffE3, "Off-Preset30"}, {SceneOnE3, "On-Preset31"},
        {SceneOffE4, "Off-Preset40"}, {SceneOnE4, "On-Preset41"}, {SceneAutoOff, "Off-Automatic"},
        {SceneImpulse, "Impuls"}, {SceneDecA1, "Dec-Area1"}, {SceneIncA1, "Inc-Area1"}, {SceneDecA2, "Dec-Area2"},
        {SceneIncA2, "Inc-Area2"}, {SceneDecA3, "Dec-Area3"}, {SceneIncA3, "Inc-Area3"}, {SceneDecA4, "Dec-Area4"},
        {SceneIncA4, "Inc-Area4"}, {SceneLocalOff, "Off-Device"}, {SceneLocalOn, "On-Device"},
        {SceneStopA1, "Stop-Area1"}, {SceneStopA2, "Stop-Area2"}, {SceneStopA3, "Stop-Area3"},
        {SceneStopA4, "Stop-Area4"}, {SceneSunProtection, "Sunprotection"}, {SceneAutoStandBy, "AutoStandBy"},
        {ScenePanic, "Panic"}, {SceneEnergyOverload, "Overload"}, {SceneStandBy, "Zone-Standby"},
        {SceneDeepOff, "Zone-Deep-Off"}, {SceneSleeping, "Sleeping"}, {SceneWakeUp, "Wake-Up"},
        {ScenePresent, "Present"}, {SceneAbsent, "Absent"}, {SceneBell, "Bell"}, {SceneAlarm, "Alarm"},
        {SceneRoomActivate, "Zone-Activate"}, {SceneFire, "Fire"}, {SceneSmoke, "Smoke"}, {SceneWater, "Water"},
        {SceneGas, "Gas"}, {SceneBell2, "Bell2"}, {SceneBell3, "Bell3"}, {SceneBell4, "Bell4"}, {SceneAlarm2, "Alarm2"},
        {SceneAlarm3, "Alarm3"}, {SceneAlarm4, "Alarm4"}, {SceneWindActive, "Wind"}, {SceneWindInactive, "WindN"},
        {SceneRainActive, "Rain"}, {SceneRainInactive, "RainN"}, {SceneHailActive, "Hail"},
        {SceneHailInactive, "HailN"},
    };

    static const std::map<int, const char*> temperatureSceneNames = {
        {0, "Off"}, {1, "Comfort"}, {2, "Economy"}, {4, "Night"}, {5, "Holiday"}, {6, "Cooling"},
        {7, "CoolingOff"}, {8, "Manual"}, {29, "ClimateControlOn"}, {30, "ClimateControlOff"}, {31, "ValveProtection"},
    };

    static const std::map<int, const char*> ventilationSceneNames = {
        {SceneOff, "Off"}, {Scene1, "Level1"}, {Scene2, "Level2"}, {Scene3, "Level3"}, {Scene4, "Level4"},
        {SceneBoost, "Boost"},
    };

    if (groupId == GroupIDControlTemperature) {
      auto it = temperatureSceneNames.find(sceneId);
      if (it != temperatureSceneNames.end()) {
        return it->second;
      }
    } else if ((groupId == GroupIDVentilation) || (groupId == GroupIDGlobalAppDsVentilation)) {
      auto it = ventilationSceneNames.find(sceneId);
      if (it != ventilationSceneNames.end()) {
        return it->second;
      }
    } else {
      auto it = defaultSceneNames.find(sceneId);
      if (it != defaultSceneNames.end()) {
        return it->second;
      }
    }

    return "Unknown";
  }

  const std::vector<int>& SceneHelper::getAvailableScenes() {
    static const std::vector<int> allScenes = {
        // Scene constants for devices
        SceneOff, SceneOffA1, SceneOffA2, SceneOffA3, SceneOffA4, Scene1, SceneA11, SceneA21, SceneA31, SceneA41,
        SceneDimArea, SceneDec, SceneInc, SceneMin, SceneMax, SceneStop, Scene2, Scene3, Scene4, Scene12, Scene13,
        Scene14, Scene22, Scene23, Scene24, Scene32, Scene33, Scene34, Scene42, Scene43, Scene44, SceneOffE1, SceneOnE1,
        SceneOffE2, SceneOnE2, SceneOffE3, SceneBoost, SceneOnE3, SceneOffE4, SceneOnE4, SceneAutoOff, SceneImpulse,
        SceneDecA1, SceneIncA1, SceneDecA2, SceneIncA2, SceneDecA3, SceneIncA3, SceneDecA4, SceneIncA4, SceneLocalOff,
        SceneLocalOn, SceneStopA1, SceneStopA2, SceneStopA3, SceneStopA4, SceneSunProtection,

        // Apartment Scenes
        SceneAutoStandBy, ScenePanic, SceneEnergyOverload, SceneStandBy, SceneDeepOff, SceneSleeping, SceneWakeUp,
        ScenePresent, SceneAbsent, SceneBell, SceneAlarm, SceneRoomActivate, SceneFire, SceneSmoke, SceneWater,
        SceneGas, SceneBell2, SceneBell3, SceneBell4, SceneAlarm2, SceneAlarm3, SceneAlarm4, SceneWindActive,
        SceneWindInactive, SceneRainActive, SceneRainInactive, SceneHailActive, SceneHailInactive,
    };

    return allScenes;
  }

  const std::vector<int>& SceneHelper::getReachableScenes(int buttonId, int group) {

    //    ID  | Für alle Farben ausser...           | Für schwarz | For Ventilation     |
    //    -------------------------------------------------------------------------------
    //    0   | Gerätetaster (+ Stimmung 2-4)       |             |                     |
    //    1   | Bereichstaster 1 (+ Stimmung 2-4)   | Alarm       |                     |
    //    2   | Bereichstaster 2 (+ Stimmung 2-4)   | Panik       |                     |
    //    3   | Bereichstaster 3 (+ Stimmung 2-4)   | Gehen       |                     |
    //    4   | Bereichstaster 4 (+ Stimmung 2-4)   |             |                     |
    //    5   | Raumtaster (+ Stimmung 1-4)         | Klingeln    | Levels 1-4 + Boost  |
    //    6   | Raumtaster (+ Stimmung 10-14)       |             |                     |
    //    7   | Raumtaster (+ Stimmung 20-24)       |             |                     |
    //    8   | Raumtaster (+ Stimmung 30-34)       |             |                     |
    //    9   | Raumtaster (+ Stimmung 40-44)       |             |                     |
    //    10  | Bereichstaster 1 (+ Stimmung 12-14) |             |                     |
    //    11  | Bereichstaster 2 (+ Stimmung 22-24) |             |                     |
    //    12  | Bereichstaster 3 (+ Stimmung 32-34) |             |                     |
    //    13  | Bereichstaster 4 (+ Stimmung 42-44) |             |                     |
    //    14  | unbenutzt                           | unbenutzt   |                     |
    //    15  | unbenutzt                           | App-Taster  |                     |​
    //    -------------------------------------------------------------------------------
    static const std::map<int, std::vector<int> > defaultReachableScenes = {
        {0,{SceneOff, Scene2, Scene3, Scene4}},                           // 0
        {1,{SceneOff, SceneOffA1, SceneA11, Scene2, Scene3, Scene4}},     // 1
        {2,{SceneOff, SceneOffA2, SceneA21, Scene2, Scene3, Scene4}},     // 2
        {3,{SceneOff, SceneOffA3, SceneA31, Scene2, Scene3, Scene4}},     // 3
        {4,{SceneOff, SceneOffA4, SceneA41, Scene2, Scene3, Scene4}},     // 4
        {5,{SceneOff, Scene1, Scene2, Scene3, Scene4}},                   // 5
        {6,{SceneOff, SceneOffE1, SceneOnE1, Scene12, Scene13, Scene14}}, // 6
        {7,{SceneOff, SceneOffE2, SceneOnE2, Scene22, Scene23, Scene24}}, // 7
        {8,{SceneOff, SceneOffE3, SceneOnE3, Scene32, Scene33, Scene34}}, // 8
        {9,{SceneOff, SceneOffE4, SceneOnE4, Scene42, Scene43, Scene44}}, // 9
        {10,{SceneOff, SceneOffA1, SceneA11, Scene12, Scene13, Scene14}},  // 10
        {11,{SceneOff, SceneOffA2, SceneA21, Scene22, Scene23, Scene24}},  // 11
        {12,{SceneOff, SceneOffA3, SceneA31, Scene32, Scene33, Scene33}},  // 12
        {13,{SceneOff, SceneOffA4, SceneA41, Scene42, Scene43, Scene44}},  // 13
    };

    static const std::map<int,std::vector<int> > jokerReachableScenes = {
        {1,{SceneAlarm}},  // 1
        {2,{ScenePanic}},  // 2
        {3,{SceneAbsent}}, // 3
        {5,{SceneBell}},   // 5
    };

    static const std::map<int, std::vector<int> > ventilationReachableScenes = {
        {5, {SceneOff, Scene1, Scene2, Scene3, Scene4, SceneBoost}}, // 5
    };

    static const std::vector<int> reachableScenesEmpty;

    switch (group) {
      case GroupIDBlack: {
        auto it = jokerReachableScenes.find(buttonId);
        if (it != jokerReachableScenes.end()) {
          return it->second;
        }
        break;
      }
      case GroupIDVentilation:
      case GroupIDGlobalAppDsVentilation: {
        auto it = ventilationReachableScenes.find(buttonId);
        if (it != ventilationReachableScenes.end()) {
          return it->second;
        }
        break;
      }
      default: {
        auto it = defaultReachableScenes.find(buttonId);
        if (it != defaultReachableScenes.end()) {
          return it->second;
        }
        break;
      }
    }

    return reachableScenesEmpty;
  }
}

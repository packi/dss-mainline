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


#ifndef SCENEHELPER_H
#define SCENEHELPER_H

#include <bitset>

#include "src/model/modelconst.h"

namespace dss {

  /** Helper functions for scene management. */
  class SceneHelper {
  private:
    static std::bitset<MaxSceneNumber + 1> m_ZonesToIgnore;
    static bool m_Initialized;

    static void initialize();
  public:
    typedef enum {
      True,
      False,
      DontCare
    } SceneOnState;

    /** Returns wheter to remember a scene.
     * Certain scenes represent events thus they won't have to be remembered.
     */
    static bool rememberScene(const unsigned int _sceneID);
    /** Returns the next scene based on _currentScene.
     * From off to Scene1 -> Scene2 -> Scene3 -> Scene4 -> Scene1...
     * \param _currentScene Scene now active.
     */
    static unsigned int getNextScene(const unsigned int _currentScene);
    /** Returns the previous scene based on _currentScene.
     * From off to Scene1 -> Scene4 -> Scene3 -> Scene2 -> Scene1...
     * \param _currentScene Scene now active.
     */
    static unsigned int getPreviousScene(const unsigned int _currentScene);
    static bool isInRange(const int _sceneNumber, const int _zoneNumber);
    static bool isMultiTipSequence(const unsigned int _scene);
    static bool isDimSequence(const unsigned int _scene);
    static uint64_t getReachableScenesBitmapForButtonID(const int _buttonID);
    static SceneOnState isOnScene(const int _groupID, const unsigned int _scene);
  }; // SceneHelper

} // namespace dss

#endif // SCENEHELPER_H

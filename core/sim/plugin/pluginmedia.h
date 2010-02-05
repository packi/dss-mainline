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

#ifndef __PLUGINBASE_H_INCLUDED
#define __PLUGINBASE_H_INCLUDED

#include "pluginbase.h"
#include "../../model/modelconst.h"

#include <pthread.h>

void* handleBell(void* ptr);

class DSIDMedia : public DSID {
private:
  int m_LastScene;
  pthread_t m_ThreadHandle;
protected:
  virtual void sendCommand(const std::string& _command) = 0;
public:
    virtual void powerOn() = 0;
    virtual void powerOff() = 0;
    virtual void deepOff() = 0;
    virtual void nextSong() = 0;
    virtual void previousSong() = 0;
    virtual void volumeUp() = 0;
    virtual void volumeDown() = 0;

    virtual void callScene(const int _sceneNr) {
      // mask out local on/off
      int realScene = _sceneNr & 0x00ff;
      bool keepScene = (realScene != dss::SceneBell);
      if(realScene == dss::SceneDeepOff || realScene == dss::ScenePanic || realScene == dss::SceneAlarm) {
        deepOff();
      } else if(realScene == dss::SceneOff || realScene == dss::SceneMin || realScene == dss::SceneStandBy) {
        powerOff();
      } else if(realScene == dss::SceneMax) {
        powerOn();
      } else if(realScene == dss::SceneBell) {
        m_ThreadHandle = 0;
        pthread_create(&m_ThreadHandle, NULL, handleBell, this );
      } else if(realScene == dss::Scene1) {
        powerOn();
      } else if(realScene == dss::Scene2) {
        if(m_LastScene == dss::Scene3) {
          previousSong();
        } else {
          nextSong();
        }
      } else if(realScene == dss::Scene3) {
        if(m_LastScene == dss::Scene4) {
          previousSong();
        } else {
          nextSong();
        }
      } else if(realScene == dss::Scene4) {
        if(m_LastScene == dss::Scene2) {
          previousSong();
        } else {
          nextSong();
        }
      }
      if(keepScene) {
        m_LastScene = realScene;
      }
    }

    virtual void saveScene(const int _sceneNr) {
    }

    virtual void undoScene(const int _sceneNr) {
    }

    virtual void increaseValue(const int _parameterNr = -1) {
      volumeUp();
    }

    virtual void decreaseValue(const int _parameterNr = -1) {
      volumeDown();
    }

    virtual void enable() {
    }

    virtual void disable() {
    }

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) {
    }

    virtual void endDim(const int _parameterNr = -1) {
    }

    virtual void setValue(const double _value, int _parameterNr = -1) {
    }

    virtual double getValue(int _parameterNr = -1) const {
      return 0.0;
    }

    virtual void setConfigurationParameter(const std::string& _name, const std::string& _value) {
    }

    bool lastWasOff() {
       return   m_LastScene == dss::SceneDeepOff || m_LastScene == dss::SceneStandBy || m_LastScene == dss::SceneAlarm
             || m_LastScene == dss::SceneOff || m_LastScene == dss::SceneMin || m_LastScene == dss::ScenePanic;
     }

public:
  DSIDMedia() {
    m_LastScene = dss::SceneOff;
  }
}; // DSIDMedia


#endif // __PLUGINBASE_H_INCLUDED


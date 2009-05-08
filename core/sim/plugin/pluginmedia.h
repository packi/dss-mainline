
#ifndef __PLUGINBASE_H_INCLUDED
#define __PLUGINBASE_H_INCLUDED

#include "pluginbase.h"
#include "../../ds485const.h"

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

    virtual void CallScene(const int _sceneNr) {
      bool keepScene = _sceneNr != dss::SceneBell && _sceneNr != dss::SceneAlarm;
      if(_sceneNr == dss::SceneDeepOff || _sceneNr == dss::ScenePanic) {
        deepOff();
      } else if(_sceneNr == dss::SceneOff || _sceneNr == dss::SceneMin || _sceneNr == dss::SceneStandBy) {
        powerOff();
      } else if(_sceneNr == dss::SceneMax) {
        powerOn();
      } else if(_sceneNr == dss::SceneBell) {
        m_ThreadHandle = 0;
        pthread_create(&m_ThreadHandle, NULL, handleBell, this );
      } else if(_sceneNr == dss::Scene1) {
        powerOn();
      } else if(_sceneNr == dss::Scene2) {
        if(m_LastScene == dss::Scene3) {
          previousSong();
        } else {
          nextSong();
        }
      } else if(_sceneNr == dss::Scene3) {
        if(m_LastScene == dss::Scene4) {
          previousSong();
        } else {
          nextSong();
        }
      } else if(_sceneNr == dss::Scene4) {
        if(m_LastScene == dss::Scene2) {
          previousSong();
        } else {
          nextSong();
        }
      }
      if(keepScene) {
        m_LastScene = _sceneNr;
      }
    }

    virtual void SaveScene(const int _sceneNr) {
    }

    virtual void UndoScene(const int _sceneNr) {
    }

    virtual void IncreaseValue(const int _parameterNr = -1) {
      volumeUp();
    }

    virtual void DecreaseValue(const int _parameterNr = -1) {
      volumeDown();
    }

    virtual void Enable() {
    }

    virtual void Disable() {
    }

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) {
    }

    virtual void EndDim(const int _parameterNr = -1) {
    }

    virtual void SetValue(const double _value, int _parameterNr = -1) {
    }

    virtual double GetValue(int _parameterNr = -1) const {
      return 0.0;
    }

    virtual void SetConfigurationParameter(const std::string& _name, const std::string& _value) {
    }

    bool lastWasOff() {
       return   m_LastScene == dss::SceneDeepOff || m_LastScene == dss::SceneStandBy
             || m_LastScene == dss::SceneOff || m_LastScene == dss::SceneMin || m_LastScene == dss::ScenePanic;
     }

public:
  DSIDMedia() {
    m_LastScene = dss::SceneOff;
  }
}; // DSIDMedia


#endif // __PLUGINBASE_H_INCLUDED


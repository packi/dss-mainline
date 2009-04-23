#include <stdlib.h>

#include <iostream>
#include <map>

#include <pthread.h>

#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h>

#include "../../../core/sim/include/dsid_plugin.h"
#include "../../../core/ds485const.h"
#include "../../../core/base.h"

class DSID {
  private:
  public:
    virtual ~DSID() {}
    virtual void CallScene(const int _sceneNr) = 0;
    virtual void SaveScene(const int _sceneNr) = 0;
    virtual void UndoScene(const int _sceneNr) = 0;

    virtual void IncreaseValue(const int _parameterNr = -1) = 0;
    virtual void DecreaseValue(const int _parameterNr = -1) = 0;

    virtual void Enable() = 0;
    virtual void Disable() = 0;

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) = 0;
    virtual void EndDim(const int _parameterNr = -1) = 0;
    virtual void SetValue(const double _value, int _parameterNr = -1) = 0;

    virtual double GetValue(int _parameterNr = -1) const = 0;

    virtual void SetConfigurationParameter(const std::string& _name, const std::string& _value) = 0;
};

void* handleBell(void* ptr);

class DSIDVLCRemote : public DSID {
private:
  pthread_t m_ThreadHandle;
  int m_RemotePort;
  std::string m_RemoteHost;
public:
    DSIDVLCRemote() {
      lastScene = dss::SceneOff;
      m_RemotePort = 4212;
      m_RemoteHost = "127.0.0.1";
    }
    virtual ~DSIDVLCRemote() {}

    void SendCommand(const std::string& _command) {
      try
       {
               Poco::Net::SocketAddress sa(m_RemoteHost, m_RemotePort);
               Poco::Net::StreamSocket sock(sa);
               Poco::Net::SocketStream str(sock);

               std::cout << "before sending: " << _command << std::endl;
               str << _command << "\r\n" << std::flush;
               str << "logout\r\n" << std::flush;
               std::cout << "done sending" << std::endl;

               sock.close();
       }
       catch (Poco::Exception& exc)
       {
               std::cerr << "******** [vlc_remote.so] exception caught: " << exc.displayText() << std::endl;
       }
    }

    int lastScene;

    virtual void CallScene(const int _sceneNr) {
      std::cout << "call scene " << _sceneNr << " (last: " << lastScene << ")\n";
      if(_sceneNr == dss::SceneDeepOff) {
        SendCommand("stop");
      } else if(_sceneNr == dss::SceneOff || _sceneNr == dss::SceneMin) {
        SendCommand("pause");
      } else if(_sceneNr == dss::SceneMax) {
        SendCommand("pause");
      } else if(_sceneNr == dss::SceneBell) {
        m_ThreadHandle = 0;
        pthread_create(&m_ThreadHandle, NULL, handleBell, this );
      } else if(_sceneNr == dss::Scene1) {
        SendCommand("pause");
      } else if(_sceneNr == dss::Scene2) {
        if(lastScene == dss::Scene3) {
          SendCommand("prev");
        } else {
          SendCommand("next");
        }
      } else if(_sceneNr == dss::Scene3) {
        if(lastScene == dss::Scene4) {
          SendCommand("prev");
        } else {
          SendCommand("next");
        }
      } else if(_sceneNr == dss::Scene4) {
        if(lastScene == dss::Scene2) {
          SendCommand("prev");
        } else {
          SendCommand("next");
        }
      }
      lastScene = _sceneNr;
      std::cout << "end call scene" << std::endl;
    }

    virtual void SaveScene(const int _sceneNr) {
      std::cout << "save scene " << _sceneNr << "\n";
    }
    virtual void UndoScene(const int _sceneNr) {
      std::cout << "undo scene " << _sceneNr << "\n";
    }

    virtual void IncreaseValue(const int _parameterNr = -1) {
      std::cout << "increase value of parameter " << _parameterNr << "\n";
      SendCommand("volup");
    }

    virtual void DecreaseValue(const int _parameterNr = -1) {
      std::cout << "decrease value of parameter " << _parameterNr << "\n";
      SendCommand("voldown");
    }

    virtual void Enable() {
      std::cout << "enable\n";
    }
    virtual void Disable() {
      std::cout << "enable\n";
    }

    virtual void StartDim(bool _directionUp, const int _parameterNr = -1) {
      std::cout << "dim up: " << _directionUp << " of parameter " << _parameterNr << "\n";
    }
    virtual void EndDim(const int _parameterNr = -1) {
      std::cout << "end dim of parameter " << _parameterNr << "\n";
    }
    virtual void SetValue(const double _value, int _parameterNr = -1) {
      std::cout << "set value " << _value << " on parameter " << _parameterNr << "\n";
    }

    virtual double GetValue(int _parameterNr = -1) const {
      return 0.0;
    }

    virtual void SetConfigurationParameter(const std::string& _name, const std::string& _value) {
      if(_name == "port") {
        m_RemotePort = dss::StrToInt(_value);
      } else if(_name == "host") {
        m_RemoteHost = _value;
      }
    }
};

void* handleBell(void* ptr) {
  DSIDVLCRemote* remote = (DSIDVLCRemote*)ptr;
  remote->SendCommand("pause");
  sleep(5);
  remote->SendCommand("pause");
  return NULL;
}

class DSIDFactory {
private:
  std::map<int, DSID*> m_DSIDs;
  static DSIDFactory* m_Instance;
  int m_NextHandle;

private:
  DSIDFactory()
  : m_NextHandle(1)
  { }
public:
  static DSIDFactory& GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSIDFactory();
    }
    return *m_Instance;
  }

  int CreateDSID() {
    DSID* newDSID = new DSIDVLCRemote();
    m_DSIDs[m_NextHandle] = newDSID;
    return m_NextHandle++;
  }

  void DestroyDSID(const int _handle) {
  }

  DSID* GetDSID(const int _handle) {
    return m_DSIDs[_handle];
  }
};

DSIDFactory* DSIDFactory::m_Instance = 0;

int dsid_getversion() {
  return DSID_PLUGIN_API_VERSION;
} // dsid_getversion

const char* dsid_get_plugin_name() {
  return "example.vlc_remote";
} // dsid_get_plugin_name

int dsid_create_instance() {
  return DSIDFactory::GetInstance().CreateDSID();
}

void dsid_register_callback(void (*callback)(int _handle, int _eventID)) {
}

void dsid_destroy_instance(int _handle) {
  DSIDFactory::GetInstance().DestroyDSID(_handle);
}

double get_value(int _handle, int _parameterNumber) {
  std::cout << "get_value" << std::endl;
  return DSIDFactory::GetInstance().GetDSID(_handle)->GetValue(_parameterNumber);
} // get_value

void set_value(int _handle, int _parameterNumber, double _value) {
  DSIDFactory::GetInstance().GetDSID(_handle)->SetValue(_value, _parameterNumber);
} // set_value

void increase_value(int _handle, int _parameterNumber) {
  DSIDFactory::GetInstance().GetDSID(_handle)->IncreaseValue(_parameterNumber);
} // increase_value

void decrease_value(int _handle, int _parameterNumber) {
  DSIDFactory::GetInstance().GetDSID(_handle)->DecreaseValue(_parameterNumber);
} // decrease_value

void call_scene(int _handle, int _sceneNr) {
  DSIDFactory::GetInstance().GetDSID(_handle)->CallScene(_sceneNr);
} // call_scene

void save_scene(int _handle, int _sceneNr) {
  DSIDFactory::GetInstance().GetDSID(_handle)->SaveScene(_sceneNr);
} // save_scene

void undo_scene(int _handle, int _sceneNr) {
  DSIDFactory::GetInstance().GetDSID(_handle)->UndoScene(_sceneNr);
} // undo_scene

void enable(int _handle) {
  DSIDFactory::GetInstance().GetDSID(_handle)->Enable();
} // enable

void disable(int _handle) {
  DSIDFactory::GetInstance().GetDSID(_handle)->Disable();
} // disable

void get_group(int _handle) {
} // get_group

void start_dim(int _handle, int _directionUp, int _parameterNumber) {
} // start_dim

void end_dim(int _handle, int _parameterNumber) {
} // end_dim

int get_group_id(int _handle) {
  return -1;
} // get_group_id

int get_function_id(int _handle) {
  return -1;
} // get_function_id

const char* get_parameter_name(int _handle, int _parameterNumber) {
  return NULL;
} // get_parameter_name

void set_configuration_parameter(int _handle, const char* _name, const char* _value) {
  DSIDFactory::GetInstance().GetDSID(_handle)->SetConfigurationParameter(_name, _value);
}

static struct dsid_interface intf_description = {
  &get_value,
  &set_value,
  &increase_value,
  &decrease_value,
  &call_scene,
  &save_scene,
  &undo_scene,
  &enable,
  &disable,
  &get_group,

  &start_dim,
  &end_dim,
  &get_group_id,
  &get_function_id,

  &get_parameter_name,

  &set_configuration_parameter,
};


struct dsid_interface* dsid_get_interface() {
  return &intf_description;
}

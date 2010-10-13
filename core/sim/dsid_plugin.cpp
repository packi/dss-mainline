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

#include "dsid_plugin.h"

#include <dlfcn.h>

#include "include/dsid_plugin.h"

namespace dss {

  //================================================== DSIDPlugin

  class DSIDPlugin : public DSIDInterface {
  private:
    void* m_SOHandle;
    const int m_Handle;
    struct dsid_interface* m_Interface;
  public:
    DSIDPlugin(const DSMeterSim& _simulator, const dss_dsid_t _dsid, const devid_t _shortAddress, void* _soHandle, const int _handle)
    : DSIDInterface(_simulator, _dsid, _shortAddress),
      m_SOHandle(_soHandle),
      m_Handle(_handle)
    {
      struct dsid_interface* (*get_interface)();
      get_interface = (struct dsid_interface*(*)())dlsym(m_SOHandle, "dsid_get_interface");
      char* error;
      if((error = dlerror()) != NULL) {
        Logger::getInstance()->log("sim: error getting interface");
      }

      m_Interface = (*get_interface)();
      if(m_Interface == NULL) {
        Logger::getInstance()->log("sim: got a null interface");
      }
    }

    virtual int getConsumption() {
      return 0;
    }

    virtual void callScene(const int _sceneNr) {
      if(m_Interface->call_scene != NULL) {
        (*m_Interface->call_scene)(m_Handle, _sceneNr);
      }
    }

    virtual void saveScene(const int _sceneNr) {
      if(m_Interface->save_scene != NULL) {
        (*m_Interface->save_scene)(m_Handle, _sceneNr);
      }
    }

    virtual void undoScene(const int _sceneNr) {
      if(m_Interface->undo_scene != NULL) {
        (*m_Interface->undo_scene)(m_Handle, _sceneNr);
      }
    }

    virtual void increaseValue(const int _parameterNr = -1) {
      if(m_Interface->increase_value != NULL) {
        (*m_Interface->increase_value)(m_Handle, _parameterNr);
      }
    }

    virtual void decreaseValue(const int _parameterNr = -1) {
      if(m_Interface->decrease_value != NULL) {
        (*m_Interface->decrease_value)(m_Handle, _parameterNr);
      }
    }

    virtual void enable() {
      if(m_Interface->enable != NULL) {
        (*m_Interface->enable)(m_Handle);
      }
    }

    virtual void disable() {
      if(m_Interface->disable != NULL) {
        (*m_Interface->disable)(m_Handle);
      }
    }

    virtual void startDim(bool _directionUp, const int _parameterNr = -1) {
      if(m_Interface->start_dim != NULL) {
        (*m_Interface->start_dim)(m_Handle, _directionUp, _parameterNr);
      }
    }

    virtual void endDim(const int _parameterNr = -1) {
      if(m_Interface->end_dim != NULL) {
        (*m_Interface->end_dim)(m_Handle, _parameterNr);
      }
    }

    virtual void setValue(const double _value, int _parameterNr = -1) {
      if(m_Interface->set_value != NULL) {
        (*m_Interface->set_value)(m_Handle, _parameterNr, _value);
      }
    }

    virtual double getValue(int _parameterNr = -1) const {
      if(m_Interface->get_value != NULL) {
        return (*m_Interface->get_value)(m_Handle, _parameterNr);
      } else {
        return 0.0;
      }
    }

    virtual uint16_t getFunctionID() {
      if(m_Interface->get_function_id != NULL) {
        return (m_Interface->get_function_id)(m_Handle);
      }
      return 0;
    } // getFunctionID

    virtual void setConfigParameter(const std::string& _name, const std::string& _value) {
      if(m_Interface->set_configuration_parameter != NULL) {
        (*m_Interface->set_configuration_parameter)(m_Handle, _name.c_str(), _value.c_str());
      }
    } // setConfigParameter

    virtual std::string getConfigParameter(const std::string& _name) const {
      if(m_Interface->get_configuration_parameter != NULL) {
        const int bufferSize = 256;
        char buffer[bufferSize];
        memset(&buffer, '\0', bufferSize);
        int len = (*m_Interface->get_configuration_parameter)(m_Handle, _name.c_str(), &buffer[0], bufferSize - 1);

        if(len > 0 && len < bufferSize) {
          buffer[len] = '\0';
          return std::string(&buffer[0]);
        }
      }
      return "";
    } // getConfigParameter

    virtual uint8_t dsLinkSend(uint8_t _value, uint8_t _flags, bool& _handled) {
      if(m_Interface->udi_send != NULL) {
        _handled = true;
        return  (*m_Interface->udi_send)(m_Handle, _value, _flags);
      }
      _handled = false;
      return 0;
    } // dsLinkSend

  }; // DSIDPlugin


  //================================================== DSIDPluginCreator

  DSIDPluginCreator::DSIDPluginCreator(void* _soHandle, const char* _pluginName)
  : DSIDCreator(_pluginName),
    m_SOHandle(_soHandle)
  {
    createInstance = (int (*)())dlsym(m_SOHandle, "dsid_create_instance");
    char* error;
    if((error = dlerror()) != NULL) {
      Logger::getInstance()->log("sim: error getting pointer to dsid_create_instance");
    }
  } // ctor

  DSIDInterface* DSIDPluginCreator::createDSID(const dss_dsid_t _dsid, const devid_t _shortAddress, const DSMeterSim& _dsMeter) {
    int handle = (*createInstance)();
    return new DSIDPlugin(_dsMeter, _dsid, _shortAddress, m_SOHandle, handle);
  } // createDSID

} // namespace dss

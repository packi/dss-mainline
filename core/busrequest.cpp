/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "busrequest.h"
#include "core/model.h"
#include "core/ds485const.h"

#include <cassert>

namespace dss {

  class PacketBuilderHints : public PacketBuilderHintsBase {
  public:
    PacketBuilderHints(BusRequest* _pRequest)
    : m_pRequest(_pRequest)
    { }

    virtual int getNumberAddressParameter() { return 0; }
    virtual uint16_t getAddressParameter(int _parameter) { return 0; }

  protected:
    BusRequest* getRequest() {
      return m_pRequest;
    }
  private:
     BusRequest* m_pRequest;
  };


  
  class CommandBusRequestPacketBuilderHints : public PacketBuilderHints {
  public:
    
    CommandBusRequestPacketBuilderHints(CommandBusRequest* _pRequest)
    : PacketBuilderHints(_pRequest),
      m_pGroup(NULL),
      m_pDevice(NULL),
      m_FunctionIDForGroup(-1),
      m_FunctionIDForDevice(-1)
    {
      determineTypeOfTarget();
    }
    
    virtual int getFunctionID() {
      if(targetIsGroup()) {
        return m_FunctionIDForGroup;
      } else if(targetIsDevice()) {
        return m_FunctionIDForDevice;
      } else {
        assert(false);
      }
    }

    virtual bool isBroadcast() {
      return targetIsGroup();
    }

    virtual uint16_t getTarget() {
      if(targetIsDevice()) {
        return m_pDevice->getModulatorID();
      }
      return 0;
    }

    virtual int getNumberAddressParameter() {
      if(targetIsDevice()) {
        return 1;
      } else if(targetIsGroup()) {
        return 2;
      }
      throw std::runtime_error("getNumberAddressParameter: failed to get address parameter count");
    }

    virtual uint16_t getAddressParameter(int _parameter) {
      if(_parameter == 0) {
        if(targetIsDevice()) {
          return m_pDevice->getShortAddress();
        } else if(targetIsGroup()) {
          return m_pGroup->getZoneID();
        }
      } else if(_parameter == 1) {
        if(targetIsGroup()) {
          return m_pGroup->getID();
        }
      }
      throw std::runtime_error("getAddressParameter: failed to get address parameter nr. " + intToString(_parameter));
    }

    void setFunctionIDForDevice(const int _value) {
      m_FunctionIDForDevice = _value;
    }

    void setFunctionIDForGroup(const int _value) {
      m_FunctionIDForGroup = _value;
    }

    virtual int getNumberOfParameter() {
      return 0;
    }

    virtual uint16_t getParameter(int _parameter) {
      return 0;
    }
  private:
    void determineTypeOfTarget() {
      PhysicalModelItem* pTarget = ((CommandBusRequest*)getRequest())->getTarget();
      m_pGroup = dynamic_cast<Group*>(pTarget);
      m_pDevice = dynamic_cast<Device*>(pTarget);
    }

    bool targetIsGroup() {
      return m_pGroup != NULL;
    }

    bool targetIsDevice() {
      return m_pDevice != NULL;
    }
  private:
    Group* m_pGroup;
    Device* m_pDevice;
    int m_FunctionIDForGroup;
    int m_FunctionIDForDevice;
  }; // CommandBusRequestPacketBuilderHints
  
  class SceneCommandPacketBuilderHints : public CommandBusRequestPacketBuilderHints {
  public:
    SceneCommandPacketBuilderHints(SceneCommandBusRequest* _pRequest)
    : CommandBusRequestPacketBuilderHints(_pRequest)
    {
    }

    virtual int getNumberOfParameter() {
      return 1;
    }

    virtual uint16_t getParameter(int _parameter) {
      if(_parameter == 0) {
        return dynamic_cast<SceneCommandBusRequest*>(getRequest())->getSceneID();
      }
      throw std::out_of_range("_parameter out of range");
    }
  };
  
  PacketBuilderHintsBase* CallSceneCommandBusRequest::getBuilderHints() {
    SceneCommandPacketBuilderHints* result = new SceneCommandPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceCallScene);
    result->setFunctionIDForGroup(FunctionGroupCallScene);
    return result;
  } // getBuilderHints

  PacketBuilderHintsBase* SaveSceneCommandBusRequest::getBuilderHints() {
    SceneCommandPacketBuilderHints* result = new SceneCommandPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceSaveScene);
    result->setFunctionIDForGroup(FunctionGroupSaveScene);
    return result;
  } // getBuilderHints

  PacketBuilderHintsBase* UndoSceneCommandBusRequest::getBuilderHints() {
    SceneCommandPacketBuilderHints* result = new SceneCommandPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceUndoScene);
    result->setFunctionIDForGroup(FunctionGroupUndoScene);
    return result;
  } // getBuilderHints
  
} // namespace dss
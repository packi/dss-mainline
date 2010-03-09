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

#include "busrequest.h"
#include "core/ds485const.h"
#include "core/model/device.h"
#include "core/model/group.h"

#include <cassert>

namespace dss {

  class PacketBuilderHints : public PacketBuilderHintsBase {
  public:
    PacketBuilderHints(BusRequest* _pRequest)
    : m_pRequest(_pRequest)
    { }

  protected:
    BusRequest* getRequest() {
      return m_pRequest;
    }
  private:
     BusRequest* m_pRequest;
  };

  class DeviceCommandPacketBuilderHints : public PacketBuilderHints {
  public:
    DeviceCommandPacketBuilderHints(DeviceCommandBusRequest* _pRequest)
    : PacketBuilderHints(_pRequest)
    {}
    
    virtual int getNumberAddressParameter() {
      return 1;
    }

    virtual int getFunctionID() {
      return m_FunctionID;
    }

    virtual int getNumberOfParameter() {
      return 0;
    }

    virtual uint16_t getParameter(int _parameter) {
      return 0;
    }

    virtual uint16_t getAddressParameter(int _parameter) {
      if(_parameter == 0) {
        return getDevice()->getShortAddress();
      }
      throw std::runtime_error("getAddressParameter: failed to get address parameter nr. " + intToString(_parameter));
    }

    virtual uint16_t getTarget() {
      return getDevice()->getDSMeterID();
    }

    virtual bool isBroadcast() {
      return false;
    }

    void setFunctionID(uint16_t _value) {
      m_FunctionID = _value;
    }
  private:
    Device* getDevice() {
      return ((DeviceCommandBusRequest*)getRequest())->getTarget();
    }
  private:
    uint16_t m_FunctionID;
  };

  boost::shared_ptr<PacketBuilderHintsBase> EnableDeviceCommandBusRequest::getBuilderHints() {
    DeviceCommandPacketBuilderHints* result = new DeviceCommandPacketBuilderHints(this);
    result->setFunctionID(FunctionDeviceEnable);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  }

  boost::shared_ptr<PacketBuilderHintsBase> DisableDeviceCommandBusRequest::getBuilderHints() {
    DeviceCommandPacketBuilderHints* result = new DeviceCommandPacketBuilderHints(this);
    result->setFunctionID(FunctionDeviceDisable);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  }

  class CommandBusRequestPacketBuilderHintsBase : public PacketBuilderHints {
  public:
    CommandBusRequestPacketBuilderHintsBase(CommandBusRequest* _pRequest)
    : PacketBuilderHints(_pRequest),
      m_pGroup(NULL),
      m_pDevice(NULL)
    {
      determineTypeOfTarget();
    }

    virtual uint16_t getTarget() {
      if(targetIsDevice()) {
        return m_pDevice->getDSMeterID();
      }
      return 0;
    }

    virtual bool isBroadcast() {
      return targetIsGroup();
    }
  protected:
    bool targetIsGroup() {
      return m_pGroup != NULL;
    }

    bool targetIsDevice() {
      return m_pDevice != NULL;
    }

    Device* getDevice() {
      return m_pDevice;
    }

    Group* getGroup() {
      return m_pGroup;
    }
  private:
    void determineTypeOfTarget() {
      PhysicalModelItem* pTarget = ((CommandBusRequest*)getRequest())->getTarget();
      m_pGroup = dynamic_cast<Group*>(pTarget);
      m_pDevice = dynamic_cast<Device*>(pTarget);
    }
  private:
    Group* m_pGroup;
    Device* m_pDevice;
  }; // CommandBusRequestPacketBuilderHintsBase

  class DeviceGroupCommandPacketBuilderHints : public CommandBusRequestPacketBuilderHintsBase {
  public:
    DeviceGroupCommandPacketBuilderHints(CommandBusRequest* _pRequest)
    : CommandBusRequestPacketBuilderHintsBase(_pRequest)
    {}

    virtual int getNumberAddressParameter() {
      if(targetIsDevice()) {
        return 2;
      } else {
        return 3;
      }
    }

    virtual uint16_t getAddressParameter(int _parameter) {
      if(_parameter == 0) {
        return targetIsGroup() ? 0 : 1;
      } if(_parameter == 1) {
        if(targetIsDevice()) {
          return getDevice()->getShortAddress();
        } else if(targetIsGroup()) {
          return getGroup()->getZoneID();
        }
      } else if(_parameter == 2) {
        if(targetIsGroup()) {
          return getGroup()->getID();
        }
      }
      throw std::runtime_error("getAddressParameter: failed to get address parameter nr. " + intToString(_parameter));
    }

    void setFunctionID(int _functionID) {
      m_FunctionID = _functionID;
    }

    virtual int getFunctionID() {
      return m_FunctionID;
    }

    virtual int getNumberOfParameter() {
      return 0;
    }

    virtual uint16_t getParameter(int _parameter) {
      return 0;
    }
  private:
    int m_FunctionID;
  };

  class CommandBusRequestPacketBuilderHints : public CommandBusRequestPacketBuilderHintsBase {
  public:

    CommandBusRequestPacketBuilderHints(CommandBusRequest* _pRequest)
    : CommandBusRequestPacketBuilderHintsBase(_pRequest),
      m_FunctionIDForGroup(-1),
      m_FunctionIDForDevice(-1)
    {
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
          return getDevice()->getShortAddress();
        } else if(targetIsGroup()) {
          return getGroup()->getZoneID();
        }
      } else if(_parameter == 1) {
        if(targetIsGroup()) {
          return getGroup()->getID();
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
      return m_Parameter.size();
    }

    virtual uint16_t getParameter(int _parameter) {
      return m_Parameter.at(_parameter);
    }

    void addParameter(uint16_t _parameter) {
      m_Parameter.push_back(_parameter);
    }
  private:
    int m_FunctionIDForGroup;
    int m_FunctionIDForDevice;
    std::vector<uint16_t> m_Parameter;
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
  
  boost::shared_ptr<PacketBuilderHintsBase> CallSceneCommandBusRequest::getBuilderHints() {
    SceneCommandPacketBuilderHints* result = new SceneCommandPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceCallScene);
    result->setFunctionIDForGroup(FunctionGroupCallScene);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> SaveSceneCommandBusRequest::getBuilderHints() {
    SceneCommandPacketBuilderHints* result = new SceneCommandPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceSaveScene);
    result->setFunctionIDForGroup(FunctionGroupSaveScene);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> UndoSceneCommandBusRequest::getBuilderHints() {
    SceneCommandPacketBuilderHints* result = new SceneCommandPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceUndoScene);
    result->setFunctionIDForGroup(FunctionGroupUndoScene);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> IncreaseValueCommandBusRequest::getBuilderHints() {
    CommandBusRequestPacketBuilderHints* result = new CommandBusRequestPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceIncreaseValue);
    result->setFunctionIDForGroup(FunctionGroupIncreaseValue);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> DecreaseValueCommandBusRequest::getBuilderHints() {
    CommandBusRequestPacketBuilderHints* result = new CommandBusRequestPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceDecreaseValue);
    result->setFunctionIDForGroup(FunctionGroupDecreaseValue);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> StartDimUpCommandBusRequest::getBuilderHints() {
    CommandBusRequestPacketBuilderHints* result = new CommandBusRequestPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceStartDimInc);
    result->setFunctionIDForGroup(FunctionGroupStartDimInc);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> StartDimDownCommandBusRequest::getBuilderHints() {
    CommandBusRequestPacketBuilderHints* result = new CommandBusRequestPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceStartDimDec);
    result->setFunctionIDForGroup(FunctionGroupStartDimDec);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> EndDimCommandBusRequest::getBuilderHints() {
    CommandBusRequestPacketBuilderHints* result = new CommandBusRequestPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceEndDim);
    result->setFunctionIDForGroup(FunctionGroupEndDim);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> SetValueCommandBusRequest::getBuilderHints() {
    CommandBusRequestPacketBuilderHints* result = new CommandBusRequestPacketBuilderHints(this);
    result->setFunctionIDForDevice(FunctionDeviceSetValue);
    result->setFunctionIDForGroup(FunctionGroupSetValue);
    result->addParameter(m_Value);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

  boost::shared_ptr<PacketBuilderHintsBase> BlinkCommandBusRequest::getBuilderHints() {
    DeviceGroupCommandPacketBuilderHints* result = new DeviceGroupCommandPacketBuilderHints(this);
    result->setFunctionID(FunctionBlink);
    return boost::shared_ptr<PacketBuilderHintsBase>(result);
  } // getBuilderHints

} // namespace dss

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

#ifndef BUSREQUEST_H
#define BUSREQUEST_H

#include <stdint.h>

#include <boost/shared_ptr.hpp>

namespace dss {
  class AddressableModelItem;
  class Device;

  class PacketBuilderHintsBase {
  public:
    virtual int getFunctionID() = 0;
    virtual int getNumberAddressParameter() = 0;
    virtual uint16_t getAddressParameter(int _parameter) = 0;
    virtual uint16_t getTarget() = 0;
    virtual bool isBroadcast() = 0;
    virtual int getNumberOfParameter() = 0;
    virtual uint16_t getParameter(int _parameter) = 0;

    virtual ~PacketBuilderHintsBase() {}; // please the compiler (virtual dtor)
  };

  class BusRequest {
  public:
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints() = 0;
    virtual ~BusRequest() {}; // please the compiler (virtual dtor)
  };

  class CommandBusRequest : public BusRequest {
  public:
    void setTarget(boost::shared_ptr<AddressableModelItem> _value) { m_pTarget = _value; }
    boost::shared_ptr<AddressableModelItem> getTarget() const { return m_pTarget; }
  private:
    boost::shared_ptr<AddressableModelItem> m_pTarget;
  };

  class DeviceCommandBusRequest : public BusRequest {
  public:
    void setTarget(boost::shared_ptr<Device> _value) { m_pTarget = _value; }
    boost::shared_ptr<Device> getTarget() const { return m_pTarget; }
  private:
    boost::shared_ptr<Device> m_pTarget;
  };

  class EnableDeviceCommandBusRequest : public DeviceCommandBusRequest {
  public:
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class DisableDeviceCommandBusRequest : public DeviceCommandBusRequest {
  public:
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class SceneCommandBusRequest : public CommandBusRequest {
  public:
    void setSceneID(const int _value) { m_SceneID = _value; }
    int getSceneID() const { return m_SceneID; }
  private:
    int m_SceneID;
  };

  class CallSceneCommandBusRequest : public SceneCommandBusRequest {
  public:
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class SaveSceneCommandBusRequest : public SceneCommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class UndoSceneCommandBusRequest : public SceneCommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class IncreaseValueCommandBusRequest : public CommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class DecreaseValueCommandBusRequest : public CommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class StartDimUpCommandBusRequest : public CommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class StartDimDownCommandBusRequest : public CommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class EndDimCommandBusRequest : public CommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };

  class SetValueCommandBusRequest : public CommandBusRequest {
  public:
    void setValue(const int _value) {
      m_Value = _value;
    }

    uint16_t getValue() const {
      return m_Value;
    }
  private:
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  private:
    uint16_t m_Value;
  };

  class BlinkCommandBusRequest : public CommandBusRequest {
    virtual boost::shared_ptr<PacketBuilderHintsBase> getBuilderHints();
  };
}

#endif // BUSREQUEST_H

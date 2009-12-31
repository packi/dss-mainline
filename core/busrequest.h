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

#ifndef BUSREQUEST_H
#define BUSREQUEST_H

#include <stdint.h>

namespace dss {
  class PhysicalModelItem;
  class PacketBuilderHintsBase {
  public:
    virtual int getFunctionID() = 0;
    virtual int getNumberAddressParameter() = 0;
    virtual uint16_t getAddressParameter(int _parameter) = 0;
    virtual uint16_t getTarget() = 0;
    virtual bool isBroadcast() = 0;
    virtual int getNumberOfParameter() = 0;
    virtual uint16_t getParameter(int _parameter) = 0;
  };

  class BusRequest {
  public:
    virtual PacketBuilderHintsBase* getBuilderHints() = 0;
  };

  class CommandBusRequest : public BusRequest {
  public:
    void setTarget(PhysicalModelItem* _value) { m_pTarget = _value; }
    PhysicalModelItem* getTarget() const { return m_pTarget; }
  private:
    PhysicalModelItem* m_pTarget;
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
    virtual PacketBuilderHintsBase* getBuilderHints();
  };

  class SaveSceneCommandBusRequest : public SceneCommandBusRequest {
    virtual PacketBuilderHintsBase* getBuilderHints();
  };
  
  class UndoSceneCommandBusRequest : public SceneCommandBusRequest {
    virtual PacketBuilderHintsBase* getBuilderHints();
  };
}

#endif // BUSREQUEST_H

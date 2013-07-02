/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#ifndef STATE_H
#define STATE_H

#include "deviceinterface.h"
#include "modeltypes.h"
#include "propertysystem.h"

#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>

namespace dss {
  class Device;
  class Group;

  typedef enum {
    State_Invalid = 0,
    State_Active = 1,
    State_Inactive = 2,
    State_Unknown = 3,
  } eState;

  typedef enum {
    StateType_Apartment = 0,
    StateType_Device = 1,
    StateType_Service = 2,
    StateType_Group = 3,
    StateType_Script = 4
  } eStateType;

  /** Represents a common class for Device, Service and Apartment states.*/
  class State : public boost::noncopyable,
                public boost::enable_shared_from_this<State> {

  private:
    PropertyNodePtr m_pPropertyNode;
    std::string m_name;
    bool m_IsPersistent;

    eState m_state;
    eStateType m_type;

    /** State provider is a device */
    boost::shared_ptr<Device> m_providerDev;
    int m_providerDevInput;

    /** State provider is a web service or js script*/
    std::string m_serviceName;

    /** State provider is a group */
    boost::shared_ptr<Group> m_providerGroup;

  private:
    void save();
    void load();
    std::string getStorageName();

  public:
    /** Constructs a state. */
    State(const std::string& _name);
    State(const std::string& _name, eState _state);
    State(boost::shared_ptr<Device>_device, int _inputIndex);
    State(boost::shared_ptr<Group> _group);
    State(eStateType _type, const std::string& _name,
          const std::string& _identifier);

    virtual ~State();

    eState getState() const;
    void setState(const callOrigin_t _origin, const eState _state);
    void setState(const callOrigin_t _origin, const int _state);
    void setState(const callOrigin_t _origin, const std::string& _state);

    std::string getName() const { return m_name; }
    void setName(const std::string& _name) { m_name = _name; }

    bool getPersistence() const;
    void setPersistence(bool _persistent);

    eStateType getType() const { return m_type; }
    PropertyNodePtr getPropertyNode() const { return m_pPropertyNode; }

    boost::shared_ptr<Device> getProviderDevice() const { return m_providerDev; }
    boost::shared_ptr<Group> getProviderGroup() const { return m_providerGroup; }
    void setProviderDevice(boost::shared_ptr<Device> _dev, int _inputIndex) {
      m_providerDev = _dev; m_providerDevInput = _inputIndex;
      removeFromPropertyTree();
      publishToPropertyTree();
    }

    std::string getProviderService() const { return m_serviceName; }

    std::string toString() const;

    void publishToPropertyTree();
    void removeFromPropertyTree();
  }; // State

} // namespace dss

#endif // STATE_H

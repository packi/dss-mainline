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
#include "propertysystem.h"

#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>

#include "ds485types.h"

namespace dss {
  class Device;
  class Group;
  class Zone;
  class DSMeter;

  typedef enum {
    State_Invalid = 0,
    State_Active = 1,
    State_Inactive = 2,
    State_Unknown = 3,
  } eState;

  typedef enum {
    StateWH_Invalid = 0,
    StateWH_Closed = 1,
    StateWH_Open = 2,
    StateWH_Tilted = 3,
    StateWH_Unknown = 4,
  } eStateWindowHandle;

  typedef enum {
    StateType_Apartment = 0,
    StateType_Device = 1,
    StateType_Service = 2,
    StateType_Group = 3,
    StateType_Script = 4,
    StateType_SensorZone = 5,
    StateType_SensorDevice = 6,
    StateType_Circuit = 7,
  } eStateType;

  /** Represents a common class for Device, Service and Apartment states.*/
  class State : public boost::noncopyable,
                public boost::enable_shared_from_this<State> {

  private:
    PropertyNodePtr m_pPropertyNode;
    std::string m_name;
    bool m_IsPersistent;
    callOrigin_t m_callOrigin;
    dsuid_t m_originDeviceDSUID;

    int m_state;
    eStateType m_type;

    /** State provider is a device */
    boost::shared_ptr<Device> m_providerDev;
    int m_providerDevInput;
    std::string m_providerDevStateName;

    /** State provider is a dSM */
    boost::shared_ptr<DSMeter> m_providerDsm;

    /** State provider is a web service or js script*/
    std::string m_serviceName;

    /** State provider is a group */
    boost::shared_ptr<Group> m_providerGroup;

    /** Custom values for a state */
    std::vector<std::string> m_values;

  private:
    void save();
    void load();
    std::string getStorageName();

  public:
    /** Constructs a state. */
    State(const std::string& _name);
    State(eStateType _type, const std::string& _name, const std::string& _identifier);
    State(boost::shared_ptr<Device> _device, int _inputIndex);
    State(boost::shared_ptr<Device> _device, const std::string& stateName);
    static std::string makeGroupName(const Group& group);
    State(boost::shared_ptr<Group> _group, const std::string& name);
    State(boost::shared_ptr<DSMeter> _meter, int _inputIndex);

    virtual ~State();

    static const std::string INVALID;

    int getState() const;
    void setState(const callOrigin_t _origin, const int _state);
    void setState(const callOrigin_t _origin, const std::string& _state);

    const std::string& getName() const { return m_name; }
    void setName(const std::string& _name) { m_name = _name; }

    bool getPersistence() const;
    void setPersistence(bool _persistent);
    bool hasPersistentData();

    typedef std::vector<std::string> ValueRange_t;
    void setValueRange(const ValueRange_t& _values);
    unsigned int getValueRangeSize() const;

    eStateType getType() const { return m_type; }
    PropertyNodePtr getPropertyNode() const { return m_pPropertyNode; }

    boost::shared_ptr<Group> getProviderGroup() const { return m_providerGroup; }
    void setProviderGroup(boost::shared_ptr<Group> _group) {
      m_providerGroup = _group;
      removeFromPropertyTree();
      publishToPropertyTree();
    }

    boost::shared_ptr<Device> getProviderDevice() const { return m_providerDev; }
    int getProviderDeviceInput() const { return m_providerDevInput; }
    void setProviderDevice(boost::shared_ptr<Device> _dev) {
      m_providerDev = _dev;
      removeFromPropertyTree();
      publishToPropertyTree();
    }

    boost::shared_ptr<DSMeter> getProviderDsm() const { return m_providerDsm; }

    std::string getProviderService() const { return m_serviceName; }

    std::string toString() const;

    void publishToPropertyTree();
    void removeFromPropertyTree();

    void setOriginDeviceDSUID(const dsuid_t _dsuid);
    std::string getOriginDeviceDSUIDString() const { return dsuid2str(m_originDeviceDSUID); }
  }; // State

  /** Represents a class for Device sensor value state.*/
  class StateSensor : public State {
  private:
    typedef enum {
      eComp_undef = 0,
      eComp_lower = 1,
      eComp_higher = 2
    } eValueComparator;
    std::string m_activateCondition;
    std::string m_deactivateCondition;
    eValueComparator m_activateComparator;
    eValueComparator m_deactivateComparator;
    double m_activateValue;
    double m_deactivateValue;
    void parseCondition(const std::string& _input, eValueComparator& _comp, double& _threshold);

  public:
    StateSensor(const std::string& _identifier, const std::string& _scriptId,
        boost::shared_ptr<Device> _dev, SensorType _sensorType,
        const std::string& activateCondition, const std::string& deactivateCondition);
    StateSensor(const std::string& _identifier, const std::string& _scriptId,
        boost::shared_ptr<Group> _group, SensorType _sensorType,
        const std::string& activateCondition, const std::string& deactivateCondition);
    virtual ~StateSensor();

    void newValue(const callOrigin_t _origin, double _value);
  }; // StateSensor

} // namespace dss

#endif // STATE_H

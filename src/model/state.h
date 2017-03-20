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
#include <boost/optional.hpp>

#include "dssfwd.h"
#include "ds485types.h"
#include "modelconst.h"

namespace dss {
  enum class StatePersistency {
    volatile_ = 0,
    persistent = 1,
  };

  /** Represents a common class for Device, Service and Apartment states.*/
  class State : public boost::noncopyable,
                public boost::enable_shared_from_this<State> {

  protected:
    PropertyNodePtr m_pPropertyNode;
    std::string m_name;
    StatePersistency m_persistency;
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

  protected:
    void save();
    void load();
    std::string getStorageName();

    /// used by derived classes to construct base class
    /// note it will not load persistent date or publish state to property tree
    State(const std::string& name, eStateType type, eState initState, StatePersistency persistency);

  public:
    /** Constructs a state. */
    State(const std::string& _name);
    State(eStateType _type, const std::string& _name, const std::string& _identifier = std::string());
    State(boost::shared_ptr<Device> _device, const std::string& stateName);
    static std::string makeGroupName(const Group& group);
    State(boost::shared_ptr<Group> _group, const std::string& name);
    State(boost::shared_ptr<DSMeter> _meter, int _inputIndex);

    virtual ~State();

    static const std::string INVALID;

    // return value from value name
    boost::optional<int> tryValueFromName(const std::string& valueName);
    int valueFromName(const std::string& valueName);

    int getState() const;
    void setState(const callOrigin_t _origin, const int _state);
    void setState(const callOrigin_t _origin, const std::string& _state);

    /// Set state to `static_cast<int>(value) + 1`
    ///
    /// Value 0 is reserved for "invalid"
    ///
    /// TODO(someday): make State (sub)class template by T to make this method type safe
    template <class T, class = typename std::enable_if<std::is_enum<T>::value>::type>
    void setState(T value) {
         setState(coSystem, static_cast<int>(value) + 1);
    }

    /// Get state as enum T. 0 is mapped to boost::none, other values are static casted to T by -1
    template <class T, class = typename std::enable_if<std::is_enum<T>::value>::type>
    boost::optional<T> getState() const {
      auto&& x = getState();
      if (x == 0) {
        return boost::none;
      }
      return static_cast<T>(x - 1);
    }

    const std::string& getName() const { return m_name; }
    void setName(const std::string& _name) { m_name = _name; }

    bool getPersistence() const;
    void setPersistence(bool _persistent);
    bool hasPersistentData();

    typedef std::vector<std::string> ValueRange_t;
    void setValueRange(ValueRange_t values) { m_values = std::move(values); }
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

  class BinaryInputState : public State {
    std::string makeName(const dsuid_t &dsuid, int inputIndex);
  public:
    BinaryInputState(boost::shared_ptr<Device> device, int inputIndex);
    ~BinaryInputState();
  };

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

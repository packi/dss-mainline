/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef GROUP_H
#define GROUP_H

#include <map>
#include <boost/thread/mutex.hpp>

#include "devicecontainer.h"
#include "addressablemodelitem.h"
#include "modelconst.h"
#include "src/businterface.h"

namespace dss {

  class Zone;
  class Status;
  class StatusField;

  /** Represents a predefined group */
  class Group : public DeviceContainer,
                public AddressableModelItem {
  private:
    __DECL_LOG_CHANNEL__;
    int m_ZoneID;
    int m_GroupID;
    ApplicationType m_ApplicationType;
    std::unique_ptr<Behavior> m_pApplicationBehavior;
    int m_LastCalledScene;
    int m_LastButOneCalledScene;
    bool m_IsValid;
    bool m_SyncPending;
    bool m_readFromDsm;
    std::string m_AssociatedSet;
    std::map<uint8_t, std::string> m_SceneNames;
    typedef std::map<uint8_t, std::string> m_SceneNames_t;
    static boost::mutex m_SceneNameMutex;
    int m_connectedDevices;
    std::unique_ptr<Status> m_status; // set only for groups supporting Status.

    // getter and setter for property proxy
    int getApplicationTypeInt() const { return static_cast<int>(getApplicationType()); }
    void setApplicationTypeInt(int applicationType) { setApplicationType(static_cast<ApplicationType>(applicationType)); }
  public:
    /** Constructs a group with the given id belonging to \a _zoneID. */
    Group(const int _id, boost::shared_ptr<Zone> _pZone);
    virtual ~Group();
    boost::shared_ptr<Group> sharedFromThis() { return boost::static_pointer_cast<Group>(shared_from_this()); }

    virtual Set getDevices() const;

    /** Returns the id of the group */
    int getID() const { return m_GroupID; }
    int getZoneID() const { return m_ZoneID; }

    /** Returns the type of the application that this group is implementing */
    ApplicationType getApplicationType() const { return m_ApplicationType; }
    void setApplicationType(ApplicationType applicationType);

    int getColor() const;

    /** Returns and sets the configuration of this group */
    uint32_t getApplicationConfiguration() const { return m_pApplicationBehavior->getConfiguration(); }
    void setApplicationConfiguration(const uint32_t applicationConfiguration) {
      return m_pApplicationBehavior->setConfiguration(applicationConfiguration);
    }

    /** Translates the configuration back and forth between binary and JSON format */
    void serializeApplicationConfiguration(uint32_t configuration, JSONWriter& writer) const {
      m_pApplicationBehavior->serializeConfiguration(configuration, writer);
    }
    std::string serializeApplicationConfiguration(uint32_t configuration) const {
      JSONWriter writer(JSONWriter::jsonNoneResult);
      m_pApplicationBehavior->serializeConfiguration(configuration, writer);
      return writer.successJSON();
    }
    uint32_t deserializeApplicationConfiguration(const std::string& jsonConfiguration) const {
      return m_pApplicationBehavior->deserializeConfiguration(jsonConfiguration);
    }

    std::vector<int> getAvailableScenes() { return m_pApplicationBehavior->getAvailableScenes(); }

    /** returns true if the group is configured and usable */
    bool isValid() const;
    void setIsValid(const bool _value) { m_IsValid = _value; }

    bool isSynchronized() const { return !m_SyncPending; }
    void setIsSynchronized(const bool _value) { m_SyncPending = !_value; }

    void setReadFromDsm(const bool _readFromDsm) { m_readFromDsm = _readFromDsm; }
    bool isReadFromDsm() const { return m_readFromDsm; }

    void setFromSpec(const GroupSpec_t& spec);
    bool isConfigEqual(const GroupSpec_t& spec);

    std::string getAssociatedSet() const { return m_AssociatedSet; }
    void setAssociatedSet(const std::string& _value) { m_AssociatedSet = _value; }

    virtual void callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token, const bool _force);
    virtual void nextScene(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual void previousScene(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual unsigned long getPowerConsumption();

    /** @copydoc Device::getLastCalledScene */
    int getLastCalledScene() const { return m_LastCalledScene; }
    /** @copydoc Device::setLastCalledScene */
    void setLastCalledScene(const int _value);
    /** @copydoc Device::setLastButOneCalledScene */
    void setLastButOneCalledScene(const int _value) {
      if (_value == m_LastCalledScene) {
        m_LastCalledScene = m_LastButOneCalledScene;
      }
    }
    /** @copydoc Device::setLastButOneCalledScene */
    void setLastButOneCalledScene() {
      m_LastCalledScene = m_LastButOneCalledScene;
    }

    Group& operator=(const Group& _other);
    void setSceneName(int _sceneNumber, const std::string& _name);
    std::string getSceneName(int _sceneNumber);

    virtual void publishToPropertyTree();

    void setOnState(const callOrigin_t _origin, const bool _on);
    void setOnState(const callOrigin_t _origin, const int _sceneId);
    eState getState();

    /** Published a sensor value to all devices of this zone */
    void sensorPush(const dsuid_t& _sourceID, SensorType _type, double _value);
    void sensorInvalid(SensorType _type);

    /// Get status object for the group. nullptr if the group does not support status.
    Status* tryGetStatus() { return m_status.get(); }
    Status& getStatus();

    /// Convenient scripting support method calling `getStatus()->getField(field).setValueAndPush(value)`.
    /// Throws on any error.
    void setStatusField(const std::string& field, const std::string& value);

    void addConnectedDevice();
    void removeConnectedDevice();
    bool hasConnectedDevices() { return (m_connectedDevices > 0); }

    static boost::shared_ptr<Group> make(const GroupSpec_t& _spec, boost::shared_ptr<Zone> _pZone);
  }; // Group

  std::ostream& operator<<(std::ostream &, const Group &);

} // namespace dss

#endif // GROUP_H

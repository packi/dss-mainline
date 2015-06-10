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

#include "modeltypes.h"
#include "modelconst.h"
#include "devicecontainer.h"
#include "addressablemodelitem.h"
#include "state.h"

namespace dss {

  class Zone;

  /** Represents a predefined group */
  class Group : public DeviceContainer,
                public AddressableModelItem {
  private:
    int m_ZoneID;
    int m_GroupID;
    int m_StandardGroupID;
    int m_LastCalledScene;
    int m_LastButOneCalledScene;
    bool m_IsValid;
    bool m_SyncPending;
    std::string m_AssociatedSet;
    std::map<uint8_t, std::string> m_SceneNames;
    typedef std::map<uint8_t, std::string> m_SceneNames_t;
    static boost::mutex m_SceneNameMutex;
    int m_connectedDevices;
  public:
    /** Constructs a group with the given id belonging to \a _zoneID. */
    Group(const int _id, boost::shared_ptr<Zone> _pZone, Apartment& _apartment);
    virtual ~Group() {};
    virtual Set getDevices() const;

    /** Returns the id of the group */
    int getID() const { return m_GroupID; }
    int getZoneID() const { return m_ZoneID; }

    /** Returns the id of the default behavior and associated standard group number */
    int getStandardGroupID() const { return m_StandardGroupID; }
    void setStandardGroupID(const int _standardGroupNumber);

    /** returns true if the group is configured and usable */
    bool isValid() const;
    void setIsValid(const bool _value) { m_IsValid = _value; }

    bool isSynchronized() const { return !m_SyncPending; }
    void setIsSynchronized(const bool _value) { m_SyncPending = !_value; }

    std::string getAssociatedSet() const { return m_AssociatedSet; }
    void setAssociatedSet(const std::string& _value) { m_AssociatedSet = _value; }

    virtual void callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token, const bool _force);
    virtual void nextScene(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual void previousScene(const callOrigin_t _origin, const SceneAccessCategory _category);
    virtual unsigned long getPowerConsumption();

    /** @copydoc Device::getLastCalledScene */
    int getLastCalledScene() const { return m_LastCalledScene; }
    /** @copydoc Device::setLastCalledScene */
    void setLastCalledScene(const int _value) {
      if (m_GroupID == GroupIDControlTemperature) {
        if (_value >= 16) {
          return;
        }
      }
      if (_value != m_LastCalledScene) {
        m_LastButOneCalledScene = m_LastCalledScene;
        m_LastCalledScene = _value;
      }
    }
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

    void publishToPropertyTree();

    void setOnState(const callOrigin_t _origin, const bool _on);
    void setOnState(const callOrigin_t _origin, const int _sceneId);
    eState getState();

    /** Published a sensor value to all devices of this zone */
    void sensorPush(const std::string& _sourceID, const int _sensorType, const double _sensorValue);

    void addConnectedDevice();
    void removeConnectedDevice();
    bool hasConnectedDevices() { return (m_connectedDevices > 0); }
  }; // Group

} // namespace dss

#endif // GROUP_H

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

#include "modeltypes.h"
#include "devicecontainer.h"
#include "addressablemodelitem.h"

namespace dss {

  class Zone;

  /** Represents a predefined group */
  class Group : public DeviceContainer,
                public AddressableModelItem {
  protected:
    int m_ZoneID;
    int m_GroupID;
    int m_LastCalledScene;
    std::map<uint8_t, std::string> m_SceneNames;
    bool m_IsInitializedFromBus;
  public:
    /** Constructs a group with the given id belonging to \a _zoneID. */
    Group(const int _id, boost::shared_ptr<Zone> _pZone, Apartment& _apartment);
    virtual ~Group() {};
    virtual Set getDevices() const;

    /** Returns the id of the group */
    int getID() const;
    int getZoneID() const;

    virtual void callScene(const int _sceneNr);

    virtual void nextScene();
    virtual void previousScene();

    virtual unsigned long getPowerConsumption();

    /** @copydoc Device::getLastCalledScene */
    int getLastCalledScene() const { return m_LastCalledScene; }
    /** @copydoc Device::setLastCalledScene */
    void setLastCalledScene(const int _value) { m_LastCalledScene = _value; }

    Group& operator=(const Group& _other);
    void setSceneName(int _sceneNumber, const std::string& _name);
    const std::string& getSceneName(int _sceneNumber);
    bool isInitializedFromBus() { return m_IsInitializedFromBus; }
    void setIsInitializedFromBus(bool _value) { m_IsInitializedFromBus = _value; }
  }; // Group


  /** Represents a user-defined-group */
  class UserGroup : public Group {
  private:
  public:
    /** Adds a device to the group.
     * This will permanently add the device to the group.
     */
    virtual void addDevice(const Device& _device);
    /** Removes a device from the group.
     * This will permanently remove the device from the group.
     */
    virtual void removeDevice(const Device& _device);
  }; // UserGroup

} // namespace dss

#endif // GROUP_H

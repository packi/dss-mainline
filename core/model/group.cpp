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

#include "group.h"
#include "scenehelper.h"
#include "set.h"
#include "apartment.h"

#include "core/ds485const.h"
#include "core/model/modelconst.h"
namespace dss {

    //============================================= Group

  Group::Group(const int _id, const int _zoneID, Apartment& _apartment)
  : AddressableModelItem(&_apartment),
    m_ZoneID(_zoneID),
    m_GroupID(_id),
    m_LastCalledScene(SceneOff)
  {
  } // ctor

  int Group::getID() const {
    return m_GroupID;
  } // getID

  int Group::getZoneID() const {
    return m_ZoneID;
  } // getZoneID

  Set Group::getDevices() const {
    return m_pApartment->getDevices().getByZone(m_ZoneID).getByGroup(m_GroupID);
  } // getDevices

  Group& Group::operator=(const Group& _other) {
    m_GroupID = _other.m_GroupID;
    m_ZoneID = _other.m_ZoneID;
    return *this;
  } // operator=

  void Group::callScene(const int _sceneNr) {
    // this might be redundant, but since a set could be
    // optimized if it contains only one device its safer like that...
    if(SceneHelper::rememberScene(_sceneNr & 0x00ff)) {
      m_LastCalledScene = _sceneNr & 0x00ff;
    }
    AddressableModelItem::callScene(_sceneNr);
  } // callScene

  unsigned long Group::getPowerConsumption() {
    return getDevices().getPowerConsumption();
  } // getPowerConsumption

  void Group::nextScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // nextScene

  void Group::previousScene() {
    callScene(SceneHelper::getPreviousScene(m_LastCalledScene));
  } // previousScene

} // namespace dss

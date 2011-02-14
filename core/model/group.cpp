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
#include "zone.h"
#include "scenehelper.h"
#include "set.h"
#include "apartment.h"
#include "core/propertysystem.h"

#include "core/model/modelconst.h"
namespace dss {

    //============================================= Group

  Group::Group(const int _id, boost::shared_ptr<Zone> _pZone, Apartment& _apartment)
  : AddressableModelItem(&_apartment),
    m_ZoneID(_pZone->getID()),
    m_GroupID(_id),
    m_LastCalledScene(SceneOff),
    m_IsInitializedFromBus(false)
  {
  } // ctor

  int Group::getID() const {
    return m_GroupID;
  } // getID

  int Group::getZoneID() const {
    return m_ZoneID;
  } // getZoneID

  Set Group::getDevices() const {
    return m_pApartment->getZone(m_ZoneID)->getDevices().getByGroup(m_GroupID);
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

  void Group::setSceneName(int _sceneNumber, const std::string& _name) {
    boost::mutex::scoped_lock lock(m_SceneNameMutex);
    m_SceneNames[_sceneNumber] = _name;
    if (m_pPropertyNode != NULL) {
      PropertyNodePtr scene = m_pPropertyNode->createProperty("scenes/scene" +
        intToString(_sceneNumber));

      scene->createProperty("scene")->setIntegerValue(_sceneNumber);
      scene->createProperty("name")->setStringValue(_name);
    }
  } // setSceneName

  const std::string& Group::getSceneName(int _sceneNumber) {
    boost::mutex::scoped_lock lock(m_SceneNameMutex);
    return m_SceneNames[_sceneNumber];
  } // getSceneName

  void Group::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone" + intToString(m_ZoneID) + "/groups/group" + intToString(m_GroupID));
        m_pPropertyNode->createProperty("group")->setIntegerValue(m_GroupID);
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Group, std::string>(*this, &Group::getName, &Group::setName));
        m_pPropertyNode->createProperty("lastCalledScene")
          ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*this, &Group::getLastCalledScene));
        m_pPropertyNode->createProperty("scenes");
      }
    }
  } // publishToPropertyTree

  boost::mutex Group::m_SceneNameMutex;

} // namespace dss

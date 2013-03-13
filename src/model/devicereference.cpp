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

#include "devicereference.h"

#include "device.h"
#include "apartment.h"

namespace dss {

  //================================================== DeviceReference

  DeviceReference::DeviceReference(const DeviceReference& _copy)
  : m_DSID(_copy.m_DSID),
    m_Apartment(_copy.m_Apartment)
  {
  } // ctor(copy)

  DeviceReference::DeviceReference(const dss_dsid_t _dsid, const Apartment* _apartment)
  : m_DSID(_dsid),
    m_Apartment(_apartment)
  {
  } // ctor(dsid)

  DeviceReference::DeviceReference(boost::shared_ptr<const Device> _device, const Apartment* _apartment)
  : m_DSID(_device->getDSID()),
    m_Apartment(_apartment)
  {
  } // ctor(device)

  boost::shared_ptr<Device> DeviceReference::getDevice() {
    return m_Apartment->getDeviceByDSID(m_DSID);
  } // getDevice

  boost::shared_ptr<const Device> DeviceReference::getDevice() const {
    return m_Apartment->getDeviceByDSID(m_DSID);
  } // getDevice

  dss_dsid_t DeviceReference::getDSID() const {
    return m_DSID;
  } // getID

  int DeviceReference::getFunctionID() const {
    return getDevice()->getFunctionID();
  } // getFunctionID

  std::string DeviceReference::getName() const {
    return getDevice()->getName();
  } //getName

  void DeviceReference::increaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getDevice()->increaseValue(_origin, _category);
  } // increaseValue

  void DeviceReference::decreaseValue(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getDevice()->decreaseValue(_origin, _category);
  } // decreaseValue

  void DeviceReference::setValue(const callOrigin_t _origin, const uint8_t _value) {
    getDevice()->setValue(_origin, _value);
  } // setValue

  bool DeviceReference::isOn() const {
    return getDevice()->isOn();
  }

  void DeviceReference::callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const bool _force) {
    getDevice()->callScene(_origin, _category, _sceneNr, _force);
  } // callScene

  void DeviceReference::saveScene(const callOrigin_t _origin, const int _sceneNr) {
    getDevice()->saveScene(_origin, _sceneNr);
  } // saveScene

  void DeviceReference::undoScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr) {
    getDevice()->undoScene(_origin, _category, _sceneNr);
  } // undoScene

  void DeviceReference::undoSceneLast(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getDevice()->undoSceneLast(_origin, _category);
  } // undoSceneLast

  unsigned long DeviceReference::getPowerConsumption() {
    return getDevice()->getPowerConsumption();
  }

  void DeviceReference::nextScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getDevice()->nextScene(_origin, _category);
  }

  void DeviceReference::previousScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getDevice()->previousScene(_origin, _category);
  }

  void DeviceReference::blink(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getDevice()->blink(_origin, _category);
  }

  std::string DeviceReference::getSensorEventName(const int _eventIndex) {
    return getDevice()->getSensorEventName(_eventIndex);
  }

} // namespace dss

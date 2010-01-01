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

#include "model.h"

#include "DS485Interface.h"
#include "ds485const.h"
#include "dss.h"
#include "logger.h"
#include "propertysystem.h"
#include "event.h"

#include "foreach.h"

#include "core/busrequestdispatcher.h"
#include "core/busrequest.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/ProcessingInstruction.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/SAX/InputSource.h>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::DOMParser;
using Poco::XML::XMLWriter;
using Poco::XML::InputSource;
using Poco::XML::Node;

namespace dss {

  //================================================== AddressableModelItem
 
  AddressableModelItem::AddressableModelItem(Apartment* _pApartment)
  : m_pApartment(_pApartment)
  {} // ctor
 
  void AddressableModelItem::turnOn() {
    callScene(SceneMax) ;
  } // turnOn
  
  void AddressableModelItem::turnOff() {
    callScene(SceneMin);
  } // turnOff
 /*
  void increaseValue(const int _parameterNr) {
  }
  
  void decreaseValue(const int _parameterNr) {
  }

  void startDim(const bool _directionUp, const int _parameterNr = -1) {
  }
  void endDim(const int _parameterNr = -1);
  void setValue(const double _value, const int _parameterNr = -1);
*/
  void AddressableModelItem::callScene(const int _sceneNr) {
    boost::shared_ptr<CallSceneCommandBusRequest> request(new CallSceneCommandBusRequest());
    request->setTarget(this);
    request->setSceneID(_sceneNr);
    m_pApartment->dispatchRequest(request);
  } // callScene

  void AddressableModelItem::saveScene(const int _sceneNr) {
    boost::shared_ptr<SaveSceneCommandBusRequest> request(new SaveSceneCommandBusRequest());
    request->setTarget(this);
    request->setSceneID(_sceneNr);
    m_pApartment->dispatchRequest(request);
  } // saveScene

  void AddressableModelItem::undoScene(const int _sceneNr) {
    boost::shared_ptr<UndoSceneCommandBusRequest> request(new UndoSceneCommandBusRequest());
    request->setTarget(this);
    request->setSceneID(_sceneNr);
    m_pApartment->dispatchRequest(request);
  } // undoScene
/*
  void nextScene();
  void previousScene();
*/
  //================================================== Device

  const devid_t ShortAddressStaleDevice = 0xFFFF;

  Device::Device(dsid_t _dsid, Apartment* _pApartment)
  : AddressableModelItem(_pApartment),
    m_DSID(_dsid),
    m_ShortAddress(ShortAddressStaleDevice),
    m_ZoneID(0),
    m_FunctionID(0),
    m_LastCalledScene(SceneOff),
    m_Consumption(0),
    m_LastDiscovered(DateTime::NullDate),
    m_FirstSeen(DateTime::NullDate)
  { } // ctor

  void Device::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone0/" + m_DSID.toString());
//        m_pPropertyNode->createProperty("name")->linkToProxy(PropertyProxyMemberFunction<Device, std::string>(*this, &Device::getName, &Device::setName));
        m_pPropertyNode->createProperty("name")->linkToProxy(PropertyProxyReference<std::string>(m_Name));
        m_pPropertyNode->createProperty("ModulatorID")->linkToProxy(PropertyProxyReference<int>(m_ModulatorID, false));
        m_pPropertyNode->createProperty("ZoneID")->linkToProxy(PropertyProxyReference<int>(m_ZoneID, false));
        if(m_pPropertyNode->getProperty("interrupt/mode") == NULL) {
          PropertyNodePtr interruptNode = m_pPropertyNode->createProperty("interrupt");
          interruptNode->setFlag(PropertyNode::Archive, true);
          PropertyNodePtr interruptModeNode = interruptNode->createProperty("mode");
          interruptModeNode->setStringValue("ignore");
          interruptModeNode->setFlag(PropertyNode::Archive, true);
        }
      }
    }
  } // publishToPropertyTree

  void Device::increaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      m_pApartment->sendCommand(cmdIncreaseValue, *this);
    } else {
      m_pApartment->sendCommand(cmdIncreaseParam, *this);
    }
  } // increaseValue

  void Device::decreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      m_pApartment->sendCommand(cmdDecreaseValue, *this);
    } else {
      m_pApartment->sendCommand(cmdDecreaseParam, *this);
    }
  } // decreaseValue

  void Device::enable() {
    boost::shared_ptr<EnableDeviceCommandBusRequest> request(new EnableDeviceCommandBusRequest());
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  } // enable

  void Device::disable() {
    boost::shared_ptr<DisableDeviceCommandBusRequest> request(new DisableDeviceCommandBusRequest());
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  } // disable

  void Device::startDim(const bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      m_pApartment->sendCommand(cmdStartDimUp, *this);
    } else {
      m_pApartment->sendCommand(cmdStartDimDown, *this);
    }
  } // startDim

  void Device::endDim(const int _parameterNr) {
    m_pApartment->sendCommand(cmdStopDim, *this);
  } // endDim

  bool Device::isOn() const {
    return (m_LastCalledScene != SceneOff) &&
           (m_LastCalledScene != SceneMin) &&
           (m_LastCalledScene != SceneDeepOff) &&
           (m_LastCalledScene != SceneStandBy);
  } // isOn

  int Device::getFunctionID() const {
    return m_FunctionID;
  } // getFunctionID

  void Device::setFunctionID(const int _value) {
    m_FunctionID = _value;
  } // setFunctionID

  bool Device::hasSwitch() const {
    return getFunctionID() == FunctionIDSwitch;
  } // hasSwitch

  void Device::setValue(const double _value, const int _parameterNr) {
    if(_parameterNr == -1) {
      m_pApartment->sendCommand(cmdSetValue, *this, static_cast<int>(_value));
    } else {
      DSS::getInstance()->getDS485Interface().setValueDevice(*this, (int)_value, _parameterNr, 1);
    }
  } // setValue

  void Device::setRawValue(const uint16_t _value, const int _parameterNr, const int _size) {
    DSS::getInstance()->getDS485Interface().setValueDevice(*this, _value, _parameterNr, _size);
  } // setRawValue

  double Device::getValue(const int _parameterNr) {
    vector<int> res = DSS::getInstance()->getDS485Interface().sendCommand(cmdGetValue, *this, _parameterNr);
    return res.front();
  } // getValue

  void Device::nextScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // nextScene

  void Device::previousScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // previousScene

  std::string Device::getName() const {
    return m_Name;
  } // getName

  void Device::setName(const std::string& _name) {
    if(m_Name != _name) {
      m_Name = _name;
      dirty();
    }
  } // setName

  void Device::dirty() {
    if(m_pApartment != NULL) {
      m_pApartment->addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    }
  } // dirty

  bool Device::operator==(const Device& _other) const {
    return _other.m_DSID == m_DSID;
  } // operator==

  devid_t Device::getShortAddress() const {
    return m_ShortAddress;
  } // getShortAddress

  void Device::setShortAddress(const devid_t _shortAddress) {
    m_ShortAddress = _shortAddress;
    publishToPropertyTree();
    m_LastDiscovered = DateTime();
  } // setShortAddress

  dsid_t Device::getDSID() const {
    return m_DSID;
  } // getDSID;

  int Device::getModulatorID() const {
    return m_ModulatorID;
  } // getModulatorID

  void Device::setModulatorID(const int _modulatorID) {
    m_ModulatorID = _modulatorID;
  } // setModulatorID

  int Device::getZoneID() const {
  	return m_ZoneID;
  } // getZoneID

  void Device::setZoneID(const int _value) {
    if(_value != m_ZoneID) {
      m_ZoneID = _value;
      if((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
        std::string basePath = "zones/zone" + intToString(m_ZoneID);
        if(m_pAliasNode == NULL) {
          PropertyNodePtr node = m_pApartment->getPropertyNode()->getProperty(basePath + "/" + m_DSID.toString());
          if(node != NULL) {
            Logger::getInstance()->log("Device::setZoneID: Target node for device " + m_DSID.toString() + " already exists", lsError);
            if(node->size() > 0) {
              Logger::getInstance()->log("Device::setZoneID: Target node for device " + m_DSID.toString() + " has children", lsFatal);
              return;
            }
          }
          m_pAliasNode = m_pApartment->getPropertyNode()->createProperty(basePath + "/" + m_DSID.toString());
          
          m_pAliasNode->alias(m_pPropertyNode);
        } else {
          PropertyNodePtr base = m_pApartment->getPropertyNode()->getProperty(basePath);
          if(base == NULL) {
            throw std::runtime_error("PropertyNode of the new zone does not exist");
          }
          base->addChild(m_pAliasNode);
        }
      }
    }
  } // setZoneID

  int Device:: getGroupIdByIndex(const int _index) const {
    return m_Groups[_index];
  } // getGroupIdByIndex

  Group& Device::getGroupByIndex(const int _index) {
    return m_pApartment->getGroup(getGroupIdByIndex(_index));
  } // getGroupByIndex

  void Device::addToGroup(const int _groupID) {
    m_GroupBitmask.set(_groupID-1);
    if(find(m_Groups.begin(), m_Groups.end(), _groupID) == m_Groups.end()) {
      m_Groups.push_back(_groupID);
    } else {
      Logger::getInstance()->log("Device " + m_DSID.toString() + " (bus: " + intToString(m_ShortAddress) + ", zone: " + intToString(m_ZoneID) + ") is already in group " + intToString(_groupID));
    }
  } // addToGroup

  void Device::removeFromGroup(const int _groupID) {
    m_GroupBitmask.reset(_groupID-1);
    std::vector<int>::iterator it = find(m_Groups.begin(), m_Groups.end(), _groupID);
    if(it != m_Groups.end()) {
      m_Groups.erase(it);
    }
  } // removeFromGroup

  void Device::resetGroups() {
    m_GroupBitmask.reset();
    m_Groups.clear();
  } // resetGroups

  int Device::getGroupsCount() const {
    return m_Groups.size();
  } // getGroupsCount

  std::bitset<63>& Device::getGroupBitmask() {
    return m_GroupBitmask;
  } // getGroupBitmask

  const std::bitset<63>& Device::getGroupBitmask() const {
    return m_GroupBitmask;
  } // getGroupBitmask

  bool Device::isInGroup(const int _groupID) const {
    return (_groupID == 0) || m_GroupBitmask.test(_groupID - 1);
  } // isInGroup

  Apartment& Device::getApartment() const {
    return *m_pApartment;
  } // getApartment

  unsigned long Device::getPowerConsumption() {
    return m_Consumption;
  } // getPowerConsumption

  uint8_t Device::dsLinkSend(uint8_t _value, bool _lastByte, bool _writeOnly) {
    uint8_t flags = 0;
    if(_lastByte) {
      flags |= DSLinkSendLastByte;
    }
    if(_writeOnly) {
      flags |= DSLinkSendWriteOnly;
    }
    return DSS::getInstance()->getDS485Interface().dSLinkSend(m_ModulatorID, m_ShortAddress, _value, flags);
  } // dsLinkSend

  int Device::getSensorValue(const int _sensorID) {
    return DSS::getInstance()->getDS485Interface().getSensorValue(*this,_sensorID);
  } // getSensorValue

  //================================================== Set

  Set::Set() {
  } // ctor

  Set::Set(Device& _device) {
    m_ContainedDevices.push_back(DeviceReference(_device, &_device.getApartment()));
  } // ctor(Device)

  Set::Set(DeviceVector _devices) {
    m_ContainedDevices = _devices;
  } // ctor(DeviceVector)

  Set::Set(const Set& _copy) {
    m_ContainedDevices = _copy.m_ContainedDevices;
  }

  void Set::turnOn() {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdTurnOn, *this);
  } // turnOn

  void Set::turnOff() {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdTurnOff, *this);
  } // turnOff

  void Set::increaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdIncreaseValue, *this);
    } else {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdIncreaseParam, *this);
    }
  } // increaseValue

  void Set::decreaseValue(const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdDecreaseValue, *this);
    } else {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdDecreaseParam, *this);
    }
  } // decreaseValue

  void Set::startDim(bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdStartDimUp, *this);
    } else {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdStartDimDown, *this);
    }
  } // startDim

  void Set::endDim(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdStopDim, *this);
  } // endDim

  void Set::setValue(const double _value, int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdSetValue, *this, (int)_value);
    } else {
      throw std::runtime_error("Can't set arbitrary parameter on a set");
    }
  } // setValue

  void Set::callScene(const int _sceneNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdCallScene, *this, _sceneNr);
  } // callScene

  void Set::saveScene(const int _sceneNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdSaveScene, *this, _sceneNr);
  } // saveScene

  void Set::undoScene(const int _sceneNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdUndoScene, *this, _sceneNr);
  } // undoScene

  void Set::nextScene() {
    throw std::runtime_error("Not yet implemented");
  } // nextScene

  void Set::previousScene() {
    throw std::runtime_error("Not yet implemented");
  } // previousScene

  void Set::perform(IDeviceAction& _deviceAction) {
    for(DeviceIterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      _deviceAction.perform(iDevice->getDevice());
    }
  } // perform

  Set Set::getSubset(const IDeviceSelector& _selector) const {
    Set result;
    foreach(DeviceReference iDevice, m_ContainedDevices) {
      if(_selector.selectDevice(iDevice.getDevice())) {
        result.addDevice(iDevice);
      }
    }
    return result;
  } // getSubset

  class ByGroupSelector : public IDeviceSelector {
  private:
    const int m_GroupNr;
  public:
    ByGroupSelector(const int _groupNr)
    : m_GroupNr(_groupNr)
    {}

    virtual ~ByGroupSelector() {}

    virtual bool selectDevice(const Device& _device) const {
      return _device.isInGroup(m_GroupNr);
    }
  };

  Set Set::getByGroup(int _groupNr) const {
    if(_groupNr != GroupIDBroadcast) {
      return getSubset(ByGroupSelector(_groupNr));
    } else {
      return *this;
    }
  } // getByGroup(id)

  Set Set::getByGroup(const Group& _group) const {
    return getByGroup(_group.getID());
  } // getByGroup(ref)

  Set Set::getByGroup(const std::string& _name) const {
    Set result;
    if(isEmpty()) {
      return result;
    } else {
      Group& g = get(0).getDevice().getApartment().getGroup(_name);
      return getByGroup(g.getID());
    }
  } // getByGroup(name)

  Set Set::getByZone(int _zoneID) const {
    if(_zoneID != 0) {
      Set result;
      foreach(const DeviceReference& dev, m_ContainedDevices) {
        if(dev.getDevice().getZoneID() == _zoneID) {
          result.addDevice(dev);
        }
      }
      return result;
    } else {
      // return a copy of this set if the broadcast zone was requested
      return *this;
    }
  } // getByZone(id)

  Set Set::getByZone(const std::string& _zoneName) const {
    Set result;
    if(isEmpty()) {
      return result;
    } else {
      Zone& zone = get(0).getDevice().getApartment().getZone(_zoneName);
      return getByZone(zone.getID());
    }
  } // getByZone(name)

  Set Set::getByModulator(const int _modulatorID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice().getModulatorID() == _modulatorID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByModulator

  Set Set::getByModulator(const Modulator& _modulator) const {
    return getByModulator(_modulator.getBusID());
  } // getByModulator

  Set Set::getByFunctionID(const int _functionID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice().getFunctionID() == _functionID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByFunctionID

  Set Set::getByPresence(const bool _presence) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice().isPresent()== _presence) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByPresence

  class ByNameSelector : public IDeviceSelector {
  private:
    const std::string m_Name;
  public:
    ByNameSelector(const std::string& _name) : m_Name(_name) {};
    virtual ~ByNameSelector() {};

    virtual bool selectDevice(const Device& _device) const {
      return _device.getName() == m_Name;
    }
  };

  DeviceReference Set::getByName(const std::string& _name) const {
    Set resultSet = getSubset(ByNameSelector(_name));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException(_name);
    }
    return resultSet.m_ContainedDevices.front();
  } // getByName


  class ByIDSelector : public IDeviceSelector {
  private:
    const devid_t m_ID;
  public:
    ByIDSelector(const devid_t _id) : m_ID(_id) {}
    virtual ~ByIDSelector() {};

    virtual bool selectDevice(const Device& _device) const {
      return _device.getShortAddress() == m_ID;
    }
  };

  DeviceReference Set::getByBusID(const devid_t _id) const {
    Set resultSet = getSubset(ByIDSelector(_id));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException(std::string("with busid ") + intToString(_id));
    }
    return resultSet.m_ContainedDevices.front();
  } // getByBusID

  class ByDSIDSelector : public IDeviceSelector {
  private:
    const dsid_t m_ID;
  public:
    ByDSIDSelector(const dsid_t _id) : m_ID(_id) {}
    virtual ~ByDSIDSelector() {};

    virtual bool selectDevice(const Device& _device) const {
      return _device.getDSID() == m_ID;
    }
  };

  DeviceReference Set::getByDSID(const dsid_t _dsid) const {
    Set resultSet = getSubset(ByDSIDSelector(_dsid));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException("with dsid " + _dsid.toString());
    }
    return resultSet.m_ContainedDevices.front();
  } // getByDSID

  int Set::length() const {
    return m_ContainedDevices.size();
  } // length

  bool Set::isEmpty() const {
    return length() == 0;
  } // isEmpty

  Set Set::combine(Set& _other) const {
    Set resultSet(_other);
    foreach(const DeviceReference& iDevice, m_ContainedDevices) {
      if(!resultSet.contains(iDevice)) {
        resultSet.addDevice(iDevice);
      }
    }
    return resultSet;
  } // combine

  Set Set::remove(const Set& _other) const {
    Set resultSet(*this);
    foreach(const DeviceReference& iDevice, _other.m_ContainedDevices) {
      resultSet.removeDevice(iDevice);
    }
    return resultSet;
  } // remove

  bool Set::contains(const DeviceReference& _device) const {
    DeviceVector::const_iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    return pos != m_ContainedDevices.end();
  } // contains

  bool Set::contains(const Device& _device) const {
    return contains(DeviceReference(_device, &_device.getApartment()));
  } // contains

  void Set::addDevice(const DeviceReference& _device) {
    if(!contains(_device)) {
      m_ContainedDevices.push_back(_device);
    }
  } // addDevice

  void Set::addDevice(const Device& _device) {
    addDevice(DeviceReference(_device, &_device.getApartment()));
  } // addDevice

  void Set::removeDevice(const DeviceReference& _device) {
    DeviceVector::iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    if(pos != m_ContainedDevices.end()) {
      m_ContainedDevices.erase(pos);
    }
  } // removeDevice

  void Set::removeDevice(const Device& _device) {
    removeDevice(DeviceReference(_device, &_device.getApartment()));
  } // removeDevice

  const DeviceReference& Set::get(int _index) const {
    return m_ContainedDevices.at(_index);
  } // get

  const DeviceReference& Set::operator[](const int _index) const {
    return get(_index);
  } // operator[]

  DeviceReference& Set::get(int _index) {
    return m_ContainedDevices.at(_index);
  } // get

  DeviceReference& Set::operator[](const int _index) {
    return get(_index);
  } // operator[]

  unsigned long Set::getPowerConsumption() {
    unsigned long result = 0;
    foreach(DeviceReference& iDevice, m_ContainedDevices) {
      result += iDevice.getPowerConsumption();
    }
    return result;
  } // getPowerConsumption

  std::ostream& operator<<(std::ostream& out, const Device& _dt) {
    out << "Device ID " << _dt.getShortAddress();
    if(!_dt.getName().empty()) {
      out << " name: " << _dt.getName();
    }
    return out;
  } // operator<<


  //================================================== Apartment

  Apartment::Apartment(DSS* _pDSS, DS485Interface* _pDS485Interface)
  : Subsystem(_pDSS, "Apartment"),
    Thread("Apartment"),
    m_IsInitializing(true),
    m_pPropertyNode(),
    m_pDS485Interface(_pDS485Interface),
    m_pBusRequestDispatcher(NULL)
  { } // ctor

  Apartment::~Apartment() {
    scrubVector(m_Devices);
    scrubVector(m_Zones);
    scrubVector(m_Modulators);
  } // dtor

  void Apartment::initialize() {
    Subsystem::initialize();
    // create default zone
    Zone* zoneZero = new Zone(0);
    addDefaultGroupsToZone(*zoneZero);
    m_Zones.push_back(zoneZero);
    zoneZero->setIsPresent(true);
    if(DSS::hasInstance()) {
      m_pPropertyNode = DSS::getInstance()->getPropertySystem().createProperty("/apartment");
      DSS::getInstance()->getPropertySystem().setStringValue(getConfigPropertyBasePath() + "configfile", getDSS().getDataDirectory() + "apartment.xml", true, false);
    }
  } // initialize

  void Apartment::doStart() {
    run();
  } // start

  void Apartment::addDefaultGroupsToZone(Zone& _zone) {
    int zoneID = _zone.getID();

    Group* grp = new Group(GroupIDBroadcast, zoneID, *this);
    grp->setName("broadcast");
    _zone.addGroup(grp);
    grp = new Group(GroupIDYellow, zoneID, *this);
    grp->setName("yellow");
    _zone.addGroup(grp);
    grp = new Group(GroupIDGray, zoneID, *this);
    grp->setName("gray");
    _zone.addGroup(grp);
    grp = new Group(GroupIDBlue, zoneID, *this);
    grp->setName("blue");
    _zone.addGroup(grp);
    grp = new Group(GroupIDCyan, zoneID, *this);
    grp->setName("cyan");
    _zone.addGroup(grp);
    grp = new Group(GroupIDRed, zoneID, *this);
    grp->setName("red");
    _zone.addGroup(grp);
    grp = new Group(GroupIDViolet, zoneID, *this);
    grp->setName("magenta");
    _zone.addGroup(grp);
    grp = new Group(GroupIDGreen, zoneID, *this);
    grp->setName("green");
    _zone.addGroup(grp);
    grp = new Group(GroupIDBlack, zoneID, *this);
    grp->setName("black");
    _zone.addGroup(grp);
    grp = new Group(GroupIDWhite, zoneID, *this);
    grp->setName("white");
    _zone.addGroup(grp);
    grp = new Group(GroupIDDisplay, zoneID, *this);
    grp->setName("display");
    _zone.addGroup(grp);
  } // addDefaultGroupsToZone

  bool Apartment::scanModulator(Modulator& _modulator) {
    _modulator.setIsPresent(true);
    _modulator.setIsValid(false);

    DS485Interface& interface = DSS::getInstance()->getDS485Interface();
    int modulatorID = _modulator.getBusID();

    log("scanModulator: Start " + intToString(modulatorID) , lsInfo);
    vector<int> zoneIDs;
    try {
      zoneIDs = interface.getZones(modulatorID);
    } catch(DS485ApiError& e) {
      log("scanModulator: Error getting ZoneIDs", lsFatal);
      return false;
    }

    int levelOrange, levelRed;
    try {
      if(interface.getEnergyBorder(modulatorID, levelOrange, levelRed)) {
        _modulator.setEnergyLevelOrange(levelOrange);
        _modulator.setEnergyLevelRed(levelRed);
      }
    } catch(DS485ApiError& e) {
      log("scanModulator: Error getting EnergyLevels", lsFatal);
      return false;
    }

    try {
      ModulatorSpec_t spec = interface.getModulatorSpec(modulatorID);
      _modulator.setSoftwareVersion(spec.get<1>());
      _modulator.setHardwareVersion(spec.get<2>());
      _modulator.setHardwareName(spec.get<3>());
      _modulator.setDeviceType(spec.get<4>());
    } catch(DS485ApiError& e) {
      log("scanModulator: Error getting dSMSpecs", lsFatal);
      return false;
    }

    bool firstZone = true;
    foreach(int zoneID, zoneIDs) {
      log("scanModulator:  Found zone with id: " + intToString(zoneID));
      Zone& zone = allocateZone(zoneID);
      zone.addToModulator(_modulator);
      zone.setIsPresent(true);
      if(firstZone) {
        zone.setFirstZoneOnModulator(modulatorID);
        firstZone = false;
      }
      vector<int> devices;
      try {
        devices = interface.getDevicesInZone(modulatorID, zoneID);
      } catch(DS485ApiError& e) {
        log("scanModulator: Error getting getDevicesInZone", lsFatal);
        return false;
      }
      foreach(int devID, devices) {
        dsid_t dsid;
        try {
          dsid = interface.getDSIDOfDevice(modulatorID, devID);
        } catch(DS485ApiError& e) {
            log("scanModulator: Error getting getDSIDOfDevice", lsFatal);
            return false;
        }

        vector<int> results;
        try {
          results = interface.sendCommand(cmdGetFunctionID, devID, modulatorID);
        } catch(DS485ApiError& e) {
          log("scanModulator: Error getting cmdGetFunctionID", lsFatal);
          return false;
        }
        int functionID = 0;
        if(results.size() == 1) {
          functionID = results.front();
        }
        log("scanModulator:    Found device with id: " + intToString(devID));
        log("scanModulator:    DSID:        " + dsid.toString());
        log("scanModulator:    Function ID: " + unsignedLongIntToHexString(functionID));
        Device& dev = allocateDevice(dsid);
        dev.setShortAddress(devID);
        dev.setModulatorID(modulatorID);
        dev.setZoneID(zoneID);
        dev.setFunctionID(functionID);

        std::vector<int> groupIdperDevices = interface.getGroupsOfDevice(modulatorID, devID);
        vector<int> groupIDsPerDevice = interface.getGroupsOfDevice(modulatorID,devID);
        foreach(int groupID, groupIDsPerDevice) {
          log(std::string("scanModulator: adding device ") + intToString(devID) + " to group " + intToString(groupID));
          dev.addToGroup(groupID);
        }

        DeviceReference devRef(dev, this);
        zone.addDevice(devRef);
        _modulator.addDevice(devRef);
        dev.setIsPresent(true);
      }
      vector<int> groupIDs;
      try {
        groupIDs = interface.getGroups(modulatorID, zoneID);
      } catch(DS485ApiError& e) {
        log("scanModulator: Error getting getGroups", lsFatal);
        return false;
      }

      foreach(int groupID, groupIDs) {
        if(groupID == 0) {
          log("scanModulator:    Group ID is zero, bailing out... (modulatorID: "
              + intToString(modulatorID) + 
              "zoneID: " + intToString(zoneID) + ")",
              lsError);
          continue;
        }
        log("scanModulator:    Found group with id: " + intToString(groupID));
        if(zone.getGroup(groupID) == NULL) {
          log(" scanModulator:    Adding new group to zone");
          zone.addGroup(new Group(groupID, zone.getID(), *this));
        }
        try {
          Group& group = getGroup(groupID);
          group.setIsPresent(true);
        } catch(ItemNotFoundException&) {
          Group* pGroup = new Group(groupID, 0, *this);
          getZone(0).addGroup(pGroup);
          pGroup->setIsPresent(true);
          log("scanModulator:     Adding new group to zone 0");
        }

        // get last called scene for zone, group
        try {
          int lastCalledScene = interface.getLastCalledScene(modulatorID, zoneID, groupID);
          Group* pGroup = zone.getGroup(groupID);
          assert(pGroup != NULL);
          log("scanModulator: zoneID: " + intToString(zoneID) + " groupID: " + intToString(groupID) + " lastScene: " + intToString(lastCalledScene));
          if(lastCalledScene < 0 || lastCalledScene > MaxSceneNumber) {
            log("scanModulator: _sceneID is out of bounds. zoneID: " + intToString(zoneID) + " groupID: " + intToString(groupID) + " scene: " + intToString(lastCalledScene), lsError);
          } else {
            onGroupCallScene(zoneID, groupID, lastCalledScene);
          }
        } catch(DS485ApiError& error) {
          log(std::string("scanModulator: Error getting last called scene '") + error.what() + "'", lsError);
        }
      }
    }
    _modulator.setIsValid(true);
    return true;

  } // scanModulator

  void Apartment::modulatorReady(int _modulatorBusID) {
    log("Modulator with id: " + intToString(_modulatorBusID) + " is ready", lsInfo);
    try {
      try {
        Modulator& mod = getModulatorByBusID(_modulatorBusID);
        if(scanModulator(mod)) {
          boost::shared_ptr<Event> modulatorReadyEvent(new Event("modulator_ready"));
          modulatorReadyEvent->setProperty("modulator", mod.getDSID().toString());
          raiseEvent(modulatorReadyEvent);
        }
      } catch(DS485ApiError& e) {
        log(std::string("Exception caught while scanning modulator " + intToString(_modulatorBusID) + " : ") + e.what(), lsFatal);

        ModelEvent* pEvent = new ModelEvent(ModelEvent::etModulatorReady);
        pEvent->addParameter(_modulatorBusID);
        addModelEvent(pEvent);
      }
    } catch(ItemNotFoundException& e) {
      log("No modulator for bus-id (" + intToString(_modulatorBusID) + ") found, re-discovering devices");
      discoverDS485Devices();
    }
  } // modulatorReady

  void Apartment::setPowerConsumption(int _modulatorBusID, unsigned long _value) {
    getModulatorByBusID(_modulatorBusID).setPowerConsumption(_value);
  } // powerConsumption

  void Apartment::setEnergyMeterValue(int _modulatorBusID, unsigned long _value) {
    getModulatorByBusID(_modulatorBusID).setEnergyMeterValue(_value);
  } // energyMeterValue

  void Apartment::discoverDS485Devices() {
    // temporary mark all modulators as absent
    foreach(Modulator* pModulator, m_Modulators) {
      pModulator->setIsPresent(false);
    }

    // Request the dsid of all modulators
    DS485CommandFrame requestFrame;
    requestFrame.getHeader().setBroadcast(true);
    requestFrame.getHeader().setDestination(0);
    requestFrame.setCommand(CommandRequest);
    requestFrame.getPayload().add<uint8_t>(FunctionModulatorGetDSID);
    if(DSS::hasInstance()) {
      DSS::getInstance()->getDS485Interface().sendFrame(requestFrame);
    }
  } // discoverDS485Devices

  void Apartment::writeConfiguration() {
    if(DSS::hasInstance()) {
      writeConfigurationToXML(DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile"));
    }
  } // writeConfiguration

  void Apartment::handleModelEvents() {
    if(!m_ModelEvents.empty()) {
      ModelEvent& event = m_ModelEvents.front();
      switch(event.getEventType()) {
      case ModelEvent::etNewDevice:
        if(event.getParameterCount() != 4) {
          log("Expected exactly 4 parameter for ModelEvent::etNewDevice");
        } else {
          onAddDevice(event.getParameter(0), event.getParameter(1), event.getParameter(2), event.getParameter(3));
        }
        break;
      case ModelEvent::etCallSceneDevice:
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etCallSceneDevice");
        } else {
          onDeviceCallScene(event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etCallSceneGroup:
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etCallSceneGroup");
        } else {
          onGroupCallScene(event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etModelDirty:
        writeConfiguration();
        break;
      case ModelEvent::etDSLinkInterrupt:
        if(event.getParameterCount() != 3) {
          log("Expected exactly 3 parameter for ModelEvent::etDSLinkInterrupt");
        } else {
          onDSLinkInterrupt(event.getParameter(0), event.getParameter(1), event.getParameter(2));
        }
        break;
      case ModelEvent::etNewModulator:
        discoverDS485Devices();
        break;
      case ModelEvent::etLostModulator:
        discoverDS485Devices();
        break;
      case ModelEvent::etModulatorReady:
        if(event.getParameterCount() != 1) {
          log("Expected exactly 1 parameter for ModelEvent::etModulatorReady");
        } else {
          try{
            Modulator& mod = getModulatorByBusID(event.getParameter(0));
            mod.setIsPresent(true);
            mod.setIsValid(false);
          } catch(ItemNotFoundException& e) {
            log("dSM is ready, but it is not yet known, re-discovering devices");
            discoverDS485Devices();
          }
        }
        break;
      case ModelEvent::etBusReady:
        log("Got bus ready event.", lsInfo);
        discoverDS485Devices();
        break;
      case ModelEvent::etPowerConsumption:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etPowerConsumption");
        } else {
          setPowerConsumption(event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etEnergyMeterValue:
        if(event.getParameterCount() != 2) {
          log("Expected exactly 2 parameter for ModelEvent::etEnergyMeterValue");
        } else {
          setEnergyMeterValue(event.getParameter(0), event.getParameter(1));
        }
        break;
      case ModelEvent::etDS485DeviceDiscovered:
        if(event.getParameterCount() != 7) {
          log("Expected exactly 7 parameter for ModelEvent::etDS485DeviceDiscovered");
        } else {
          int busID = event.getParameter(0);
          uint64_t dsidUpper = (uint64_t(event.getParameter(1)) & 0x00ffff) << 48;
          dsidUpper |= (uint64_t(event.getParameter(2)) & 0x00ffff) << 32;
          dsidUpper |= (uint64_t(event.getParameter(3))  & 0x00ffff) << 16;
          dsidUpper |= (uint64_t(event.getParameter(4)) & 0x00ffff);
          dsid_t newDSID(dsidUpper,
                         ((uint32_t(event.getParameter(5)) & 0x00ffff) << 16) | (uint32_t(event.getParameter(6)) & 0x00ffff));
          log ("Discovered device with busID: " + intToString(busID) + " and dsid: " + newDSID.toString());
          try{
             getModulatorByDSID(newDSID).setBusID(busID);
             log ("dSM present");
             getModulatorByDSID(newDSID).setIsPresent(true);
          } catch(ItemNotFoundException& e) {
             log ("dSM not present");
             Modulator& modulator = allocateModulator(newDSID);
             modulator.setBusID(busID);
             modulator.setIsPresent(true);
             modulator.setIsValid(false);
             ModelEvent* pEvent = new ModelEvent(ModelEvent::etModulatorReady);
             pEvent->addParameter(busID);
             addModelEvent(pEvent);
          }
        }
        break;
      default:
        assert(false);
        break;
      }

      m_ModelEventsMutex.lock();
      m_ModelEvents.erase(m_ModelEvents.begin());
      m_ModelEventsMutex.unlock();
    } else {
      m_NewModelEvent.waitFor(1000);
      bool hadToUpdate = false;
      foreach(Modulator* pModulator, m_Modulators) {
        if(pModulator->isPresent()) {
          if(!pModulator->isValid()) {
            modulatorReady(pModulator->getBusID());
            hadToUpdate = true;
            break;
          }
        }
      }

      // If we didn't have to update for one cycle, assume that we're done
      if(!hadToUpdate && m_IsInitializing) {
        log("******** Finished loading model from dSM(s)...", lsInfo);
        m_IsInitializing = false;

        {
          boost::shared_ptr<Event> readyEvent(new Event("model_ready"));
          raiseEvent(readyEvent);
        }
      }
    }
  } // handleModelEvents

  void Apartment::readConfiguration() {
    if(DSS::hasInstance()) {
      std::string configFileName = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "configfile");
      if(!boost::filesystem::exists(configFileName)) {
        log(std::string("Apartment::execute: Could not open config-file for apartment: '") + configFileName + "'", lsWarning);
      } else {
        readConfigurationFromXML(configFileName);
      }
    }
  } // readConfiguration

  void Apartment::raiseEvent(boost::shared_ptr<Event> _pEvent) {
    if(DSS::hasInstance()) {
      getDSS().getEventQueue().pushEvent(_pEvent);
    }
  } // raiseEvent

  void Apartment::waitForInterface() {
    if(DSS::hasInstance()) {
      DS485Interface& interface = DSS::getInstance()->getDS485Interface();

      log("Apartment::execute: Waiting for interface to get ready", lsInfo);

      while(!interface.isReady() && !m_Terminated) {
        sleepMS(1000);
      }
    }

    boost::shared_ptr<Event> readyEvent(new Event("interface_ready"));
    raiseEvent(readyEvent);
  } // waitForInterface

  void Apartment::execute() {
    {
      boost::shared_ptr<Event> runningEvent(new Event("running"));
      raiseEvent(runningEvent);
    }

    // load devices/modulators/etc. from a config-file
    readConfiguration();

    {
      boost::shared_ptr<Event> configReadEvent(new Event("config_read"));
      raiseEvent(configReadEvent);
    }

    waitForInterface();

    log("Apartment::execute: Interface is ready, enumerating model", lsInfo);
    discoverDS485Devices();

    while(!m_Terminated) {
      handleModelEvents();
    }
  } // run

  void Apartment::addModelEvent(ModelEvent* _pEvent) {
    // filter out dirty events, as this will rewrite apartment.xml
    if(m_IsInitializing && (_pEvent->getEventType() == ModelEvent::etModelDirty)) {
      delete _pEvent;
    } else {
      m_ModelEventsMutex.lock();
      m_ModelEvents.push_back(_pEvent);
      m_ModelEventsMutex.unlock();
      m_NewModelEvent.signal();
    }
  } // addModelEvent

  const int ApartmentConfigVersion = 1;

  void Apartment::readConfigurationFromXML(const std::string& _fileName) {
    setName("dSS");
    std::ifstream inFile(_fileName.c_str());

    InputSource input(inFile);
    DOMParser parser;
    AutoPtr<Document> pDoc = parser.parse(&input);
    Element* rootNode = pDoc->documentElement();

    if(rootNode->localName() == "config") {
      if(rootNode->hasAttribute("version") && (strToInt(rootNode->getAttribute("version")) == ApartmentConfigVersion)) {
        Node* curNode = rootNode->firstChild();
        while(curNode != NULL) {
          std::string nodeName = curNode->localName();
          if(nodeName == "devices") {
            loadDevices(curNode);
          } else if(nodeName == "modulators") {
            loadModulators(curNode);
          } else if(nodeName == "zones") {
            loadZones(curNode);
          } else if(nodeName == "apartment") {
            Element* elem = dynamic_cast<Element*>(curNode);
            if(elem != NULL) {
              Element* nameElem = elem->getChildElement("name");
              if(nameElem->hasChildNodes()) {
                setName(nameElem->firstChild()->nodeValue());
              }
            }
          }
          curNode = curNode->nextSibling();
        }
      } else {
        log("Config file has the wrong version");
      }
    }
  } // readConfigurationFromXML

  void Apartment::loadDevices(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "device") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("dsid")) {
          dsid_t dsid = dsid_t::fromString(elem->getAttribute("dsid"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }

          DateTime firstSeen;
          if(elem->hasAttribute("firstSeen")) {
            firstSeen = DateTime(dateFromISOString(elem->getAttribute("firstSeen").c_str()));
          }

          Device& newDevice = allocateDevice(dsid);
          if(!name.empty()) {
            newDevice.setName(name);
          }
          newDevice.setFirstSeen(firstSeen);
          Element* propertiesElem = elem->getChildElement("properties");
          if(propertiesElem != NULL) {
            newDevice.publishToPropertyTree();
            newDevice.getPropertyNode()->loadChildrenFromNode(propertiesElem);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadDevices

  void Apartment::loadModulators(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "modulator") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("id")) {
          dsid_t id = dsid_t::fromString(elem->getAttribute("id"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }
          Modulator& newModulator = allocateModulator(id);
          if(!name.empty()) {
            newModulator.setName(name);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadModulators

  void Apartment::loadZones(Node* _node) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "zone") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if((elem != NULL) && elem->hasAttribute("id")) {
          int id = strToInt(elem->getAttribute("id"));
          std::string name;
          Element* nameElem = elem->getChildElement("name");
          if((nameElem != NULL) && nameElem->hasChildNodes()) {
            name = nameElem->firstChild()->nodeValue();
          }
          Zone& newZone = allocateZone(id);
          if(!name.empty()) {
            newZone.setName(name);
          }
        }
      }
      curNode = curNode->nextSibling();
    }
  } // loadZones

  void DeviceToXML(const Device* _pDevice, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pDeviceNode = _pDocument->createElement("device");
    pDeviceNode->setAttribute("dsid", _pDevice->getDSID().toString());
    if(!_pDevice->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pDevice->getName());
      pNameNode->appendChild(txtNode);
      pDeviceNode->appendChild(pNameNode);
    }    
    pDeviceNode->setAttribute("firstSeen", _pDevice->getFirstSeen());
    if(_pDevice->getPropertyNode() != NULL) {
      AutoPtr<Element> pPropertiesNode = _pDocument->createElement("properties");
      pDeviceNode->appendChild(pPropertiesNode);
      _pDevice->getPropertyNode()->saveChildrenAsXML(_pDocument, pPropertiesNode, PropertyNode::Archive);
    }

    _parentNode->appendChild(pDeviceNode);
  } // deviceToXML

  void ZoneToXML(const Zone* _pZone, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pZoneNode = _pDocument->createElement("zone");
    pZoneNode->setAttribute("id", intToString(_pZone->getID()));
    if(!_pZone->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pZone->getName());
      pNameNode->appendChild(txtNode);
      pZoneNode->appendChild(pNameNode);
    }
    _parentNode->appendChild(pZoneNode);
  } // zoneToXML

  void ModulatorToXML(const Modulator* _pModulator, AutoPtr<Element>& _parentNode, AutoPtr<Document>& _pDocument) {
    AutoPtr<Element> pModulatorNode = _pDocument->createElement("modulator");
    pModulatorNode->setAttribute("id", _pModulator->getDSID().toString());
    if(!_pModulator->getName().empty()) {
      AutoPtr<Element> pNameNode = _pDocument->createElement("name");
      AutoPtr<Text> txtNode = _pDocument->createTextNode(_pModulator->getName());
      pNameNode->appendChild(txtNode);
      pModulatorNode->appendChild(pNameNode);
    }
    _parentNode->appendChild(pModulatorNode);
  } // modulatorToXML

  void Apartment::writeConfigurationToXML(const std::string& _fileName) {
    log("Writing apartment config to '" + _fileName + "'", lsInfo);
    AutoPtr<Document> pDoc = new Document;

    AutoPtr<ProcessingInstruction> pXMLHeader = pDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    pDoc->appendChild(pXMLHeader);

    AutoPtr<Element> pRoot = pDoc->createElement("config");
    pRoot->setAttribute("version", intToString(ApartmentConfigVersion));
    pDoc->appendChild(pRoot);

    // apartment
    AutoPtr<Element> pApartment = pDoc->createElement("apartment");
    pRoot->appendChild(pApartment);
    AutoPtr<Element> pApartmentName = pDoc->createElement("name");
    AutoPtr<Text> pApartmentNameText = pDoc->createTextNode(getName());
    pApartmentName->appendChild(pApartmentNameText);
    pApartment->appendChild(pApartmentName);

    // devices
    AutoPtr<Element> pDevices = pDoc->createElement("devices");
    pRoot->appendChild(pDevices);
    foreach(Device* pDevice, m_Devices) {
      DeviceToXML(pDevice, pDevices, pDoc);
    }

    // zones
    AutoPtr<Element> pZones = pDoc->createElement("zones");
    pRoot->appendChild(pZones);
    foreach(Zone* pZone, m_Zones) {
      ZoneToXML(pZone, pZones, pDoc);
    }

    // modulators
    AutoPtr<Element> pModulators = pDoc->createElement("modulators");
    pRoot->appendChild(pModulators);
    foreach(Modulator* pModulator, m_Modulators) {
      ModulatorToXML(pModulator, pModulators, pDoc);
    }

    std::string tmpOut = _fileName + ".tmp";
    std::ofstream ofs(tmpOut.c_str());

    if(ofs) {
      DOMWriter writer;
      writer.setNewLine("\n");
      writer.setOptions(XMLWriter::PRETTY_PRINT);
      writer.writeNode(ofs, pDoc);

      ofs.close();

      // move it to the desired location
      rename(tmpOut.c_str(), _fileName.c_str());
    } else {
      log("Could not open file '" + tmpOut + "' for writing", lsFatal);
    }
  } // writeConfigurationToXML

  Device& Apartment::getDeviceByDSID(const dsid_t _dsid) const {
    foreach(Device* dev, m_Devices) {
      if(dev->getDSID() == _dsid) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDeviceByShortAddress const

  Device& Apartment::getDeviceByDSID(const dsid_t _dsid) {
    foreach(Device* dev, m_Devices) {
      if(dev->getDSID() == _dsid) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDeviceByShortAddress

  Device& Apartment::getDeviceByShortAddress(const Modulator& _modulator, const devid_t _deviceID) const {
    foreach(Device* dev, m_Devices) {
      if((dev->getShortAddress() == _deviceID) &&
          (_modulator.getBusID() == dev->getModulatorID())) {
        return *dev;
      }
    }
    throw ItemNotFoundException(intToString(_deviceID));
  } // getDeviceByShortAddress

  Device& Apartment::getDeviceByName(const std::string& _name) {
    foreach(Device* dev, m_Devices) {
      if(dev->getName() == _name) {
        return *dev;
      }
    }
    throw ItemNotFoundException(_name);
  } // getDeviceByName

  Set Apartment::getDevices() const {
    DeviceVector devs;
    foreach(Device* dev, m_Devices) {
      devs.push_back(DeviceReference(*dev, this));
    }

    return Set(devs);
  } // getDevices

  Zone& Apartment::getZone(const std::string& _zoneName) {
    foreach(Zone* zone, m_Zones) {
      if(zone->getName() == _zoneName) {
        return *zone;
      }
    }
    throw ItemNotFoundException(_zoneName);
  } // getZone(name)

  Zone& Apartment::getZone(const int _id) {
    foreach(Zone* zone, m_Zones) {
      if(zone->getID() == _id) {
        return *zone;
      }
    }
    throw ItemNotFoundException(intToString(_id));
  } // getZone(id)

  vector<Zone*>& Apartment::getZones() {
    return m_Zones;
  } // getZones

  Modulator& Apartment::getModulator(const std::string& _modName) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->getName() == _modName) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(_modName);
  } // getModulator(name)

  Modulator& Apartment::getModulatorByBusID(const int _busId) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->getBusID() == _busId) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(intToString(_busId));
  } // getModulatorByBusID

  Modulator& Apartment::getModulatorByDSID(const dsid_t _dsid) {
    foreach(Modulator* modulator, m_Modulators) {
      if(modulator->getDSID() == _dsid) {
        return *modulator;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getModulatorByDSID

  vector<Modulator*>& Apartment::getModulators() {
    return m_Modulators;
  } // getModulators

  // Group queries
  Group& Apartment::getGroup(const std::string& _name) {
    Group* pResult = getZone(0).getGroup(_name);
    if(pResult != NULL) {
      return *pResult;
    }
    throw ItemNotFoundException(_name);
  } // getGroup(name)

  Group& Apartment::getGroup(const int _id) {
    Group* pResult = getZone(0).getGroup(_id);
    if(pResult != NULL) {
      return *pResult;
    }
    throw ItemNotFoundException(intToString(_id));
  } // getGroup(id)

  Device& Apartment::allocateDevice(const dsid_t _dsid) {
    // search for existing device
    foreach(Device* device, m_Devices) {
      if(device->getDSID() == _dsid) {
        DeviceReference devRef(*device, this);
        getZone(0).addDevice(devRef);
        return *device;
      }
    }

    Device* pResult = new Device(_dsid, this);
    pResult->setFirstSeen(DateTime());
    m_Devices.push_back(pResult);
    DeviceReference devRef(*pResult, this);
    getZone(0).addDevice(devRef);
    return *pResult;
  } // allocateDevice

  Modulator& Apartment::allocateModulator(const dsid_t _dsid) {
    foreach(Modulator* modulator, m_Modulators) {
      if((modulator)->getDSID() == _dsid) {
        return *modulator;
      }
    }

    Modulator* pResult = new Modulator(_dsid);
    m_Modulators.push_back(pResult);
    return *pResult;
  } // allocateModulator

  Zone& Apartment::allocateZone(int _zoneID) {
    if(getPropertyNode() != NULL) {
      getPropertyNode()->createProperty("zones/zone" + intToString(_zoneID));
    }

    foreach(Zone* zone, m_Zones) {
  		if(zone->getID() == _zoneID) {
  			return *zone;
  		}
  	}

  	Zone* zone = new Zone(_zoneID);
  	m_Zones.push_back(zone);
  	addDefaultGroupsToZone(*zone);
  	return *zone;
  } // allocateZone

  void Apartment::removeZone(int _zoneID) {
    for(std::vector<Zone*>::iterator ipZone = m_Zones.begin(), e = m_Zones.end();
        ipZone != e; ++ipZone) {
      Zone* pZone = *ipZone;
      if(pZone->getID() == _zoneID) {
        m_Zones.erase(ipZone);
        delete pZone;
        return;
      }
    }
  } // removeZone
  
  void Apartment::removeDevice(dsid_t _device) {
    for(std::vector<Device*>::iterator ipDevice = m_Devices.begin(), e = m_Devices.end();
        ipDevice != e; ++ipDevice) {
      Device* pDevice = *ipDevice;
      if(pDevice->getDSID() == _device) {
        m_Devices.erase(ipDevice);
        delete pDevice;
        return;
      }
    }
  } // removeDevice
  
  void Apartment::removeModulator(dsid_t _modulator) {
    for(std::vector<Modulator*>::iterator ipModulator = m_Modulators.begin(), e = m_Modulators.end();
        ipModulator != e; ++ipModulator) {
      Modulator* pModulator = *ipModulator;
      if(pModulator->getDSID() == _modulator) {
        m_Modulators.erase(ipModulator);
        delete pModulator;
        return;
      }
    }
  } // removeModulator

  class SetLastCalledSceneAction : public IDeviceAction {
  protected:
    int m_SceneID;
  public:
    SetLastCalledSceneAction(const int _sceneID)
    : m_SceneID(_sceneID) {}
    virtual ~SetLastCalledSceneAction() {}

    virtual bool perform(Device& _device) {
      _device.setLastCalledScene(m_SceneID);
      return true;
    }
  };

  void Apartment::onGroupCallScene(const int _zoneID, const int _groupID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onGroupCallScene: Scene number is out of bounds. zoneID: " + intToString(_zoneID) + " groupID: " + intToString(_groupID) + " scene: " + intToString(_sceneID), lsError);
        return;
      }
      Zone& zone = getZone(_zoneID);
      Group* group = zone.getGroup(_groupID);
      if(group != NULL) {
        log("OnGroupCallScene: group-id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "' scene: " + intToString(_sceneID));
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          Set s = zone.getDevices().getByGroup(_groupID);
          SetLastCalledSceneAction act(_sceneID & 0x00ff);
          s.perform(act);

          vector<Zone*> zonesToUpdate;
          if(_zoneID == 0) {
            zonesToUpdate = m_Zones;
          } else {
            zonesToUpdate.push_back(&zone);
          }
          foreach(Zone* pZone, zonesToUpdate) {
            if(_groupID == 0) {
              foreach(Group* pGroup, pZone->getGroups()) {
                pGroup->setLastCalledScene(_sceneID & 0x00ff);
              }
            } else {
              Group* pGroup = pZone->getGroup(_groupID);
              if(pGroup != NULL) {
                pGroup->setLastCalledScene(_sceneID & 0x00ff);
              }
            }
          }
        }
      } else {
        log("OnGroupCallScene: Could not find group with id '" + intToString(_groupID) + "' in Zone '" + intToString(_zoneID) + "'", lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnGroupCallScene: Could not find zone with id '" + intToString(_zoneID) + "'", lsError);
    }

  } // onGroupCallScene

  void Apartment::onDeviceCallScene(const int _modulatorID, const int _deviceID, const int _sceneID) {
    try {
      if(_sceneID < 0 || _sceneID > MaxSceneNumber) {
        log("onDeviceCallScene: _sceneID is out of bounds. modulator-id '" + intToString(_modulatorID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID), lsError);
        return;
      }
      Modulator& mod = getModulatorByBusID(_modulatorID);
      try {
        log("OnDeviceCallScene: modulator-id '" + intToString(_modulatorID) + "' for device '" + intToString(_deviceID) + "' scene: " + intToString(_sceneID));
        DeviceReference devRef = mod.getDevices().getByBusID(_deviceID);
        if(SceneHelper::rememberScene(_sceneID & 0x00ff)) {
          devRef.getDevice().setLastCalledScene(_sceneID & 0x00ff);
        }
      } catch(ItemNotFoundException& e) {
        log("OnDeviceCallScene: Could not find device with bus-id '" + intToString(_deviceID) + "' on modulator '" + intToString(_modulatorID) + "' scene:" + intToString(_sceneID), lsError);
      }
    } catch(ItemNotFoundException& e) {
      log("OnDeviceCallScene: Could not find modulator with bus-id '" + intToString(_modulatorID) + "'", lsError);
    }
  } // onDeviceCallScene

  void Apartment::onAddDevice(const int _modID, const int _zoneID, const int _devID, const int _functionID) {
    // get full dsid
    log("New Device found");
    log("  Modulator: " + intToString(_modID));
    log("  Zone:      " + intToString(_zoneID));
    log("  BusID:     " + intToString(_devID));
    log("  FID:       " + intToString(_functionID));

    dsid_t dsid = getDSS().getDS485Interface().getDSIDOfDevice(_modID, _devID);
    Device& dev = allocateDevice(dsid);
    DeviceReference devRef(dev, this);

    log("  DSID:      " + dsid.toString());

    // remove from old modulator
    try {
      Modulator& oldModulator = getModulatorByBusID(dev.getModulatorID());
      oldModulator.removeDevice(devRef);
    } catch(std::runtime_error&) {
    }

    // remove from old zone
    if(dev.getZoneID() != 0) {
      try {
        Zone& oldZone = getZone(dev.getZoneID());
        oldZone.removeDevice(devRef);
        // TODO: check if the zone is empty on the modulator and remove it in that case
      } catch(std::runtime_error&) {
      }
    }

    // update device
    dev.setModulatorID(_modID);
    dev.setZoneID(_zoneID);
    dev.setShortAddress(_devID);
    dev.setFunctionID(_functionID);
    dev.setIsPresent(true);

    // add to new modulator
    Modulator& modulator = getModulatorByBusID(_modID);
    modulator.addDevice(devRef);

    // add to new zone
    Zone& newZone = allocateZone(_zoneID);
    newZone.addToModulator(modulator);
    newZone.addDevice(devRef);

    // get groups of device
    dev.resetGroups();
    vector<int> groups = getDSS().getDS485Interface().getGroupsOfDevice(_modID, _devID);
    foreach(int iGroup, groups) {
      log("  Adding to Group: " + intToString(iGroup));
      dev.addToGroup(iGroup);
    }
    
    {
      boost::shared_ptr<Event> readyEvent(new Event("new_device"));
      readyEvent->setProperty("device", dsid.toString());
      readyEvent->setProperty("zone", intToString(_zoneID));
      raiseEvent(readyEvent);
    }
  } // onAddDevice

  void Apartment::onDSLinkInterrupt(const int _modID, const int _devID, const int _priority) {
    // get full dsid
    log("dSLinkInterrupt:");
    log("  Modulator: " + intToString(_modID));
    log("  DevID:     " + intToString(_devID));
    log("  Priority:  " + intToString(_priority));

    try {
      Modulator& modulator = getModulatorByBusID(_modID);
      try {
        Device& device = getDeviceByShortAddress(modulator, _devID);
        PropertyNodePtr deviceNode = device.getPropertyNode();
        if(deviceNode == NULL) {
          return;
        }
        PropertyNodePtr modeNode = deviceNode->getProperty("interrupt/mode");
        if(modeNode == NULL) {
          return;
        }
        std::string mode = modeNode->getStringValue();
        if(mode == "ignore") {
          log("ignoring interrupt");
        } else if(mode == "raise_event") {
          log("raising interrupt as event");
          std::string eventName = "dslink_interrupt";
          PropertyNodePtr eventNameNode = deviceNode->getProperty("interrupt/event/name");
          if(eventNameNode == NULL) {
            log("no node called 'interrupt/event' found, assuming name is 'dslink_interrupt'");
          } else {
            eventName = eventNameNode->getAsString();
          }

          // create event to be raised
          DeviceReference devRef(device, this);
          boost::shared_ptr<Event> evt(new Event(eventName, &devRef));
          evt->setProperty("device", device.getDSID().toString());
          std::string priorityString = "unknown";
          if(_priority == 0) {
            priorityString = "normal";
          } else if(_priority == 1) {
            priorityString = "high";
          }
          evt->setProperty("priority", priorityString);
          raiseEvent(evt);
        } else {
          log("unknown interrupt mode '" + mode + "'", lsError);
        }
      } catch (ItemNotFoundException& ex) {
        log("Apartment::onDSLinkInterrupt: Unknown device with ID " + intToString(_devID), lsFatal);
        return;
      }
    } catch(ItemNotFoundException& ex) {
      log("Apartment::onDSLinkInterrupt: Unknown Modulator with ID " + intToString(_modID), lsFatal);
      return;
    }
  } // onDSLinkInterrupt

  void Apartment::sendCommand(DS485Command _command, const Device& _device, int _parameter) {
    if(m_pDS485Interface != NULL) {
      m_pDS485Interface->sendCommand(_command, _device, _parameter);
    } else {
      throw std::runtime_error("Apartment::sendCommand: DS485Interface is NULL");
    }    
  } // sendCommand
  
  void Apartment::dispatchRequest(boost::shared_ptr<BusRequest> _pRequest) {
    if(m_pBusRequestDispatcher != NULL) {
      m_pBusRequestDispatcher->dispatchRequest(_pRequest);
    } else {
      throw std::runtime_error("Apartment::dispatchRequest: m_pBusRequestDispatcher is NULL");
    }
  } // dispatchRequest

  
  //================================================== Modulator

  Modulator::Modulator(const dsid_t _dsid)
  : m_DSID(_dsid),
    m_BusID(0xFF),
    m_PowerConsumption(0),
    m_EnergyMeterValue(0),
    m_IsValid(false)
  {
  } // ctor

  Set Modulator::getDevices() const {
    return m_ConnectedDevices;
  } // getDevices

  void Modulator::addDevice(const DeviceReference& _device) {
    if(!contains(m_ConnectedDevices, _device)) {
  	  m_ConnectedDevices.push_back(_device);
    } else {
      Logger::getInstance()->log("Modulator::addDevice: DUPLICATE DEVICE Detected modulator: " + intToString(m_BusID) + " device: " + _device.getDSID().toString(), lsFatal);
    }
  } // addDevice

  void Modulator::removeDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_ConnectedDevices.begin(), m_ConnectedDevices.end(), _device);
    if(pos != m_ConnectedDevices.end()) {
      m_ConnectedDevices.erase(pos);
    }
  } // removeDevice

  dsid_t Modulator::getDSID() const {
    return m_DSID;
  } // getDSID

  int Modulator::getBusID() const {
    return m_BusID;
  } // getBusID

  void Modulator::setBusID(const int _busID) {
    m_BusID = _busID;
  } // setBusID

  unsigned long Modulator::getPowerConsumption() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_PowerConsumptionAge)) {
      m_PowerConsumption =  DSS::getInstance()->getDS485Interface().getPowerConsumption(m_BusID);
      m_PowerConsumptionAge = now;
    }
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long Modulator::getEnergyMeterValue() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_EnergyMeterValueAge)) {
      m_EnergyMeterValue = DSS::getInstance()->getDS485Interface().getEnergyMeterValue(m_BusID);
      m_EnergyMeterValueAge = now;
    }
    return m_EnergyMeterValue;
  } // getEnergyMeterValue


  /** set the consumption in mW */
  void Modulator::setPowerConsumption(unsigned long _value)
  {
    DateTime now;
    m_PowerConsumptionAge = now;
    m_PowerConsumption = _value;
  }

  /** set the meter value in Wh */
  void Modulator::setEnergyMeterValue(unsigned long _value)
  {
    DateTime now;
    m_EnergyMeterValueAge = now;
    m_EnergyMeterValue = _value;
  }

  unsigned long Modulator::getCachedPowerConsumption() {
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long Modulator::getCachedEnergyMeterValue() {
    return m_EnergyMeterValue;
  } // getEnergyMeterValue

  
  //================================================== Zone

  Zone::~Zone() {
    scrubVector(m_Groups);
    // we don't own our modulators
    m_Modulators.clear();
  } // dtor

  Set Zone::getDevices() const {
    return Set(m_Devices);
  } // getDevices

  void Zone::addDevice(DeviceReference& _device) {
    const Device& dev = _device.getDevice();
  	int oldZoneID = dev.getZoneID();
  	if((oldZoneID != -1) && (oldZoneID != 0)) {
  		try {
  		  Zone& oldZone = dev.getApartment().getZone(oldZoneID);
  		  oldZone.removeDevice(_device);
  		} catch(std::runtime_error&) {
  		}
  	}
    if(!contains(m_Devices, _device)) {
      m_Devices.push_back(_device);
    } else {
      // don't warn about multiple additions to zone 0
      if(m_ZoneID != 0) {
        Logger::getInstance()->log("Zone::addDevice: DUPLICATE DEVICE Detected Zone: " + intToString(m_ZoneID) + " device: " + _device.getDSID().toString(), lsWarning);
      }
    }
    _device.getDevice().setZoneID(m_ZoneID);
  } // addDevice

  void Zone::addGroup(Group* _group) {
    if(_group->getZoneID() != m_ZoneID) {
      throw std::runtime_error("Zone::addGroup: ZoneID of _group does not match own");
    }
    m_Groups.push_back(_group);
  } // addGroup

  void Zone::removeGroup(UserGroup* _group) {
    std::vector<Group*>::iterator it = find(m_Groups.begin(), m_Groups.end(), _group);
    if(it != m_Groups.end()) {
      m_Groups.erase(it);
    }
  } // removeGroup

  void Zone::removeDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_Devices.begin(), m_Devices.end(), _device);
    if(pos != m_Devices.end()) {
      m_Devices.erase(pos);
    }
  } // removeDevice

  Group* Zone::getGroup(const std::string& _name) const {
    for(vector<Group*>::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->getName() == _name) {
          return *ipGroup;
        }
    }
    return NULL;
  } // getGroup

  Group* Zone::getGroup(const int _id) const {
    for(vector<Group*>::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->getID() == _id) {
          return *ipGroup;
        }
    }
    return NULL;
  } // getGroup

  int Zone::getID() const {
    return m_ZoneID;
  } // getID

  void Zone::setZoneID(const int _value) {
    m_ZoneID = _value;
  } // setZoneID

  void Zone::addToModulator(const Modulator& _modulator) {
    // make sure the zone is connected to the modulator
    if(find(m_Modulators.begin(), m_Modulators.end(), &_modulator) == m_Modulators.end()) {
      m_Modulators.push_back(&_modulator);
    }
  } // addToModulator

  void Zone::removeFromModulator(const Modulator& _modulator) {
    m_Modulators.erase(find(m_Modulators.begin(), m_Modulators.end(), &_modulator));
  } // removeFromModulator

  vector<int> Zone::getModulators() const {
  	vector<int> result;
  	for(vector<const Modulator*>::const_iterator iModulator = m_Modulators.begin(), e = m_Modulators.end();
  	    iModulator != e; ++iModulator) {
  		result.push_back((*iModulator)->getBusID());
  	}
  	return result;
  } // getModulators

  bool Zone::registeredOnModulator(const Modulator& _modulator) const {
    return find(m_Modulators.begin(), m_Modulators.end(), &_modulator) != m_Modulators.end();
  } // registeredOnModulator

  void Zone::turnOn() {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdTurnOn, *this, GroupIDBroadcast);
  } // turnOn

  void Zone::turnOff() {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdTurnOff, *this, GroupIDBroadcast);
  } // turnOff

  void Zone::increaseValue(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdIncreaseValue, *this, GroupIDBroadcast, _parameterNr);
  } // increaseValue

  void Zone::decreaseValue(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdDecreaseValue, *this, GroupIDBroadcast, _parameterNr);
  } // decreaseValue

  void Zone::startDim(const bool _directionUp, const int _parameterNr) {
    if(_directionUp) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdStartDimUp, *this, GroupIDBroadcast, _parameterNr);
    } else {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdStartDimDown, *this, GroupIDBroadcast, _parameterNr);
    }
  } // startDim

  void Zone::endDim(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdStopDim, *this, GroupIDBroadcast, _parameterNr);
  } // endDim

  void Zone::setValue(const double _value, const int _parameterNr) {
    if(_parameterNr == -1) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdSetValue, *this, GroupIDBroadcast, static_cast<int>(_value));
    } else {
      throw std::runtime_error("Can't set arbitrary parameter on a zone");
    }
  } // setValue

  void Zone::callScene(const int _sceneNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdCallScene, *this, GroupIDBroadcast, _sceneNr);
  } // callScene

  void Zone::saveScene(const int _sceneNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdSaveScene, *this, GroupIDBroadcast, _sceneNr);
  } // saveScene

  void Zone::undoScene(const int _sceneNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdUndoScene, *this, GroupIDBroadcast, _sceneNr);
  } // undoScene

  unsigned long Zone::getPowerConsumption() {
    return getDevices().getPowerConsumption();
  } // getPowerConsumption

  void Zone::nextScene() {
    Group* broadcastGroup = getGroup(0);
    if(broadcastGroup != NULL) {
      broadcastGroup->nextScene();
    }
  } // nextScene

  void Zone::previousScene() {
    Group* broadcastGroup = getGroup(0);
    if(broadcastGroup != NULL) {
      broadcastGroup->previousScene();
    }
  } // previousScene


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

  void Group::increaseValue(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdIncreaseValue, m_pApartment->getZone(m_ZoneID), m_GroupID, _parameterNr);
  } // increaseValue

  void Group::decreaseValue(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdDecreaseValue, m_pApartment->getZone(m_ZoneID), m_GroupID, _parameterNr);
  } // decreaseValue

  void Group::startDim(bool _directionUp, const int _parameterNr)  {
    if(_directionUp) {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdStartDimUp, m_pApartment->getZone(m_ZoneID), m_GroupID, _parameterNr);
    } else {
      DSS::getInstance()->getDS485Interface().sendCommand(cmdStartDimDown, m_pApartment->getZone(m_ZoneID), m_GroupID, _parameterNr);
    }
  } // startDim

  void Group::endDim(const int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdStopDim, m_pApartment->getZone(m_ZoneID), m_GroupID, _parameterNr);
  } // endDim

  void Group::setValue(const double _value, int _parameterNr) {
    DSS::getInstance()->getDS485Interface().sendCommand(cmdSetValue, m_pApartment->getZone(m_ZoneID), m_GroupID, static_cast<int>(_value));
  } // setValue

  Group& Group::operator=(const Group& _other) {
    m_Devices = _other.m_Devices;
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


  //================================================== DeviceReference

  DeviceReference::DeviceReference(const DeviceReference& _copy)
  : m_DSID(_copy.m_DSID),
    m_Apartment(_copy.m_Apartment)
  {
  } // ctor(copy)

  DeviceReference::DeviceReference(const dsid_t _dsid, const Apartment* _apartment)
  : m_DSID(_dsid),
    m_Apartment(_apartment)
  {
  } // ctor(dsid)

  DeviceReference::DeviceReference(const Device& _device, const Apartment* _apartment)
  : m_DSID(_device.getDSID()),
    m_Apartment(_apartment)
  {
  } // ctor(device)

  Device& DeviceReference::getDevice() {
    return m_Apartment->getDeviceByDSID(m_DSID);
  } // getDevice

  const Device& DeviceReference::getDevice() const {
    return m_Apartment->getDeviceByDSID(m_DSID);
  } // getDevice

  dsid_t DeviceReference::getDSID() const {
    return m_DSID;
  } // getID

  std::string DeviceReference::getName() const {
    return getDevice().getName();
  } //getName

  void DeviceReference::turnOn() {
    getDevice().turnOn();
  } // turnOn

  void DeviceReference::turnOff() {
    getDevice().turnOff();
  } // turnOff

  void DeviceReference::increaseValue(const int _parameterNr) {
    getDevice().increaseValue(_parameterNr);
  } // increaseValue

  void DeviceReference::decreaseValue(const int _parameterNr) {
    getDevice().decreaseValue(_parameterNr);
  } // decreaseValue

  void DeviceReference::enable() {
    getDevice().enable();
  } // enable

  void DeviceReference::disable() {
    getDevice().disable();
  } // disable

  void DeviceReference::startDim(const bool _directionUp, const int _parameterNr) {
    getDevice().startDim(_directionUp, _parameterNr);
  } // startDim

  void DeviceReference::endDim(const int _parameterNr) {
    getDevice().endDim(_parameterNr);
  } // endDim

  void DeviceReference::setValue(const double _value, const int _parameterNr) {
    getDevice().setValue(_value, _parameterNr);
  } // setValue

  bool DeviceReference::isOn() const {
    return getDevice().isOn();
  }

  bool DeviceReference::hasSwitch() const {
    return getDevice().hasSwitch();
  }

  void DeviceReference::callScene(const int _sceneNr) {
    getDevice().callScene(_sceneNr);
  } // callScene

  void DeviceReference::saveScene(const int _sceneNr) {
    getDevice().saveScene(_sceneNr);
  } // saveScene

  void DeviceReference::undoScene(const int _sceneNr) {
    getDevice().undoScene(_sceneNr);
  } // undoScene

  unsigned long DeviceReference::getPowerConsumption() {
    return getDevice().getPowerConsumption();
  }

  void DeviceReference::nextScene() {
    getDevice().nextScene();
  }

  void DeviceReference::previousScene() {
    getDevice().previousScene();
  }

  void DeviceContainer::setName(const std::string& _name) {
    if(m_Name != _name) {
      m_Name = _name;
      if(DSS::hasInstance()) {
        DSS::getInstance()->getApartment().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      }
    }
  }

  //================================================== Utils

  unsigned int SceneHelper::getNextScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case ScenePanic:
    case SceneStandBy:
    case SceneDeepOff:
    case SceneOff:
    case Scene1:
      return Scene2;
    case Scene2:
      return Scene3;
    case Scene3:
      return Scene4;
    case Scene4:
      return Scene2;
    default:
      return Scene1;
    }
  } // getNextScene

  unsigned int SceneHelper::getPreviousScene(const unsigned int _currentScene) {
    switch(_currentScene) {
    case ScenePanic:
    case SceneStandBy:
    case SceneDeepOff:
    case SceneOff:
    case Scene1:
      return Scene4;
    case Scene2:
      return Scene4;
    case Scene3:
      return Scene2;
    case Scene4:
      return Scene3;
    default:
      return Scene1;
    }
  } // getPreviousScene

  bool SceneHelper::m_Initialized = false;
  std::bitset<64> SceneHelper::m_ZonesToIgnore;

  void SceneHelper::initialize() {
    m_Initialized = true;
    m_ZonesToIgnore.reset();
    m_ZonesToIgnore.set(SceneInc);
    m_ZonesToIgnore.set(SceneDec);
    m_ZonesToIgnore.set(SceneStop);
    m_ZonesToIgnore.set(SceneBell);
    m_ZonesToIgnore.set(SceneEnergyOverload);
    m_ZonesToIgnore.set(SceneEnergyHigh);
    m_ZonesToIgnore.set(SceneEnergyMiddle);
    m_ZonesToIgnore.set(SceneEnergyLow);
    m_ZonesToIgnore.set(SceneEnergyClassA);
    m_ZonesToIgnore.set(SceneEnergyClassB);
    m_ZonesToIgnore.set(SceneEnergyClassC);
    m_ZonesToIgnore.set(SceneEnergyClassD);
    m_ZonesToIgnore.set(SceneEnergyClassE);
    m_ZonesToIgnore.set(SceneLocalOff);
    m_ZonesToIgnore.set(SceneLocalOn);
  }

  bool SceneHelper::rememberScene(const unsigned int _scene) {
    if(!m_Initialized) {
      initialize();
      assert(m_Initialized);
    }
    return !m_ZonesToIgnore.test(_scene);
  } // rememberScene

}

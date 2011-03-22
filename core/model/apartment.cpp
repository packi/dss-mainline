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

#include "apartment.h"

#include "core/businterface.h"
#include "core/model/modelconst.h"
#include "core/logger.h"
#include "core/propertysystem.h"
#include "core/event.h"
#include "core/foreach.h"

#include "core/model/busscanner.h"
#include "core/model/scenehelper.h"
#include "core/model/modelevent.h"

#include "core/model/set.h"
#include "core/model/device.h"
#include "core/model/set.h"
#include "core/model/zone.h"
#include "core/model/group.h"
#include "core/model/modulator.h"

namespace dss {


  //================================================== Apartment

  Apartment::Apartment(DSS* _pDSS)
  : m_pBusInterface(NULL),
    m_pModelMaintenance(NULL),
    m_pPropertySystem(NULL)
  {
    // create default (broadcast) zone
    boost::shared_ptr<Zone> zoneZero = allocateZone(0);
    zoneZero->setIsPresent(true);
  } // ctor

  Apartment::~Apartment() {
  } // dtor

  void Apartment::addDefaultGroupsToZone(boost::shared_ptr<Zone> _zone) {
    boost::shared_ptr<Group> grp(new Group(GroupIDBroadcast, _zone, *this));
    grp->setName("broadcast");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDYellow, _zone, *this));
    grp->setName("yellow");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDGray, _zone, *this));
    grp->setName("gray");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDBlue, _zone, *this));
    grp->setName("blue");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDCyan, _zone, *this));
    grp->setName("cyan");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDViolet, _zone, *this));
    grp->setName("magenta");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDRed, _zone, *this));
    grp->setName("red");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDGreen, _zone, *this));
    grp->setName("green");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDBlack, _zone, *this));
    grp->setName("black");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDWhite, _zone, *this));
    grp->setName("white");
    _zone->addGroup(grp);
    grp.reset(new Group(GroupIDDisplay, _zone, *this));
    grp->setName("display");
    _zone->addGroup(grp);
  } // addDefaultGroupsToZone

  boost::shared_ptr<Device> Apartment::getDeviceByDSID(const dss_dsid_t _dsid) const {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      if(dev->getDSID() == _dsid) {
        return dev;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDeviceByShortAddress const

  boost::shared_ptr<Device> Apartment::getDeviceByDSID(const dss_dsid_t _dsid) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      if(dev->getDSID() == _dsid) {
        return dev;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDeviceByShortAddress

  boost::shared_ptr<Device> Apartment::getDeviceByName(const std::string& _name) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      if(dev->getName() == _name) {
        return dev;
      }
    }
    throw ItemNotFoundException(_name);
  } // getDeviceByName

  Set Apartment::getDevices() const {
    DeviceVector devs;
    AssertLocked lock(this);
    foreach(boost::shared_ptr<Device> dev, m_Devices) {
      devs.push_back(DeviceReference(dev, this));
    }

    return Set(devs);
  } // getDevices

  boost::shared_ptr<Zone> Apartment::getZone(const std::string& _zoneName) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<Zone> zone, m_Zones) {
      if(zone->getName() == _zoneName) {
        return zone;
      }
    }
    throw ItemNotFoundException(_zoneName);
  } // getZone(name)

  boost::shared_ptr<Zone> Apartment::getZone(const int _id) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<Zone> zone, m_Zones) {
      if(zone->getID() == _id) {
        return zone;
      }
    }
    throw ItemNotFoundException(intToString(_id));
  } // getZone(id)

  std::vector<boost::shared_ptr<Zone> > Apartment::getZones() {
    AssertLocked lock(this);
    std::vector<boost::shared_ptr<Zone> > result = m_Zones;
    return result;
  } // getZones

  boost::shared_ptr<DSMeter> Apartment::getDSMeter(const std::string& _modName) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if(dsMeter->getName() == _modName) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(_modName);
  } // getDSMeter(name)

  boost::shared_ptr<DSMeter> Apartment::getDSMeterByDSID(const dss_dsid_t _dsid) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if(dsMeter->getDSID() == _dsid) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDSMeterByDSID

  std::vector<boost::shared_ptr<DSMeter> > Apartment::getDSMeters() {
    AssertLocked lock(this);
    std::vector<boost::shared_ptr<DSMeter> > result = m_DSMeters;
    return result;
  } // getDSMeters

  // Group queries
  boost::shared_ptr<Group> Apartment::getGroup(const std::string& _name) {
    boost::shared_ptr<Group> pResult = getZone(0)->getGroup(_name);
    if(pResult != NULL) {
      return pResult;
    }
    throw ItemNotFoundException(_name);
  } // getGroup(name)

  boost::shared_ptr<Group> Apartment::getGroup(const int _id) {
    boost::shared_ptr<Group> pResult = getZone(0)->getGroup(_id);
    if(pResult != NULL) {
      return pResult;
    }
    throw ItemNotFoundException(intToString(_id));
  } // getGroup(id)

  boost::shared_ptr<Device> Apartment::allocateDevice(const dss_dsid_t _dsid) {
    AssertLocked lock(this);
    boost::shared_ptr<Device> pResult;
    // search for existing device
    foreach(boost::shared_ptr<Device> device, m_Devices) {
      if(device->getDSID() == _dsid) {
        pResult = device;
        break;
      }
    }

    if(pResult == NULL) {
      pResult.reset(new Device(_dsid, this));
      pResult->setFirstSeen(DateTime());
      m_Devices.push_back(pResult);
    }
    DeviceReference devRef(pResult, this);
    getZone(0)->addDevice(devRef);
    pResult->publishToPropertyTree();
    return pResult;
  } // allocateDevice

  boost::shared_ptr<DSMeter> Apartment::allocateDSMeter(const dss_dsid_t _dsid) {
    AssertLocked lock(this);
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if((dsMeter)->getDSID() == _dsid) {
        return dsMeter;
      }
    }

    boost::shared_ptr<DSMeter> pResult(new DSMeter(_dsid, this));
    m_DSMeters.push_back(pResult);
    return pResult;
  } // allocateDSMeter

  boost::shared_ptr<Zone> Apartment::allocateZone(int _zoneID) {
    AssertLocked lock(this);
    boost::shared_ptr<Zone> result;
    foreach(boost::shared_ptr<Zone> zone, m_Zones) {
      if(zone->getID() == _zoneID) {
        result = zone;
        break;
      }
    }

    if(result == NULL) {
      result.reset(new Zone(_zoneID, this));
      result->publishToPropertyTree();
      addDefaultGroupsToZone(result);
      m_Zones.push_back(result);
    } else {
      result->publishToPropertyTree();
    }
    return result;
  } // allocateZone

  void Apartment::removeZone(int _zoneID) {
    AssertLocked lock(this);
    for(std::vector<boost::shared_ptr<Zone> >::iterator ipZone = m_Zones.begin(), e = m_Zones.end();
        ipZone != e; ++ipZone) {
      boost::shared_ptr<Zone> pZone = *ipZone;
      if(pZone->getID() == _zoneID) {
        m_Zones.erase(ipZone);
        return;
      }
    }
  } // removeZone

  void Apartment::removeDevice(dss_dsid_t _device) {
    AssertLocked lock(this);
    for(std::vector<boost::shared_ptr<Device> >::iterator ipDevice = m_Devices.begin(), e = m_Devices.end();
        ipDevice != e; ++ipDevice) {
      boost::shared_ptr<Device> pDevice = *ipDevice;
      if(pDevice->getDSID() == _device) {
        int zoneID = pDevice->getZoneID();
        DeviceReference devRef = DeviceReference(pDevice, this);
        if(zoneID != 0) {
          getZone(zoneID)->removeDevice(devRef);
        }
        getZone(0)->removeDevice(devRef);
        m_Devices.erase(ipDevice);
        return;
      }
    }
  } // removeDevice

  void Apartment::removeDSMeter(dss_dsid_t _dsMeter) {
    AssertLocked lock(this);
    for(std::vector<boost::shared_ptr<DSMeter> >::iterator ipDSMeter = m_DSMeters.begin(), e = m_DSMeters.end();
        ipDSMeter != e; ++ipDSMeter) {
      boost::shared_ptr<DSMeter> pDSMeter = *ipDSMeter;
      if(pDSMeter->getDSID() == _dsMeter) {
        m_DSMeters.erase(ipDSMeter);
        return;
      }
    }
  } // removeDSMeter

  ActionRequestInterface* Apartment::getActionRequestInterface() {
    if(m_pBusInterface != NULL) {
      return m_pBusInterface->getActionRequestInterface();
    }
    return NULL;
  } // getActionRequestInterface

  DeviceBusInterface* Apartment::getDeviceBusInterface() {
    if(m_pBusInterface != NULL) {
      return m_pBusInterface->getDeviceBusInterface();
    }
    return NULL;
  } // getDeviceBusInterface

  void Apartment::setPropertySystem(PropertySystem* _value) {
    m_pPropertySystem = _value;
    if(m_pPropertySystem != NULL) {
      m_pPropertyNode = m_pPropertySystem->createProperty("/apartment");
      foreach(boost::shared_ptr<Zone> pZone, m_Zones) {
        pZone->publishToPropertyTree();
      }
    }
  } // setPropertySystem

} // namespace dss

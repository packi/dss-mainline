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

#include "core/DS485Interface.h"
#include "core/ds485const.h"
#include "core/model/modelconst.h"
#include "core/logger.h"
#include "core/propertysystem.h"
#include "core/event.h"
#include "core/foreach.h"

#include "core/busrequestdispatcher.h"
#include "core/model/busrequest.h"

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
  : m_pDS485Interface(NULL),
    m_pBusRequestDispatcher(NULL),
    m_pModelMaintenance(NULL),
    m_pPropertySystem(NULL)
  {
    // create default (broadcast) zone
    Zone& zoneZero = allocateZone(0);
    zoneZero.setIsPresent(true);
  } // ctor

  Apartment::~Apartment() {
    scrubVector(m_Devices);
    scrubVector(m_Zones);
  } // dtor

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

  Device& Apartment::getDeviceByShortAddress(boost::shared_ptr<const DSMeter> _dsMeter, const devid_t _deviceID) const {
    foreach(Device* dev, m_Devices) {
      if((dev->getShortAddress() == _deviceID) &&
          (_dsMeter->getBusID() == dev->getDSMeterID())) {
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

  std::vector<Zone*>& Apartment::getZones() {
    return m_Zones;
  } // getZones

  boost::shared_ptr<DSMeter> Apartment::getDSMeter(const std::string& _modName) {
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if(dsMeter->getName() == _modName) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(_modName);
  } // getDSMeter(name)

  boost::shared_ptr<DSMeter> Apartment::getDSMeterByBusID(const int _busId) {
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if(dsMeter->getBusID() == _busId) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(intToString(_busId));
  } // getDSMeterByBusID

  boost::shared_ptr<DSMeter> Apartment::getDSMeterByDSID(const dsid_t _dsid) {
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if(dsMeter->getDSID() == _dsid) {
        return dsMeter;
      }
    }
    throw ItemNotFoundException(_dsid.toString());
  } // getDSMeterByDSID

  std::vector<boost::shared_ptr<DSMeter> >& Apartment::getDSMeters() {
    return m_DSMeters;
  } // getDSMeters

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
    Device* pResult = NULL;
    // search for existing device
    foreach(Device* device, m_Devices) {
      if(device->getDSID() == _dsid) {
        pResult = device;
        break;
      }
    }

    if(pResult == NULL) {
      pResult = new Device(_dsid, this);
      pResult->setFirstSeen(DateTime());
      m_Devices.push_back(pResult);
    }
    DeviceReference devRef(*pResult, this);
    getZone(0).addDevice(devRef);
    pResult->publishToPropertyTree();
    return *pResult;
  } // allocateDevice

  boost::shared_ptr<DSMeter> Apartment::allocateDSMeter(const dsid_t _dsid) {
    foreach(boost::shared_ptr<DSMeter> dsMeter, m_DSMeters) {
      if((dsMeter)->getDSID() == _dsid) {
        return dsMeter;
      }
    }

    boost::shared_ptr<DSMeter> pResult(new DSMeter(_dsid, this));
    m_DSMeters.push_back(pResult);
    return pResult;
  } // allocateDSMeter

  Zone& Apartment::allocateZone(int _zoneID) {
    Zone* result = NULL;
    foreach(Zone* zone, m_Zones) {
      if(zone->getID() == _zoneID) {
        result = zone;
        break;
      }
    }

    if(result == NULL) {
      result = new Zone(_zoneID, this);
      result->publishToPropertyTree();
      addDefaultGroupsToZone(*result);
      m_Zones.push_back(result);
    } else {
      result->publishToPropertyTree();
    }
    return *result;
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
        int zoneID = pDevice->getZoneID();
        DeviceReference devRef = DeviceReference(*pDevice, this);
        if(zoneID != 0) {
          getZone(zoneID).removeDevice(devRef);
        }
        getZone(0).removeDevice(devRef);
        m_Devices.erase(ipDevice);
        delete pDevice;
        return;
      }
    }
  } // removeDevice

  void Apartment::removeDSMeter(dsid_t _dsMeter) {
    for(std::vector<boost::shared_ptr<DSMeter> >::iterator ipDSMeter = m_DSMeters.begin(), e = m_DSMeters.end();
        ipDSMeter != e; ++ipDSMeter) {
      boost::shared_ptr<DSMeter> pDSMeter = *ipDSMeter;
      if(pDSMeter->getDSID() == _dsMeter) {
        m_DSMeters.erase(ipDSMeter);
        return;
      }
    }
  } // removeDSMeter

  void Apartment::dispatchRequest(boost::shared_ptr<BusRequest> _pRequest) {
    if(m_pBusRequestDispatcher != NULL) {
      m_pBusRequestDispatcher->dispatchRequest(_pRequest);
    } else {
      throw std::runtime_error("Apartment::dispatchRequest: m_pBusRequestDispatcher is NULL");
    }
  } // dispatchRequest

  DeviceBusInterface* Apartment::getDeviceBusInterface() {
    if(m_pDS485Interface != NULL) {
      return m_pDS485Interface->getDeviceBusInterface();
    }
    return NULL;
  } // getDeviceBusInterface

  void Apartment::setPropertySystem(PropertySystem* _value) {
    m_pPropertySystem = _value;
    if(m_pPropertySystem != NULL) {
      m_pPropertyNode = m_pPropertySystem->createProperty("/apartment");
      foreach(Zone* pZone, m_Zones) {
        pZone->publishToPropertyTree();
      }
    }
  } // setPropertySystem

} // namespace dss

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

#include "modulator.h"

#include "core/dss.h"
#include "core/logger.h"
#include "core/businterface.h"
#include "core/propertysystem.h"
#include "core/dsidhelper.h"
#include "set.h"
#include "apartment.h"

namespace dss {


  //================================================== DSMeter

  DSMeter::DSMeter(const dss_dsid_t _dsid, Apartment* _pApartment)
  : m_DSID(_dsid),
    m_PowerConsumption(0),
    m_EnergyMeterValue(0),
    m_IsValid(false),
    m_pApartment(_pApartment)
  {
    publishToPropertyTree();
  } // ctor

  void DSMeter::publishToPropertyTree() {
    assert(m_pPropertyNode == NULL);
    if((m_pApartment != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
      m_pPropertyNode =
        m_pApartment->getPropertyNode()->createProperty("dSMeters/" + m_DSID.toString());
      m_pPropertyNode->createProperty("powerConsumption")
        ->linkToProxy(PropertyProxyReference<int>(m_PowerConsumption, false));
      m_pPropertyNode->createProperty("powerConsumptionAge")
      ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_PowerConsumptionTimeStamp, &DateTime::toString));
      m_pPropertyNode->createProperty("energyMeterValue")
        ->linkToProxy(PropertyProxyReference<int>(m_EnergyMeterValue, false));
      m_pPropertyNode->createProperty("isValid")
        ->linkToProxy(PropertyProxyReference<bool>(m_IsValid, false));
      m_pPropertyNode->createProperty("energyLevelRed")
        ->linkToProxy(PropertyProxyReference<int>(m_EnergyLevelRed, false));
      m_pPropertyNode->createProperty("energyLevelOrange")
        ->linkToProxy(PropertyProxyReference<int>(m_EnergyLevelOrange, false));
      m_pPropertyNode->createProperty("hardwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_HardwareVersion, false));
      m_pPropertyNode->createProperty("softwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_SoftwareVersion, false));
      m_pPropertyNode->createProperty("apiVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_ApiVersion, false));
      m_pPropertyNode->createProperty("hardwareName")
        ->linkToProxy(PropertyProxyReference<std::string>(m_HardwareName, false));
      m_pPropertyNode->createProperty("name")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, std::string>(*this, &DSMeter::getName, &DSMeter::setName));
    }
  } // publishToPropertyTree

  Set DSMeter::getDevices() const {
    return m_ConnectedDevices;
  } // getDevices

  void DSMeter::addDevice(const DeviceReference& _device) {
    if(!contains(m_ConnectedDevices, _device)) {
      m_ConnectedDevices.push_back(_device);
    } else {
      Logger::getInstance()->log("DSMeter::addDevice: DUPLICATE DEVICE Detected dsMeter: " + 
                                 m_DSID.toString() + " device: " + _device.getDSID().toString(), lsFatal);
    }
  } // addDevice

  void DSMeter::removeDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_ConnectedDevices.begin(), m_ConnectedDevices.end(), _device);
    if(pos != m_ConnectedDevices.end()) {
      m_ConnectedDevices.erase(pos);
    }
  } // removeDevice

  dss_dsid_t DSMeter::getDSID() const {
    return m_DSID;
  } // getDSID

  unsigned long DSMeter::getPowerConsumption() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_PowerConsumptionTimeStamp)) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(m_DSID, dsid);
      m_PowerConsumption =  DSS::getInstance()->getBusInterface().getMeteringBusInterface()->getPowerConsumption(dsid);
      m_PowerConsumptionTimeStamp = now;
    }
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long DSMeter::getEnergyMeterValue() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_EnergyMeterValueTimeStamp)) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(m_DSID, dsid);
      m_EnergyMeterValue = DSS::getInstance()->getBusInterface().getMeteringBusInterface()->getEnergyMeterValue(dsid);
      m_EnergyMeterValueTimeStamp = now;
    }
    return m_EnergyMeterValue;
  } // getEnergyMeterValue

  /** set the consumption in mW */
  void DSMeter::setPowerConsumption(unsigned long _value) {
    DateTime now;
    m_PowerConsumptionTimeStamp = now;
    m_PowerConsumption = _value;
  }

  /** set the meter value in Wh */
  void DSMeter::setEnergyMeterValue(unsigned long _value)  {
    DateTime now;
    m_EnergyMeterValueTimeStamp = now;
    m_EnergyMeterValue = _value;
  }

  unsigned long DSMeter::getCachedPowerConsumption() {
    return m_PowerConsumption;
  } // getPowerConsumption

  const DateTime& DSMeter::getCachedPowerConsumptionTimeStamp() const {
    return m_PowerConsumptionTimeStamp;
  }

  unsigned long DSMeter::getCachedEnergyMeterValue() {
    return m_EnergyMeterValue;
  } // getEnergyMeterValue

  const DateTime& DSMeter::getCachedEnergyMeterTimeStamp() const {
    return m_EnergyMeterValueTimeStamp;
  }
} // namespace dss

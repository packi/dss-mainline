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

#include "src/dss.h"
#include "src/logger.h"
#include "src/businterface.h"
#include "src/propertysystem.h"
#include "set.h"
#include "apartment.h"

namespace dss {


  //================================================== DSMeter

  DSMeter::DSMeter(const dss_dsid_t _dsid, Apartment* _pApartment)
  : m_DSID(_dsid),
    m_PowerConsumption(0),
    m_EnergyMeterValue(0),
    m_EnergyMeterValueWh(0),
    m_LastReportedEnergyMeterValue(0),
    m_ReceivedMeterValue(false),
    m_HardwareVersion(0),
    m_armSoftwareVersion(0),
    m_dspSoftwareVersion(0),
    m_ApiVersion(0),
    m_IsValid(false),
    m_IsInitialized(false),
    m_HasPendingEvents(false),
    m_pApartment(_pApartment),
    m_BinaryInputEventCount(_dsid.toString())
  {
    publishToPropertyTree();
  } // ctor

  void DSMeter::publishToPropertyTree() {
    assert(m_pPropertyNode == NULL);
    if((m_pApartment != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
      m_pPropertyNode =
        m_pApartment->getPropertyNode()->createProperty("dSMeters/" + m_DSID.toString());
      m_pPropertyNode->createProperty("dSID")->setStringValue(m_DSID.toString());
      m_pPropertyNode->createProperty("powerConsumption")
        ->linkToProxy(PropertyProxyReference<int>(m_PowerConsumption, false));
      m_pPropertyNode->createProperty("powerConsumptionAge")
        ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_PowerConsumptionTimeStamp, &DateTime::toString));
      m_pPropertyNode->createProperty("energyMeterValue")
        ->linkToProxy(PropertyProxyReference<double>(m_EnergyMeterValueWh));
      m_pPropertyNode->createProperty("energyMeterValueWs")
        ->linkToProxy(PropertyProxyReference<double>(m_EnergyMeterValue));
      m_pPropertyNode->createProperty("energyMeterAge")
        ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_EnergyMeterValueTimeStamp, &DateTime::toString));
      m_pPropertyNode->createProperty("isValid")
        ->linkToProxy(PropertyProxyReference<bool>(m_IsValid, false));
      m_pPropertyNode->createProperty("isInitialized")
        ->linkToProxy(PropertyProxyReference<bool>(m_IsInitialized, false));
      m_pPropertyNode->createProperty("present")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, bool, false>(*this, &DSMeter::isPresent));
      m_pPropertyNode->createProperty("hardwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_HardwareVersion, false));
      m_pPropertyNode->createProperty("armSoftwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_armSoftwareVersion, false));
      m_pPropertyNode->createProperty("dspSoftwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_dspSoftwareVersion, false));
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
    DeviceIterator pos = std::find(m_ConnectedDevices.begin(), m_ConnectedDevices.end(), _device);
    if(pos != m_ConnectedDevices.end()) {
      m_ConnectedDevices.erase(pos);
    } else {
      Logger::getInstance()->log("DSMeter::removeDevice: not found, dSM: " +
          m_DSID.toString() + " device: " + _device.getDSID().toString(), lsInfo);
    }
  } // removeDevice

  dss_dsid_t DSMeter::getDSID() const {
    return m_DSID;
  } // getDSID

  unsigned long DSMeter::getPowerConsumption() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_PowerConsumptionTimeStamp)) {
      unsigned long newvalue = 0;
      if(isPresent()) {
        newvalue = DSS::getInstance()->getBusInterface().getMeteringBusInterface()->getPowerConsumption(m_DSID);
      }
      setPowerConsumption(newvalue);
    }
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long long DSMeter::getEnergyMeterValue() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_EnergyMeterValueTimeStamp)) {
      unsigned long long newValue;
      if(isPresent()) {
        newValue = DSS::getInstance()->getBusInterface().getMeteringBusInterface()->getEnergyMeterValue(m_DSID);
        updateEnergyMeterValue(newValue);
      }
    }
    return m_EnergyMeterValue;
  } // getEnergyMeterValue

  void DSMeter::setPowerConsumption(unsigned long _value) {
    DateTime now;
    m_PowerConsumptionTimeStamp = now;
    m_PowerConsumption = _value;
  }

  void DSMeter::updateEnergyMeterValue(unsigned long _value)  {
    DateTime now;
    if(!m_ReceivedMeterValue && (m_EnergyMeterValue != 0)) {
      // Do nothing, the first value is just to set a baseline
    } else if(_value >= m_LastReportedEnergyMeterValue) {
      m_EnergyMeterValue += _value - m_LastReportedEnergyMeterValue;
      m_EnergyMeterValueWh = m_EnergyMeterValue / 3600;
    }
    m_ReceivedMeterValue = true;
    m_LastReportedEnergyMeterValue = _value;
    m_EnergyMeterValueTimeStamp = now;
  }

  void DSMeter::initializeEnergyMeterValue(unsigned long long _value) {
    m_EnergyMeterValue = _value;
    m_EnergyMeterValueWh = _value / 3600;
  } // initializeEnergyMeterValue

  unsigned long DSMeter::getCachedPowerConsumption() {
    return m_PowerConsumption;
  } // getPowerConsumption

  const DateTime& DSMeter::getCachedPowerConsumptionTimeStamp() const {
    return m_PowerConsumptionTimeStamp;
  }

  double DSMeter::getCachedEnergyMeterValue() {
    return m_EnergyMeterValue;
  } // getEnergyMeterValue

  const DateTime& DSMeter::getCachedEnergyMeterTimeStamp() const {
    return m_EnergyMeterValueTimeStamp;
  }
} // namespace dss

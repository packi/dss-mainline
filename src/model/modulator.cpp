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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "modulator.h"

#include <digitalSTROM/dsuid.h>

#include "src/dss.h"
#include "src/logger.h"
#include "src/businterface.h"
#include "src/propertysystem.h"
#include "set.h"
#include "apartment.h"
#include "src/metering/metering.h"

namespace dss {

  bool busMemberIsDSMeter(BusMemberDevice_t type)
  {
    if ((type == BusMember_dSM11) ||
        (type == BusMember_dSM12) ||
        (type == BusMember_vDC)) {
      return true;
    }
    return false;
  }

  bool busMemberIsdSM(BusMemberDevice_t type)
  {
    if ((type == BusMember_dSM11) ||
        (type == BusMember_dSM12)) {
      return true;
    }
    return false;
  }

  //================================================== DSMeter

  DSMeter::DSMeter(const dsuid_t _dsid, Apartment* _pApartment)
  : m_DSID(_dsid),
    m_DeviceType(BusMember_Unknown),
    m_capHasDevices(false),
    m_capHasMetering(false),
    m_capHasTemperatureControl(false),
    m_PowerConsumption(0),
    m_EnergyMeterValue(0),
    m_EnergyMeterValueWh(0),
    m_LastReportedEnergyMeterValue(0),
    m_ReceivedMeterValue(false),
    m_readStoredMeterValues(false),
    m_HardwareVersion(""),
    m_armSoftwareVersion(0),
    m_dspSoftwareVersion(0),
    m_ApiVersion(0),
    m_IsValid(false),
    m_IsInitialized(false),
    m_HasPendingEvents(false),
    m_pApartment(_pApartment),
    m_DatamodelHash(0),
    m_DatamoderModificationCount(0),
    m_BinaryInputEventCount(dsuid2str(_dsid)),
    m_dSMPropertyFlags(0),
    m_IgnoreActionsFromNewDevices(false),
    m_ApartmentState(DSM_APARTMENT_STATE_UNKNOWN),
    m_dSMState(DSM_STATE_UNKNOWN)
  {
    publishToPropertyTree();
  } // ctor

  void DSMeter::publishToPropertyTree() {
    assert(m_pPropertyNode == NULL);
    if((m_pApartment != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
      m_pPropertyNode =
        m_pApartment->getPropertyNode()->createProperty("dSMeters/" + dsuid2str(m_DSID));
      m_pPropertyNode->createProperty("dSUID")->setStringValue(dsuid2str(m_DSID));
      dsid_t dsid;
      if (dsuid_to_dsid(m_DSID, &dsid)) {
        m_pPropertyNode->createProperty("dSID")->setStringValue(dsid2str(dsid));
      } else {
        m_pPropertyNode->createProperty("dSID")->setStringValue("");
      }
      m_pPropertyNode->createProperty("DisplayID")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, std::string, false>(*this, &DSMeter::getDisplayID));
      m_pPropertyNode->createProperty("name")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, std::string>(*this, &DSMeter::getName, &DSMeter::setName));

      m_pPropertyNode->createProperty("isValid")
        ->linkToProxy(PropertyProxyReference<bool>(m_IsValid, false));
      m_pPropertyNode->createProperty("isInitialized")
        ->linkToProxy(PropertyProxyReference<bool>(m_IsInitialized, false));
      m_pPropertyNode->createProperty("state")
        ->linkToProxy(PropertyProxyReference<int>(m_dSMState, false));
      m_pPropertyNode->createProperty("present")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, bool, false>(*this, &DSMeter::isPresent));
      m_pPropertyNode->createProperty("busMemberType")
        ->linkToProxy(PropertyProxyReference<int, BusMemberDevice_t>(m_DeviceType, false));

      PropertyNodePtr capNode = m_pPropertyNode->createProperty("capabilities");
      capNode->createProperty("devices")
        ->linkToProxy(PropertyProxyReference<bool>(m_capHasDevices, false));
      capNode->createProperty("metering")
        ->linkToProxy(PropertyProxyReference<bool>(m_capHasMetering, false));
      capNode->createProperty("temperature-control")
        ->linkToProxy(PropertyProxyReference<bool>(m_capHasTemperatureControl, false));

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

      m_pPropertyNode->createProperty("hardwareVersion")->setIntegerValue(0);
      m_pPropertyNode->createProperty("hardwareVersion2")
        ->linkToProxy(PropertyProxyReference<std::string>(m_HardwareVersion, false));
      m_pPropertyNode->createProperty("softwareVersion")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, std::string, false>(*this, &DSMeter::getSoftwareVersion));
      m_pPropertyNode->createProperty("armSoftwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_armSoftwareVersion, false));
      m_pPropertyNode->createProperty("dspSoftwareVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_dspSoftwareVersion, false));
      m_pPropertyNode->createProperty("apiVersion")
        ->linkToProxy(PropertyProxyReference<int>(m_ApiVersion, false));
      m_pPropertyNode->createProperty("hardwareName")
        ->linkToProxy(PropertyProxyMemberFunction<DSMeter, std::string, false>(*this, &DSMeter::getHardwareName));
      m_pPropertyNode->createProperty("ignoreActionsFromNewDevices")
        ->linkToProxy(PropertyProxyReference<bool>(m_IgnoreActionsFromNewDevices, false));

      m_pPropertyNode->createProperty("ConfigURL")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcConfigURL, false));
      m_pPropertyNode->createProperty("ModelUID")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcModelUID, false));
      m_pPropertyNode->createProperty("HardwareGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareGuid, false));
      m_pPropertyNode->createProperty("HardwareModelGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareModelGuid, false));
      m_pPropertyNode->createProperty("VendorGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcVendorGuid, false));
      m_pPropertyNode->createProperty("OemGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcOemGuid, false));
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
                                 dsuid2str(m_DSID) + " device: " + dsuid2str(_device.getDSID()), lsFatal);
    }
  } // addDevice

  void DSMeter::removeDevice(const DeviceReference& _device) {
    DeviceIterator pos = std::find(m_ConnectedDevices.begin(), m_ConnectedDevices.end(), _device);
    if(pos != m_ConnectedDevices.end()) {
      m_ConnectedDevices.erase(pos);
    } else {
      Logger::getInstance()->log("DSMeter::removeDevice: not found, dSM: " +
          dsuid2str(m_DSID) + " device: " + dsuid2str(_device.getDSID()), lsInfo);
    }
  } // removeDevice

  dsuid_t DSMeter::getDSID() const {
    return m_DSID;
  } // getDSID

  std::string DSMeter::getDisplayID() const
  {
    dsid_t dsid;
    std::string displayID;
    if (dsuid_to_dsid(m_DSID, &dsid)) {
      displayID = dsid2str(dsid);
      displayID = displayID.substr(displayID.size() - 8);
    } else {
      displayID = dsuid2str(m_DSID).substr(0, 8) + "\u2026";
    }
    if (m_DSID.id[16] != 0) {
      displayID += "-" + intToString(m_DSID.id[16]);
    }
    return displayID;
  }

  unsigned long DSMeter::getPowerConsumption() {
    DateTime now;
    if (!(now.addSeconds(-1) < m_PowerConsumptionTimeStamp)) {
      unsigned long newvalue = 0;
      if(isPresent() && getCapability_HasMetering()) {
        newvalue = DSS::getInstance()->getBusInterface().getMeteringBusInterface()->getPowerConsumption(m_DSID);
      }
      setPowerConsumption(newvalue);
    }
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long long DSMeter::getEnergyMeterValue() {
    DateTime now;
    if (!(now.addSeconds(-1) < m_EnergyMeterValueTimeStamp)) {
      unsigned long long newValue;
      if(isPresent() && getCapability_HasMetering()) {
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
    if (!m_readStoredMeterValues) {
      // get the last/first value from the database
      unsigned long lastEnergyCounter;
      if (DSS::hasInstance()) {
        lastEnergyCounter = DSS::getInstance()->getMetering().
                                                getLastEnergyCounter(shared_from_this());
        initializeEnergyMeterValue(lastEnergyCounter);
        m_readStoredMeterValues = true;
      }
    }
    if(!m_ReceivedMeterValue && (m_EnergyMeterValue > 0.1)) {
      // Do nothing, the first value is just to set a baseline
    } else if((_value >= m_LastReportedEnergyMeterValue) &&
              ((_value - m_LastReportedEnergyMeterValue) < (32 * 250))) {
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

  void DSMeter::setPropertyFlags(std::bitset<8> _flags) {
    m_dSMPropertyFlags = _flags;
    m_IgnoreActionsFromNewDevices = m_dSMPropertyFlags.test(4);
  }

  std::string DSMeter::getHardwareName() const {
    if (!m_HardwareName.empty()) {
      return m_HardwareName;
    }

    switch (getBusMemberType()) {
    case BusMember_dSM11:
      return "dSM11";
    case BusMember_dSM12:
      return "dSM12";
    default:
      return "";
    }
  }

  std::string DSMeter::getSoftwareVersion() const {
    if (!m_softwareVersion.empty()) {
      return m_softwareVersion;
    }

    return intToString((m_armSoftwareVersion >> 24) & 0xFF) + "." +
           intToString((m_armSoftwareVersion >> 16) & 0xFF) + "." +
           intToString((m_armSoftwareVersion >> 8) & 0xFF) + "." +
           intToString(m_armSoftwareVersion & 0xFF);
  }
} // namespace dss

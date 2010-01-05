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
#include "core/DS485Interface.h"
#include "set.h"

namespace dss {

  //================================================== DSMeter

  DSMeter::DSMeter(const dsid_t _dsid)
  : m_DSID(_dsid),
    m_BusID(0xFF),
    m_PowerConsumption(0),
    m_EnergyMeterValue(0),
    m_IsValid(false)
  {
  } // ctor

  Set DSMeter::getDevices() const {
    return m_ConnectedDevices;
  } // getDevices

  void DSMeter::addDevice(const DeviceReference& _device) {
    if(!contains(m_ConnectedDevices, _device)) {
      m_ConnectedDevices.push_back(_device);
    } else {
      Logger::getInstance()->log("DSMeter::addDevice: DUPLICATE DEVICE Detected dsMeter: " + intToString(m_BusID) + " device: " + _device.getDSID().toString(), lsFatal);
    }
  } // addDevice

  void DSMeter::removeDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_ConnectedDevices.begin(), m_ConnectedDevices.end(), _device);
    if(pos != m_ConnectedDevices.end()) {
      m_ConnectedDevices.erase(pos);
    }
  } // removeDevice

  dsid_t DSMeter::getDSID() const {
    return m_DSID;
  } // getDSID

  int DSMeter::getBusID() const {
    return m_BusID;
  } // getBusID

  void DSMeter::setBusID(const int _busID) {
    m_BusID = _busID;
  } // setBusID

  unsigned long DSMeter::getPowerConsumption() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_PowerConsumptionAge)) {
      m_PowerConsumption =  DSS::getInstance()->getDS485Interface().getPowerConsumption(m_BusID);
      m_PowerConsumptionAge = now;
    }
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long DSMeter::getEnergyMeterValue() {
    DateTime now;
    if(!now.addSeconds(-1).before(m_EnergyMeterValueAge)) {
      m_EnergyMeterValue = DSS::getInstance()->getDS485Interface().getEnergyMeterValue(m_BusID);
      m_EnergyMeterValueAge = now;
    }
    return m_EnergyMeterValue;
  } // getEnergyMeterValue


  /** set the consumption in mW */
  void DSMeter::setPowerConsumption(unsigned long _value)
  {
    DateTime now;
    m_PowerConsumptionAge = now;
    m_PowerConsumption = _value;
  }

  /** set the meter value in Wh */
  void DSMeter::setEnergyMeterValue(unsigned long _value)
  {
    DateTime now;
    m_EnergyMeterValueAge = now;
    m_EnergyMeterValue = _value;
  }

  unsigned long DSMeter::getCachedPowerConsumption() {
    return m_PowerConsumption;
  } // getPowerConsumption

  unsigned long DSMeter::getCachedEnergyMeterValue() {
    return m_EnergyMeterValue;
  } // getEnergyMeterValue


} // namespace dss

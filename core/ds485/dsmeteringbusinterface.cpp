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

#include "dsmeteringbusinterface.h"

#include "dsbusinterface.h"

namespace dss {

  //================================================== DSMeteringBusInterface

  unsigned long DSMeteringBusInterface::getPowerConsumption(const dsid_t& _dsMeterID) {
    uint32_t power;
    int ret = CircuitEnergyMeterValue_get(m_DSMApiHandle, _dsMeterID, &power, NULL);
    DSBusInterface::checkResultCode(ret);

    return power;
  } // getPowerConsumption

  void DSMeteringBusInterface::requestPowerConsumption() {
    uint32_t power;
    int ret = CircuitEnergyMeterValue_get(m_DSMApiHandle, m_BroadcastDSID, &power, NULL);
    DSBusInterface::checkResultCode(ret);
  } // requestPowerConsumption

  unsigned long DSMeteringBusInterface::getEnergyMeterValue(const dsid_t& _dsMeterID) {
    uint32_t energy;
    int ret = CircuitEnergyMeterValue_get(m_DSMApiHandle, _dsMeterID, NULL, &energy);
    DSBusInterface::checkResultCode(ret);

    return energy;
  } // getEnergyMeterValue

  void DSMeteringBusInterface::requestEnergyMeterValue() {
    uint32_t energy;
    int ret = CircuitEnergyMeterValue_get(m_DSMApiHandle, m_BroadcastDSID, NULL, &energy);
    DSBusInterface::checkResultCode(ret);
  } // requestEnergyMeterValue

} // namespace dss
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

#include "src/dsidhelper.h"

#include "dsbusinterface.h"

namespace dss {

  //================================================== DSMeteringBusInterface

  unsigned long DSMeteringBusInterface::getPowerConsumption(const dss_dsid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint32_t power;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);

    int ret = CircuitEnergyMeterValue_Ws_get(m_DSMApiHandle, dsmDSID, &power, NULL);
    DSBusInterface::checkResultCode(ret);

    return power;
  } // getPowerConsumption

  void DSMeteringBusInterface::requestMeterData() {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    int ret = CircuitEnergyMeterValue_Ws_get(m_DSMApiHandle, m_BroadcastDSID, NULL, NULL);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // requestPowerConsumption

  unsigned long DSMeteringBusInterface::getEnergyMeterValue(const dss_dsid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint32_t energy;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);

    int ret = CircuitEnergyMeterValue_Ws_get(m_DSMApiHandle, dsmDSID, NULL, &energy);
    DSBusInterface::checkResultCode(ret);

    return energy;
  } // getEnergyMeterValue

} // namespace dss

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

#include "simmeteringbusinterface.h"

#include "core/sim/dssim.h"
#include "core/sim/dsmetersim.h"

namespace dss {

  unsigned long SimMeteringBusInterface::getPowerConsumption(const dss_dsid_t& _dsMeterID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      return pMeter->getPowerConsumption();
    }
    return 0;
  } // getPowerConsumption

  void SimMeteringBusInterface::requestMeterData() {
    if(m_pEventSink != NULL) {
      for(int iMeter = 0; iMeter < m_pSimulation->getDSMeterCount(); iMeter++) {
        boost::shared_ptr<DSMeterSim>
          pMeter = m_pSimulation->getDSMeter(iMeter);
        m_pEventSink->onMeteringEvent(NULL, pMeter->getDSID(),
                                      pMeter->getPowerConsumption(),
                                      pMeter->getEnergyMeterValue());
      }
    }
  } // requestPowerConsumption

  unsigned long SimMeteringBusInterface::getEnergyMeterValue(const dss_dsid_t& _dsMeterID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      return pMeter->getEnergyMeterValue();
    }
    return 0;
  } // getEnergyMeterValue

  void SimMeteringBusInterface::setBusEventSink(BusEventSink* _value) {
    m_pEventSink = _value;
  }

} // namespace dss
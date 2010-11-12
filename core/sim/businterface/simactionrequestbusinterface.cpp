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

#include "simactionrequestbusinterface.h"

#include "core/sim/dssim.h"
#include "core/sim/dsmetersim.h"

#include "core/model/group.h"
#include "core/model/device.h"

namespace dss {

  //================================================== SimActionRequestBusInterface

  void SimActionRequestBusInterface::blink(AddressableModelItem* pTarget) {
    // TODO: the simulation can't blink
  } // blink

  void SimActionRequestBusInterface::callScene(AddressableModelItem *pTarget,
                                               const uint16_t scene) {
    Group* pGroup = dynamic_cast<Group*>(pTarget);
    Device* pDevice = dynamic_cast<Device*>(pTarget);
    for(int iMeter =  0; iMeter < m_pSimulation->getDSMeterCount(); iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(pGroup != NULL) {
        pMeter->groupCallScene(pGroup->getZoneID(), pGroup->getID(), scene);
      } else {
        if(pMeter->getDSID() == pDevice->getDSMeterDSID()) {
          pMeter->deviceCallScene(pDevice->getShortAddress(), scene);
        }
      }
    }
  } // callScene

  void SimActionRequestBusInterface::saveScene(AddressableModelItem *pTarget,
                                               const uint16_t scene) {
    Group* pGroup = dynamic_cast<Group*>(pTarget);
    Device* pDevice = dynamic_cast<Device*>(pTarget);
    for(int iMeter =  0; iMeter < m_pSimulation->getDSMeterCount(); iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(pGroup != NULL) {
        pMeter->groupSaveScene(pGroup->getZoneID(), pGroup->getID(), scene);
      } else {
        if(pMeter->getDSID() == pDevice->getDSMeterDSID()) {
          pMeter->deviceSaveScene(pDevice->getShortAddress(), scene);
        }
      }
    }
  } // saveScene

  void SimActionRequestBusInterface::undoScene(AddressableModelItem *pTarget) {
    Group* pGroup = dynamic_cast<Group*>(pTarget);
    Device* pDevice = dynamic_cast<Device*>(pTarget);
    for(int iMeter =  0; iMeter < m_pSimulation->getDSMeterCount(); iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(pGroup != NULL) {
        pMeter->groupUndoScene(pGroup->getZoneID(), pGroup->getID());
      } else {
        if(pMeter->getDSID() == pDevice->getDSMeterDSID()) {
          pMeter->deviceUndoScene(pDevice->getShortAddress());
        }
      }
    }
  } // undoScene

  void SimActionRequestBusInterface::setValue(AddressableModelItem *pTarget,
                                              const double _value) {
    Group* pGroup = dynamic_cast<Group*>(pTarget);
    Device* pDevice = dynamic_cast<Device*>(pTarget);
    for(int iMeter =  0; iMeter < m_pSimulation->getDSMeterCount(); iMeter++) {
      boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(iMeter);
      if(pGroup != NULL) {
        pMeter->groupSetValue(pGroup->getZoneID(), pGroup->getID(), _value);
      } else {
        if(pMeter->getDSID() == pDevice->getDSMeterDSID()) {
          pMeter->deviceSetValue(pDevice->getShortAddress(), _value);
        }
      }
    }
  } // setValue

} // namespace dss

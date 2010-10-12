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

#include "dsactionrequest.h"

#include "core/dsidhelper.h"
#include "core/logger.h"

#include "core/model/group.h"
#include "core/model/device.h"

namespace dss {

  //================================================== DSActionRequest

  void DSActionRequest::callScene(AddressableModelItem *pTarget, const uint16_t scene) {
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ZoneGroupActionRequest_action_call_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), scene);
    } else if(pDevice)  {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_call_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress(), scene);
    }
  }

  void DSActionRequest::saveScene(AddressableModelItem *pTarget, const uint16_t scene) {
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ZoneGroupActionRequest_action_call_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), scene);
    } else if(pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_save_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress(), scene);
    }
  }

  void DSActionRequest::undoScene(AddressableModelItem *pTarget) {
    Group *pGroup = dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ZoneGroupActionRequest_action_undo_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID());
    } else if(pDevice)  {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_undo_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress());
    }
  }

  void DSActionRequest::blink(AddressableModelItem *pTarget) {
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ZoneGroupActionRequest_action_blink(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID());
    } else if(pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_blink(m_DSMApiHandle, dsid, pDevice->getShortAddress());
    }
  }

  void DSActionRequest::setValue(AddressableModelItem *pTarget,const double _value) {
    // TODO: libdsm
    Logger::getInstance()->log("DSBusInterface::setValue(): not implemented yet", lsInfo);
  }

} // namespace dss

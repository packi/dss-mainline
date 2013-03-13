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
#include "dsbusinterface.h"

#include "src/dsidhelper.h"
#include "src/logger.h"

#include "src/model/group.h"
#include "src/model/device.h"

namespace dss {

  //================================================== DSActionRequest

  void DSActionRequest::callScene(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint16_t scene, const bool _force) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      if(_force) {
        ret = ZoneGroupActionRequest_action_force_call_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin, scene);
      } else {
        ret = ZoneGroupActionRequest_action_call_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin, scene);
      }
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice)  {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      if(_force) {
        ret = DeviceActionRequest_action_force_call_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress(), scene);
      } else {
        ret = DeviceActionRequest_action_call_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress(), scene);
      }
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::saveScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_save_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin, scene);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      ret = DeviceActionRequest_action_save_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress(), scene);
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::undoScene(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category, const uint16_t scene) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup = dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_undo_scene_number(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin, scene);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice)  {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      ret = DeviceActionRequest_action_undo_scene_number(m_DSMApiHandle, dsid, pDevice->getShortAddress(), scene);
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::undoSceneLast(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup = dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_undo_scene(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice)  {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      ret = DeviceActionRequest_action_undo_scene(m_DSMApiHandle, dsid, pDevice->getShortAddress());
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::blink(AddressableModelItem *pTarget, const uint16_t _origin, const SceneAccessCategory _category) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_blink(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      ret = DeviceActionRequest_action_blink(m_DSMApiHandle, dsid, pDevice->getShortAddress());
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::setValue(AddressableModelItem *pTarget, const uint16_t _origin, const uint8_t _value) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_set_outval(m_DSMApiHandle, m_BroadcastDSID, pGroup->getZoneID(), pGroup->getID(), _origin, _value);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      ret = DeviceActionRequest_action_set_outval(m_DSMApiHandle, dsid, pDevice->getShortAddress(), _value);
      DSBusInterface::checkResultCode(ret);
    }
  }

} // namespace dss

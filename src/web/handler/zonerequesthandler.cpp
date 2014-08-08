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


#include "zonerequesthandler.h"

#include <digitalSTROM/dsuid/dsuid.h>

#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/apartment.h"
#include "src/model/scenehelper.h"
#include "src/model/modelmaintenance.h"
#include "src/structuremanipulator.h"
#include "src/stringconverter.h"
#include "util.h"
#include "src/web/json.h"

namespace dss {


  //=========================================== ZoneRequestHandler

  ZoneRequestHandler::ZoneRequestHandler(Apartment& _apartment,
                                         StructureModifyingBusInterface* _pBusInterface,
                                         StructureQueryBusInterface* _pQueryBusInterface)
  : m_Apartment(_apartment),
    m_pStructureBusInterface(_pBusInterface),
    m_pStructureQueryBusInterface(_pQueryBusInterface)
  { }


  WebServerResponse ZoneRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");
    bool ok = true;
    std::string errorMessage;
    std::string zoneName = _request.getParameter("name");
    std::string zoneIDString = _request.getParameter("id");
    boost::shared_ptr<Zone> pZone;
    if(!zoneIDString.empty()) {
      int zoneID = strToIntDef(zoneIDString, -1);
      if(zoneID != -1) {
        try {
          pZone = m_Apartment.getZone(zoneID);
        } catch(std::runtime_error& e) {
          ok = false;
          errorMessage = "Could not find zone with id '" + zoneIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse id '" + zoneIDString + "'";
      }
    } else if(!zoneName.empty()) {
      try {
        pZone = m_Apartment.getZone(zoneName);
      } catch(std::runtime_error& e) {
        ok = false;
        errorMessage = "Could not find zone named '" + zoneName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or id to identify zone";
    }
    if(ok) {
      boost::shared_ptr<Group> pGroup;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          pGroup = pZone->getGroup(groupName);
          if(pGroup == NULL) {
            // TODO: this might better be done by the zone
            throw std::runtime_error("dummy");
          }
        } catch(std::runtime_error& e) {
          errorMessage = "Could not find group with name '" + groupName + "'";
          ok = false;
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            pGroup = pZone->getGroup(groupID);
            if(pGroup == NULL) {
              // TODO: this might better be done by the zone
              throw std::runtime_error("dummy");
            }
          } else {
            errorMessage = "Could not parse group id '" + groupIDString + "'";
            ok = false;
          }
        } catch(std::runtime_error& e) {
          errorMessage = "Could not find group with ID '" + groupIDString + "'";
          ok = false;
        }
      }
      if(ok) {
        if(isDeviceInterfaceCall(_request)) {
          boost::shared_ptr<IDeviceInterface> interface;
          if(pGroup != NULL) {
            interface = pGroup;
          }
          if(ok) {
            if(interface == NULL) {
              interface = pZone;
            }
            return handleDeviceInterfaceRequest(_request, interface, _session);
          }
        } else if(_request.getMethod() == "getLastCalledScene") {
          int lastScene = 0;
          if(pGroup != NULL) {
            lastScene = pGroup->getLastCalledScene();
          } else if(pZone != NULL) {
            lastScene = pZone->getGroup(0)->getLastCalledScene();
          } else {
            // should never reach here because ok, would be false
            assert(false);
          }
          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("scene", lastScene);
          return success(resultObj);
        } else if(_request.getMethod() == "getName") {
          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("name", pZone->getName());
          return success(resultObj);
        } else if(_request.getMethod() == "setName") {
          if (_request.hasParameter("newName")) {

            std::string newName = st.convert(_request.getParameter("newName"));
            newName = escapeHTML(newName);

            pZone->setName(newName);
          } else {
            return failure("missing parameter 'newName'");
          }
          return success();
        } else if(_request.getMethod() == "sceneSetName") {
          if(pGroup == NULL) {
            return failure("Need group to work");
          }
          int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
          if(sceneNumber == -1) {
            return failure("Need valid parameter 'sceneNumber'");
          }
          if(!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
            return failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
          }

          if (_request.hasParameter("newName")) {
            std::string name = st.convert(_request.getParameter("newName"));
            name = escapeHTML(name);

            pGroup->setSceneName(sceneNumber, name);
            StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
            manipulator.sceneSetName(pGroup, sceneNumber, name);
            // raise ModelDirty to force rewrite of model data to apartment.xml
            DSS::getInstance()->getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
          } else {
            return failure("missing parameter 'newName'");
          }
          return success();
        } else if(_request.getMethod() == "sceneGetName") {
          if(pGroup == NULL) {
            return failure("Need group to work");
          }
          int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
          if(sceneNumber == -1) {
            return failure("Need valid parameter 'sceneNumber'");
          }
          if(!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
            return failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
          }

          std::string sceneName = pGroup->getSceneName(sceneNumber);
          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("name", sceneName);
          return success(resultObj);
        } else if(_request.getMethod() == "getReachableScenes") {
          uint64_t reachableScenes = 0uLL;
          Set devicesInZone;
          if (pGroup) {
            devicesInZone = pZone->getDevices().getByGroup(pGroup);
          } else {
            devicesInZone = pZone->getDevices();
          }
          for(int iDevice = 0; iDevice < devicesInZone.length(); iDevice++) {
            DeviceReference& ref = devicesInZone.get(iDevice);
            int buttonID = ref.getDevice()->getButtonID();
            reachableScenes |= SceneHelper::getReachableScenesBitmapForButtonID(buttonID);
          }
          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          boost::shared_ptr<JSONArray<int> > reachableArray(new JSONArray<int>());
          resultObj->addElement("reachableScenes", reachableArray);
          for(int iBit = 0; iBit < 64; iBit++) {
            if((reachableScenes & (1uLL << iBit)) != 0uLL) {
              reachableArray->add(iBit);
            }
          }
          return success(resultObj);
        } else if(_request.getMethod() == "setSensorSource") {
          if (pZone->getID() == 0) {
            return failure("Not allowed to assign sensor for zone 0");
          }
          std::string dsuidStr = _request.getParameter("dsuid");
          if (dsuidStr.empty()) {
            return failure("Missing parameter 'dsuid'");
          }
          dsuid_t dsuid = str2dsuid(dsuidStr);

          int type = strToIntDef(_request.getParameter("sensorType"), -1);
          if (type < 0) {
            return failure("Missing or invalid parameter 'sensorType'");
          }
          boost::shared_ptr<Device> dev = m_Apartment.getDeviceByDSID(dsuid);
          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.setZoneSensor(pZone, type, dev);
          return success();
        } else if(_request.getMethod() == "clearSensorSource") {
          if (pZone->getID() == 0) {
            return success();
          }
          int type = strToIntDef(_request.getParameter("sensorType"), -1);
          if (type < 0) {
            return failure("Missing or invalid parameter 'sensorType'");
          }
          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.resetZoneSensor(pZone, type);
          return success();
        } else {
          throw std::runtime_error("Unhandled function");
        }
      }
    }
    if(!ok) {
      return failure(errorMessage);
    } else {
      return success(); // TODO: check this, we shouldn't get here
    }
  } // handleRequest

} // namespace dss

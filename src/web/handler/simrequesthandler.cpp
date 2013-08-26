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

#include "simrequesthandler.h"

#include "src/model/modelconst.h"
#include "src/model/group.h"
#include "src/model/zone.h"
#include "src/model/apartment.h"

namespace dss {


  //=========================================== SimRequestHandler

  SimRequestHandler::SimRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  WebServerResponse SimRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(beginsWith(_request.getMethod(), "switch")) {
      if(_request.getMethod() == "switch/pressed") {
        int buttonNr = strToIntDef(_request.getParameter("buttonNumber"), -1);
        if(buttonNr == -1) {
          return failure("Invalid button number");
        }

        int zoneID = strToIntDef(_request.getParameter("zoneID"), -1);
        if(zoneID == -1) {
          return failure("Could not parse zoneID");
        }
        int groupID = strToIntDef(_request.getParameter("groupID"), -1);
        if(groupID == -1) {
          return failure("Could not parse groupID");
        }
        try {
          boost::shared_ptr<Zone> zone = m_Apartment.getZone(zoneID);
          boost::shared_ptr<Group> pGroup = zone->getGroup(groupID);

          if(pGroup == NULL) {
            return failure("Could not find group");
          }

          switch(buttonNr) {
          case 1: // upper-left
          case 3: // upper-right
          case 7: // lower-left
          case 9: // lower-right
            break;
          case 2: // up
            pGroup->increaseValue(coSim, SAC_UNKNOWN);
            break;
          case 8: // down
            pGroup->decreaseValue(coSim, SAC_UNKNOWN);
            break;
          case 4: // left
            pGroup->previousScene(coSim, SAC_UNKNOWN);
            break;
          case 6: // right
            pGroup->nextScene(coSim, SAC_UNKNOWN);
            break;
          case 5:
            {
              if(groupID == GroupIDGreen) {
                m_Apartment.getGroup(0)->callScene(coSim, SAC_UNKNOWN, SceneBell, "", false);
              } else if(groupID == GroupIDRed){
                m_Apartment.getGroup(0)->callScene(coSim, SAC_UNKNOWN, SceneAlarm, "", false);
              } else {
                const int lastScene = pGroup->getLastCalledScene();
                if(lastScene == SceneOff || lastScene == SceneDeepOff ||
                  lastScene == SceneStandBy || lastScene == ScenePanic)
                {
                  pGroup->callScene(coSim, SAC_UNKNOWN, Scene1, "", false);
                } else {
                  pGroup->callScene(coSim, SAC_UNKNOWN, SceneOff, "", false);
                }
              }
            }
            break;
          default:
            return failure("Invalid button nr (range is 1..9)");
          }
        } catch(std::runtime_error&) {
          return failure("Could not find zone");
        }
        return success();
      }
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


} // namespace dss

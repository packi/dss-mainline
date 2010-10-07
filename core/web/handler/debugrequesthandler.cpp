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

#include "debugrequesthandler.h"

#include <sstream>
#include <boost/scoped_ptr.hpp>

#include "core/web/json.h"

// TODO: libdsm
// #include "core/ds485/ds485.h"
// #include "core/ds485/framebucketcollector.h"
#include "core/ds485/ds485proxy.h"

#include "core/dss.h"
#include "core/propertysystem.h"

#include "core/ds485const.h"

#include "core/model/apartment.h"
#include "core/model/device.h"


namespace dss {


  //=========================================== DebugRequestHandler

  DebugRequestHandler::DebugRequestHandler(DSS& _dss)
  : m_DSS(_dss)
  { }

  boost::shared_ptr<JSONObject> DebugRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::ostringstream logSStream;
    if(_request.getMethod() == "sendFrame") {

#if 0
      // TODO: libdsm
      int destination = strToIntDef(_request.getParameter("destination"),0) & 0x3F;
      bool broadcast = _request.getParameter("broadcast") == "true";
      int counter = strToIntDef(_request.getParameter("counter"), 0x00) & 0x03;
      int command = strToIntDef(_request.getParameter("command"), 0x09 /* request */) & 0x00FF;
      int length = strToIntDef(_request.getParameter("length"), 0x00) & 0x0F;

      logSStream
          << "sending frame: "
          << "\ndest:    " << destination
          << "\nbcst:    " << broadcast
          << "\ncntr:    " << counter
          << "\ncmd :    " << command
          << "\nlen :    " << length;

      Logger::getInstance()->log(logSStream.str());
      logSStream.str("");

      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->getHeader().setBroadcast(broadcast);
      frame->getHeader().setDestination(destination);
      frame->getHeader().setCounter(counter);
      frame->setCommand(command);
      for(int iByte = 0; iByte < length; iByte++) {
        uint8_t byte = strToIntDef(_request.getParameter(std::string("payload_") + intToString(iByte+1)), 0xFF);

        logSStream << "b" << std::dec << iByte << ": " << std::hex << (int)byte << "\n";
        Logger::getInstance()->log(logSStream.str());
        logSStream.str("");

        frame->getPayload().add<uint8_t>(byte);
      }

      logSStream << std::dec << "done" << std::endl;
      Logger::getInstance()->log(logSStream.str());
      logSStream.str("");

      BusInterface* intf = &DSS::getInstance()->getBusInterface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
      } else {
        delete frame;
      }
      return success();
#endif
    } else if(_request.getMethod() == "pingDevice") {


#if 0
      // TODO: libdsm
      if (DSS::getInstance()->getPropertySystem().getBoolValue("/system/js/settings/extendedPing/active") == true) {
        return failure("Another ping round is currently active, please try again later!");
      }
      std::string deviceDSIDString = _request.getParameter("dsid");
      if(deviceDSIDString.empty()) {
        return failure("Missing parameter 'dsid'");
      }
      try {
        DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", true, true, true);
        dss_dsid_t deviceDSID = dss_dsid_t::fromString(deviceDSIDString);
        boost::shared_ptr<Device> device = m_DSS.getApartment().getDeviceByDSID(deviceDSID);
        // TODO: move this code somewhere else (might also relax the requirement
        //       having DSS as constructor parameter)
        DS485CommandFrame* frame = new DS485CommandFrame();
        frame->getHeader().setBroadcast(false);
        frame->getHeader().setDestination(device->getDSMeterID());
        frame->setCommand(CommandRequest);
        frame->getPayload().add<uint8_t>(FunctionDeviceGetTransmissionQuality);
        frame->getPayload().add<uint16_t>(device->getShortAddress());
        BusInterface* intf = &m_DSS.getBusInterface();
        DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
        if(proxy != NULL) {
          boost::shared_ptr<FrameBucketCollector> bucket = proxy->sendFrameAndInstallBucket(*frame, FunctionDeviceGetTransmissionQuality);
          boost::shared_ptr<DS485CommandFrame> recFrame;
          int rval;
          do {
            bucket->waitForFrame(2500);
            recFrame = bucket->popFrame();
            if(recFrame == NULL) {
              DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", false, true, true);
              return failure("No result received");
            }
            PayloadDissector pd(recFrame->getPayload());
            pd.get<uint8_t>();
            rval = int(pd.get<uint16_t>());
            if(rval < 0) {
              DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", false, true, true);
              return failure("dSM reported error-code: " + intToString(rval));
            }
          } while (rval != 2);

          PayloadDissector pd(recFrame->getPayload());
          pd.get<uint8_t>();
          pd.get<uint16_t>(); // response index
          pd.get<uint16_t>(); // device address
          int qualityHK = pd.get<uint16_t>();
          int qualityRK = pd.get<uint16_t>();
          boost::shared_ptr<JSONObject> obj(new JSONObject());
          obj->addProperty("qualityHK", qualityHK);
          obj->addProperty("qualityRK", qualityRK);
          DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", false, true, true);
          return success(obj);
        } else {
          delete frame;
          DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", false, true, true);
          return failure("Proxy has a wrong type or is null");
        }
      } catch(ItemNotFoundException&) {
        DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", false, true, true);
        return failure("Could not find device with dsid '" + deviceDSIDString + "'");
      } catch(std::invalid_argument&) {
        DSS::getInstance()->getPropertySystem().setBoolValue("/system/js/settings/extendedPing/active", false, true, true);
        return failure( "Could not parse dsid '" + deviceDSIDString + "'");
      }
#endif

    } else if(_request.getMethod() == "resetZone") {

#if 0
      // TODO: libdsm
      std::string zoneIDStr = _request.getParameter("zoneID");
      int zoneID;
      try {
        zoneID = strToInt(zoneIDStr);
      } catch(std::runtime_error&) {
        return failure("Could not parse Zone ID");
      }
      boost::scoped_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
      frame->getHeader().setBroadcast(true);
      frame->getHeader().setDestination(0);
      frame->setCommand(CommandRequest);
      frame->getPayload().add<uint8_t>(FunctionZoneRemoveAllDevicesFromZone);
      frame->getPayload().add<uint16_t>(zoneID);
      BusInterface* intf = &DSS::getInstance()->getBusInterface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
        return success("Please restart your dSMs");
      } else {
        return failure("Proxy has a wrong type or is null");
      }
#endif

    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss

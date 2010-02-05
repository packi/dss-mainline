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

#include "core/web/json.h"

#include "core/ds485/ds485.h"
#include "core/ds485/ds485proxy.h"
#include "core/ds485/framebucketcollector.h"

#include "core/dss.h"
#include "core/ds485const.h"

#include "core/model/apartment.h"
#include "core/model/device.h"


namespace dss {


  //=========================================== DebugRequestHandler

  DebugRequestHandler::DebugRequestHandler(DSS& _dss)
  : m_DSS(_dss)
  { }

  boost::shared_ptr<JSONObject> DebugRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    std::ostringstream logSStream;
    if(_request.getMethod() == "sendFrame") {
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

      DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
      } else {
        delete frame;
      }
      return success();
    } else if(_request.getMethod() == "dSLinkSend") {
      std::string deviceDSIDString =_request.getParameter("dsid");
      Device* pDevice = NULL;
      if(!deviceDSIDString.empty()) {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        if(!(deviceDSID == NullDSID)) {
          try {
            Device& device = m_DSS.getApartment().getDeviceByDSID(deviceDSID);
            pDevice = &device;
          } catch(std::runtime_error& e) {
            return failure("Could not find device with dsid '" + deviceDSIDString + "'");
          }
        } else {
          return failure("Could not parse dsid '" + deviceDSIDString + "'");
        }
      } else {
        return failure("Missing parameter 'dsid'");
      }

      int iValue = strToIntDef(_request.getParameter("value"), -1);
      if(iValue == -1) {
        return failure("Missing parameter 'value'");
      }
      if(iValue < 0 || iValue > 0x00ff) {
        return failure("Parameter 'value' is out of range (0-0xff)");
      }
      bool writeOnly = false;
      bool lastValue = false;
      if(_request.getParameter("writeOnly") == "true") {
        writeOnly = true;
      }
      if(_request.getParameter("lastValue") == "true") {
        lastValue = true;
      }
      uint8_t result;
      try {
        result = pDevice->dsLinkSend(iValue, lastValue, writeOnly);
      } catch(std::runtime_error& e) {
        return failure(std::string("Error: ") + e.what());
      }
      if(writeOnly) {
        return success();
      } else {
        boost::shared_ptr<JSONObject> obj(new JSONObject());
        obj->addProperty("value", result);
        return success(obj);
      }
    } else if(_request.getMethod() == "pingDevice") {
      std::string deviceDSIDString = _request.getParameter("dsid");
      if(deviceDSIDString.empty()) {
        return failure("Missing parameter 'dsid'");
      }
      try {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        Device& device = m_DSS.getApartment().getDeviceByDSID(deviceDSID);
        // TODO: move this code somewhere else (might also relax the requirement
        //       having DSS as constructor parameter)
        DS485CommandFrame* frame = new DS485CommandFrame();
        frame->getHeader().setBroadcast(false);
        frame->getHeader().setDestination(device.getDSMeterID());
        frame->setCommand(CommandRequest);
        frame->getPayload().add<uint8_t>(FunctionDeviceGetTransmissionQuality);
        frame->getPayload().add<uint16_t>(device.getShortAddress());
        DS485Interface* intf = &m_DSS.getDS485Interface();
        DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
        if(proxy != NULL) {
          boost::shared_ptr<FrameBucketCollector> bucket = proxy->sendFrameAndInstallBucket(*frame, FunctionDeviceGetTransmissionQuality);
          bucket->waitForFrame(5000);

          boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();
          if(recFrame == NULL) {
            return failure("No result received");
          }
          PayloadDissector pd(recFrame->getPayload());
          pd.get<uint8_t>();
          int errC = int(pd.get<uint16_t>());
          if(errC < 0) {
            return failure("dSM reported error-code: " + intToString(errC));
          }
          pd.get<uint16_t>(); // device address
          int qualityHK = pd.get<uint16_t>();
          int qualityRK = pd.get<uint16_t>();
          boost::shared_ptr<JSONObject> obj(new JSONObject());
          obj->addProperty("qualityHK", qualityHK);
          obj->addProperty("qualityRK", qualityRK);
          return success(obj);
        } else {
          delete frame;
          return failure("Proxy has a wrong type or is null");
        }
      } catch(ItemNotFoundException&) {
        return failure("Could not find device with dsid '" + deviceDSIDString + "'");
      } catch(std::invalid_argument&) {
        return failure( "Could not parse dsid '" + deviceDSIDString + "'");
      }
    } else if(_request.getMethod() == "resetZone") {
      std::string zoneIDStr = _request.getParameter("zoneID");
      int zoneID;
      try {
        zoneID = strToInt(zoneIDStr);
      } catch(std::runtime_error&) {
        return failure("Could not parse Zone ID");
      }
      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->getHeader().setBroadcast(true);
      frame->getHeader().setDestination(0);
      frame->setCommand(CommandRequest);
      frame->getPayload().add<uint8_t>(FunctionZoneRemoveAllDevicesFromZone);
      frame->getPayload().add<uint8_t>(zoneID);
      DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
        return success("Please restart your dSMs");
      } else {
        delete frame;
        return failure("Proxy has a wrong type or is null");
      }
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "circuitrequesthandler.h"

#include <digitalSTROM/dsuid.h>

#include "src/ds485types.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/model/modulator.h"
#include "src/stringconverter.h"
#include "src/structuremanipulator.h"
#include "src/util.h"
#include "src/vdc-connection.h"
#include "src/protobufjson.h"

namespace dss {

  //=========================================== CircuitRequestHandler

  CircuitRequestHandler::CircuitRequestHandler(Apartment& _apartment,
                                               StructureModifyingBusInterface* _pStructureBusInterface,
                                               StructureQueryBusInterface* _pStructureQueryBusInterface)
  : m_Apartment(_apartment),
    m_pStructureBusInterface(_pStructureBusInterface),
    m_pStructureQueryBusInterface(_pStructureQueryBusInterface)
  { }

  WebServerResponse CircuitRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    std::string idString = _request.getParameter("id");
    std::string dsuidStr = _request.getParameter("dsuid");
    if (idString.empty() && dsuidStr.empty()) {
      return JSONWriter::failure("Missing parameter dsuid");
    }

    dsuid_t dsuid;
    try {
      dsuid = dsidOrDsuid2dsuid(idString, dsuidStr);
    } catch(std::runtime_error& e) {
      return JSONWriter::failure(e.what());
    }

    std::string params;
    vdcapi::PropertyElement parsedParamsElement;
    const vdcapi::PropertyElement* paramsElement = &vdcapi::PropertyElement::default_instance();
    if (_request.getParameter("params", params) && !params.empty()) {
      parsedParamsElement = ProtobufToJSon::jsonToElement(params);
      paramsElement = &parsedParamsElement;
    }

    JSONWriter json;
    try {
      boost::shared_ptr<DSMeter> dsMeter = m_Apartment.getDSMeterByDSID(dsuid);
      if(_request.getMethod() == "getName") {
        json.add("name", dsMeter->getName());
        return json.successJSON();
      } else if(_request.getMethod() == "setName") {
        if(_request.hasParameter("newName")) {
          std::string nameStr = _request.getParameter("newName");
          nameStr = escapeHTML(nameStr);
          dsMeter->setName(nameStr);
          if (m_pStructureBusInterface != NULL) {
            StructureManipulator manipulator(*m_pStructureBusInterface,
                                             *m_pStructureQueryBusInterface,
                                             m_Apartment);
            manipulator.meterSetName(dsMeter, nameStr);
          }
          return json.success();
        } else {
          return json.failure("missing parameter newName");
        }
      } else if(_request.getMethod() == "getConsumption") {
        if (!dsMeter->getCapability_HasMetering()) {
          return json.failure("Metering not supported on this device");
        }
        json.add("consumption", static_cast<unsigned long long>(dsMeter->getPowerConsumption()));
        return json.successJSON();
      } else if(_request.getMethod() == "getEnergyMeterValue") {
        if (!dsMeter->getCapability_HasMetering()) {
          return JSONWriter::failure("Metering not supported on this device");
        }
        json.add("meterValue", dsMeter->getEnergyMeterValue());
        return json.successJSON();
      } else if(_request.getMethod() == "rescan") {
        dsMeter->setIsValid(false);
        return json.success();
      } else if(_request.getMethod() == "learnIn") {
        int timeout = -1;
        _request.getParameter("timeout", timeout);
        vdcapi::Message res = VdcHelper::callLearningFunction(dsMeter->getDSID(), true, timeout, *paramsElement);
        json.add("response");
        ProtobufToJSon::protoPropertyToJson(res, json);
        return json.successJSON();
      } else if(_request.getMethod() == "learnOut") {
        int timeout = -1;
        _request.getParameter("timeout", timeout);
        vdcapi::Message res = VdcHelper::callLearningFunction(dsMeter->getDSID(), false, timeout, *paramsElement);
        json.add("response");
        ProtobufToJSon::protoPropertyToJson(res, json);
        return json.successJSON();
      } else if(_request.getMethod() == "firmwareCheck") {
        vdcapi::Message res = VdcHelper::callFirmwareFunction(dsMeter->getDSID(), true, false, *paramsElement);
        json.add("response");
        ProtobufToJSon::protoPropertyToJson(res, json);
        return json.successJSON();
      } else if(_request.getMethod() == "firmwareUpdate") {
        bool clearSettings = false;
        _request.getParameter("clearsettings", clearSettings);
        vdcapi::Message res = VdcHelper::callFirmwareFunction(dsMeter->getDSID(), false, clearSettings, *paramsElement);
        json.add("response");
        ProtobufToJSon::protoPropertyToJson(res, json);
        return json.successJSON();
      } else if(_request.getMethod() == "storeAccessToken") {
        std::string authdata;
        std::string vdcPath("authentication");
        std::string vdcScope;
        if (_request.hasParameter("authdata")) {
          authdata = _request.getParameter("authdata");
        } else {
          return json.failure("missing parameter authdata");
        }
        if (_request.hasParameter("path")) {
          vdcPath = _request.getParameter("path");
        }
        if (_request.hasParameter("scope")) {
          vdcScope = _request.getParameter("scope");
        }
        google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
        vdcapi::PropertyElement* param0 = query.Add();
        param0->set_name(vdcPath);
        vdcapi::PropertyElement* param1 = param0->add_elements();
        param1->set_name("authdata");
        param1->mutable_value()->set_v_string(authdata);
        if (!vdcScope.empty()) {
          vdcapi::PropertyElement* param2 = param0->add_elements();
          param2->set_name("scope");
          param2->mutable_value()->set_v_string(vdcScope);
        }
        vdcapi::Message res = VdcConnection::setProperty(dsMeter->getDSID(), dsMeter->getDSID(), query);
        json.add("response");
        ProtobufToJSon::protoPropertyToJson(res, json);
        return json.successJSON();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    } catch(ItemNotFoundException&) {
      return JSONWriter::failure("Could not find dSMeter with given dSUID");
    }
  } // handleRequest

} // namespace dss

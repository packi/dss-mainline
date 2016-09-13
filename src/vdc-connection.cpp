/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "vdc-connection.h"

#include <iostream>

#include "base.h"
#include "util.h"
#include "ds485types.h"
#include "dss.h"
#include "model/apartment.h"
#include "ds485/dsbusinterface.h"
#include "messages/vdc-messages.pb.h"
#include "stringconverter.h"
#include "logger.h"
#include "model-features.h"
#include "vdc-element-reader.h"

namespace dss {

  VdsdSpec_t VdcHelper::getSpec(dsuid_t _vdsm, dsuid_t _device) {
    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty *getprop =
                                    message.mutable_vdsm_request_get_property();
    getprop->set_dsuid(dsuid2str(_device));

    vdcapi::PropertyElement *query = getprop->add_query();
    query->set_name("hardwareModelGuid");
    query = getprop->add_query();
    query->set_name("modelUID");
    query = getprop->add_query();
    query->set_name("modelFeatures");
    query = getprop->add_query();
    query->set_name("vendorGuid");
    query = getprop->add_query();
    query->set_name("oemGuid");
    query = getprop->add_query();
    query->set_name("oemModelGuid");
    query = getprop->add_query();
    query->set_name("configURL");
    query = getprop->add_query();
    query->set_name("hardwareGuid");
    query = getprop->add_query();
    query->set_name("model");
    query = getprop->add_query();
    query->set_name("hardwareVersion");
    query = getprop->add_query();
    query->set_name("name");

    uint8_t buffer_in[4096];
    uint8_t buffer_out[4096];
    uint16_t bs;

    memset(buffer_in, 0, sizeof(buffer_in));
    memset(buffer_out, 0, sizeof(buffer_out));

    if (!message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
      throw std::runtime_error("could not serialize message");
    }

    if (DSS::hasInstance()) {
      DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
          _vdsm, message.ByteSize(), buffer_in, &bs, buffer_out);
    } else {
      throw std::runtime_error("!DSS::hasInstance()");
    }

    message.Clear();
    if (bs > sizeof(buffer_out)) {
      throw std::runtime_error("incoming message too large, dropping");
    }

    if (!message.ParseFromArray(buffer_out, bs)) {
      throw std::runtime_error("could not parse response message");
    }

    // error message
    if (message.type() == vdcapi::GENERIC_RESPONSE) {
      throw std::runtime_error("received error with code " +
                               intToString(message.generic_response().code()));
    }
    if (!message.has_vdc_response_get_property()) {
      throw std::runtime_error("received unexpected reply");
    }

    VdsdSpec_t ret;
    VdcElementReader rootReader(message.vdc_response_get_property().properties());
    ret.hardwareModelGuid = rootReader["hardwareModelGuid"].getValueAsString();
    ret.vendorGuid = rootReader["vendorGuid"].getValueAsString();
    ret.oemGuid = rootReader["oemGuid"].getValueAsString();
    ret.oemModelGuid = rootReader["oemModelGuid"].getValueAsString();
    ret.configURL = rootReader["configURL"].getValueAsString();
    ret.hardwareGuid = rootReader["hardwareGuid"].getValueAsString();
    ret.hardwareInfo = rootReader["model"].getValueAsString();
    ret.modelUID = rootReader["modelUID"].getValueAsString();
    ret.hardwareVersion = rootReader["hardwareVersion"].getValueAsString();
    ret.name = rootReader["name"].getValueAsString();

    ret.modelFeatures = boost::make_shared<std::vector<int> >();
    std::vector<int>& features = *ret.modelFeatures;
    VdcElementReader featuresReader = rootReader["modelFeatures"];
    for (VdcElementReader::iterator it = featuresReader.begin(); it != featuresReader.end(); it++) {
      VdcElementReader featureReader = *it;
      if (!featureReader.getValueAsBool()) {
        continue;
      }
      const std::string& featureName = featureReader.getName();
      try {
        features.push_back(ModelFeatures::getInstance()->nameToFeature(featureName));
      } catch (std::runtime_error &ex) {
        Logger::getInstance()->log("Ignoring feature '" + featureName + "' from device " +
                                    dsuid2str(_device));
      }
    }

    return ret;
  }

  boost::shared_ptr<VdcSpec_t> VdcHelper::getCapabilities(dsuid_t _vdsm) {
    vdcapi::Message message;
    boost::shared_ptr<VdcSpec_t> ret = boost::make_shared<VdcSpec_t>();

    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty *getprop =
                                    message.mutable_vdsm_request_get_property();
    getprop->set_dsuid(dsuid2str(_vdsm));

    vdcapi::PropertyElement *query = getprop->add_query();
    query->set_name("capabilities");
    query = getprop->add_query();
    query->set_name("modelVersion");
    query = getprop->add_query();
    query->set_name("model");
    query = getprop->add_query();
    query->set_name("hardwareVersion");
    query = getprop->add_query();
    query->set_name("hardwareModelGuid");
    query = getprop->add_query();
    query->set_name("modelUID");
    query = getprop->add_query();
    query->set_name("vendorGuid");
    query = getprop->add_query();
    query->set_name("oemGuid");
    query = getprop->add_query();
    query->set_name("oemModelGuid");
    query = getprop->add_query();
    query->set_name("configURL");
    query = getprop->add_query();
    query->set_name("hardwareGuid");
    query = getprop->add_query();
    query->set_name("name");


    uint8_t buffer_in[4096];
    uint8_t buffer_out[4096];
    uint16_t bs;

    memset(buffer_in, 0, sizeof(buffer_in));
    memset(buffer_out, 0, sizeof(buffer_out));

    if (!message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
      throw std::runtime_error("could not serialize message");
    }

    if (DSS::hasInstance()) {
      DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
          _vdsm, message.ByteSize(), buffer_in, &bs, buffer_out);
    } else {
      return ret;
    }

    message.Clear();
    if (bs > sizeof(buffer_out)) {
      throw std::runtime_error("incoming message too large, dropping");
    }

    if (!message.ParseFromArray(buffer_out, bs)) {
      throw std::runtime_error("could not parse response message");
    }

    // error message
    if (message.type() == vdcapi::GENERIC_RESPONSE) {
      throw std::runtime_error("received error with code " +
                               intToString(message.generic_response().code()));
    }
    if (!message.has_vdc_response_get_property()) {
      throw std::runtime_error("received unexpected reply");
    }

    vdcapi::vdc_ResponseGetProperty response =
                                            message.vdc_response_get_property();

    for (int i = 0; i < response.properties_size(); i++) {
      vdcapi::PropertyElement el = response.properties(i);

      if (!el.has_name() || !el.has_value()) {
        continue;
      }

      StringConverter st("UTF-8", "UTF-8");
      vdcapi::PropertyValue val = el.value();
      if (el.name() == "metering" && val.has_v_bool()) {
        ret->hasMetering = val.v_bool();
      } else if (el.name() == "modelVersion" && val.has_v_string()) {
        try {
          ret->modelVersion = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "model" && val.has_v_string()) {
        try {
          ret->model = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "hardwareVersion" && val.has_v_string()) {
        try {
          ret->hardwareVersion = st.convert(val.v_string());
        } catch (std::exception& e) {}
        // FOKEL
      } else if (el.name() == "hardwareModelGuid") {
        try {
          ret->hardwareModelGuid = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "vendorGuid") {
        try {
          ret->vendorGuid = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "oemGuid") {
        try {
          ret->oemGuid = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "oemModelGuid") {
        try {
          ret->oemModelGuid = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "configURL") {
        try {
          ret->configURL = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "hardwareGuid") {
        try {
          ret->hardwareGuid = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "modelUID") {
        try {
          ret->modelUID = st.convert(val.v_string());
        } catch (std::exception& e) {}
      } else if (el.name() == "name") {
        try {
          ret->name = st.convert(val.v_string());
        } catch (std::exception& e) {}
      }
    }

    return ret;
  }

  void VdcHelper::getIcon(dsuid_t _vdsm, dsuid_t _device, size_t *size, uint8_t **data) {
    vdcapi::Message message;

    *size = 0;
    *data = NULL;

    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty *getprop =
                                    message.mutable_vdsm_request_get_property();
    getprop->set_dsuid(dsuid2str(_device));

    vdcapi::PropertyElement *query = getprop->add_query();
    query->set_name("deviceIcon16");

    uint8_t buffer_in[4096];
    uint8_t buffer_out[4096];
    uint16_t bs = 0;

    memset(buffer_in, 0, sizeof(buffer_in));
    memset(buffer_out, 0, sizeof(buffer_out));

    if (!message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
      throw std::runtime_error("could not serialize message");
    }

    if (DSS::hasInstance()) {
      DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
          _vdsm, message.ByteSize(), buffer_in, &bs, buffer_out);
    } else {
      return;
    }

    message.Clear();
    if (bs > sizeof(buffer_out)) {
      throw std::runtime_error("incoming message too large, dropping");
    }

    if (!message.ParseFromArray(buffer_out, bs)) {
      throw std::runtime_error("could not parse response message");
    }

    // error message
    if (message.type() == vdcapi::GENERIC_RESPONSE) {
      throw std::runtime_error("received error with code " +
                               intToString(message.generic_response().code()));
    }
    if (!message.has_vdc_response_get_property()) {
      throw std::runtime_error("received unexpected reply");
    }

    vdcapi::vdc_ResponseGetProperty response =
                                            message.vdc_response_get_property();

    if (response.properties_size() == 0) {
      return;
    }

    for (int i = 0; i < response.properties_size(); i++) {
      vdcapi::PropertyElement el = response.properties(i);

      if (!el.has_name() || !el.has_value()) {
        continue;
      }

      vdcapi::PropertyValue val = el.value();
      if (el.name() == "deviceIcon16" && (val.has_v_bytes())) {
        std::string icon = val.v_bytes();
        if (icon.length() > 0) {
          *data = (uint8_t *)malloc(sizeof(uint8_t) * icon.length());
          if (*data == NULL) {
            throw std::runtime_error("could not allocate memory for buffer");
          }
          memcpy(*data, icon.data(), icon.length());
          *size = icon.length();
        }
      }
    }

    return;
  }

  VdcHelper::State VdcHelper::getState(dsuid_t _vdsm, dsuid_t _device)
  {
    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty *getprop = message.mutable_vdsm_request_get_property();
    getprop->set_dsuid(dsuid2str(_device));

    {
      vdcapi::PropertyElement *query = getprop->add_query();
      query->set_name("binaryInputStates");
    }
    {
      vdcapi::PropertyElement *query = getprop->add_query();
      query->set_name("deviceStates");
    }

    uint8_t buffer_in[4096];
    uint8_t buffer_out[4096];
    uint16_t bs = 0;

    memset(buffer_in, 0, sizeof(buffer_in));
    memset(buffer_out, 0, sizeof(buffer_out));

    if (!message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
      throw std::runtime_error("could not serialize message");
    }

    if (DSS::hasInstance()) {
      DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
          _vdsm, message.ByteSize(), buffer_in, &bs, buffer_out);
    } else {
      return VdcHelper::State();
    }

    message.Clear();
    if (bs > sizeof(buffer_out)) {
      throw std::runtime_error("incoming message too large, dropping");
    }
    if (!message.ParseFromArray(buffer_out, bs)) {
      throw std::runtime_error("could not parse response message");
    }
    if (message.type() == vdcapi::GENERIC_RESPONSE) {
      throw std::runtime_error("received error with code " +
          intToString(message.generic_response().code()));
    }
    if (!message.has_vdc_response_get_property()) {
      throw std::runtime_error("received unexpected reply");
    }

    Logger::getInstance()->log("VdcHelper::getState: message " + message.DebugString(), lsDebug);
    vdcapi::vdc_ResponseGetProperty response = message.vdc_response_get_property();
    VdcElementReader reader(message.vdc_response_get_property().properties());

    State state;
    std::map<int,int64_t>& binaryInputStates = state.binaryInputStates;
    VdcElementReader biStatesReader = reader["binaryInputStates"];
    for (VdcElementReader::iterator it = biStatesReader.begin(); it != biStatesReader.end(); it++) {
      VdcElementReader biStateReader = *it;
      const std::string& biStateName = biStateReader.getName();
      int biStateIndex = strToInt(biStateName);
      {
        VdcElementReader reader = biStateReader["error"];
        if (reader.getValueAsInt() != 0) {
            Logger::getInstance()->log("VdcHelper::getStateInputValue: device:" + dsuid2str(_device) +
                " name:" + biStateName + " error:" + reader.getValueAsString(), lsWarning);
            continue;
        }
      }
      {
        VdcElementReader reader = biStateReader["extendedValue"];
        if (reader.isValid()) {
            binaryInputStates[biStateIndex] = reader.getValueAsInt();
            continue;
        }
      }
      {
        VdcElementReader reader = biStateReader["value"];
        if (reader.isValid()) {
            binaryInputStates[biStateIndex] = reader.getValueAsBool() ? State_Active : State_Inactive;
            continue;
        }
      }
    }

    std::vector<std::pair<std::string, std::string> >& deviceStates = state.deviceStates;
    VdcElementReader deviceStatesReader = reader["deviceStates"];
    for (VdcElementReader::iterator it = deviceStatesReader.begin(); it != deviceStatesReader.end(); it++) {
      VdcElementReader reader = *it;
      deviceStates.push_back(std::pair<std::string, std::string>(reader.getName(), reader["value"].getValueAsString()));
    }

    for (std::map<int,int64_t>::const_iterator it = binaryInputStates.begin(); it != binaryInputStates.end(); ++it ) {
      Logger::getInstance()->log("VdcHelper::getState: device " + dsuid2str(_device) +
          ": " + intToString(it->first) + "=" + intToString(it->second), lsDebug);
    }
    for (std::vector<std::pair<std::string, std::string> >::const_iterator it = deviceStates.begin();
        it != deviceStates.end(); ++it ) {
      Logger::getInstance()->log("VdcHelper::getState: device " + dsuid2str(_device) +
          ": " + it->first + "=" + it->second, lsDebug);
    }
    return state;
  }

}// namespace

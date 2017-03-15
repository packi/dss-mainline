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
#include <boost/make_shared.hpp>

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
#include "src/vdc-element-reader.h"

namespace dss {

  VdsdSpec_t VdcHelper::getSpec(dsuid_t _vdsm, dsuid_t _device) {
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
    vdcapi::PropertyElement* el = query.Add();
    el->set_name("hardwareModelGuid");
    el = query.Add();
    el->set_name("modelUID");
    el = query.Add();
    el->set_name("modelFeatures");
    el = query.Add();
    el->set_name("vendorGuid");
    el = query.Add();
    el->set_name("vendorId");
    el = query.Add();
    el->set_name("vendorName");
    el = query.Add();
    el->set_name("oemGuid");
    el = query.Add();
    el->set_name("oemModelGuid");
    el = query.Add();
    el->set_name("configURL");
    el = query.Add();
    el->set_name("hardwareGuid");
    el = query.Add();
    el->set_name("model");
    el = query.Add();
    el->set_name("modelVersion");
    el = query.Add();
    el->set_name("hardwareVersion");
    el = query.Add();
    el->set_name("name");
    el = query.Add();
    el->set_name("deviceClass");
    el = query.Add();
    el->set_name("deviceClassVersion");

    vdcapi::Message message = VdcConnection::getProperty(_vdsm, _device, query);

    VdsdSpec_t ret;
    VdcElementReader rootReader(message.vdc_response_get_property().properties());
    ret.hardwareModelGuid = rootReader["hardwareModelGuid"].getValueAsString();
    ret.vendorGuid = rootReader["vendorGuid"].getValueAsString();
    ret.oemGuid = rootReader["oemGuid"].getValueAsString();
    ret.oemModelGuid = rootReader["oemModelGuid"].getValueAsString();
    ret.configURL = rootReader["configURL"].getValueAsString();
    ret.hardwareGuid = rootReader["hardwareGuid"].getValueAsString();
    ret.model = rootReader["model"].getValueAsString();
    ret.modelUID = rootReader["modelUID"].getValueAsString();
    ret.hardwareVersion = rootReader["hardwareVersion"].getValueAsString();
    ret.name = rootReader["name"].getValueAsString();
    ret.vendorId = rootReader["vendorId"].getValueAsString();
    ret.vendorName = rootReader["vendorName"].getValueAsString();
    ret.modelVersion = rootReader["modelVersion"].getValueAsString();
    ret.deviceClass = rootReader["deviceClass"].getValueAsString();
    ret.deviceClassVersion = rootReader["deviceClassVersion"].getValueAsString();

    ret.modelFeatures = boost::make_shared<std::vector<ModelFeatureId> >();
    auto&& features = *ret.modelFeatures;
    VdcElementReader featuresReader = rootReader["modelFeatures"];
    for (VdcElementReader::iterator it = featuresReader.begin(); it != featuresReader.end(); it++) {
      VdcElementReader featureReader = *it;
      if (!featureReader.getValueAsBool()) {
        continue;
      }
      auto&& featureName = featureReader.getName();
      if (auto&& feature = modelFeatureFromName(featureName)) {
        features.push_back(*feature);
      } else {
        Logger::getInstance()->log("Ignoring feature '" + featureName + "' from device " +
                                    dsuid2str(_device));
      }
    }

    return ret;
  }

  boost::shared_ptr<VdcSpec_t> VdcHelper::getCapabilities(dsuid_t _vdsm) {
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
    vdcapi::PropertyElement* el = query.Add();
    el->set_name("capabilities");
    el = query.Add();
    el->set_name("modelVersion");
    el = query.Add();
    el->set_name("model");
    el = query.Add();
    el->set_name("hardwareVersion");
    el = query.Add();
    el->set_name("hardwareModelGuid");
    el = query.Add();
    el->set_name("implementationId");
    el = query.Add();
    el->set_name("modelUID");
    el = query.Add();
    el->set_name("vendorGuid");
    el = query.Add();
    el->set_name("oemGuid");
    el = query.Add();
    el->set_name("oemModelGuid");
    el = query.Add();
    el->set_name("configURL");
    el = query.Add();
    el->set_name("hardwareGuid");
    el = query.Add();
    el->set_name("name");

    vdcapi::Message message = VdcConnection::getProperty(_vdsm, _vdsm, query);

    boost::shared_ptr<VdcSpec_t> ret = boost::make_shared<VdcSpec_t>();
    vdcapi::vdc_ResponseGetProperty response = message.vdc_response_get_property();
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
      } else if (el.name() == "implementationId") {
        try {
          ret->implementationId = st.convert(val.v_string());
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
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
    vdcapi::PropertyElement* el = query.Add();
    el->set_name("deviceIcon16");

    *size = 0;
    *data = NULL;

    vdcapi::Message message = VdcConnection::getProperty(_vdsm, _device, query);

    vdcapi::vdc_ResponseGetProperty response = message.vdc_response_get_property();
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
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
    vdcapi::PropertyElement* el = query.Add();
    el->set_name("binaryInputStates");
    el = query.Add();
    el->set_name("deviceStates");

    vdcapi::Message message = VdcConnection::getProperty(_vdsm, _device, query);

    Logger::getInstance()->log("VdcHelper::getState: message " + message.DebugString(), lsDebug);
    vdcapi::vdc_ResponseGetProperty response = message.vdc_response_get_property();
    VdcElementReader reader(message.vdc_response_get_property().properties());

    State state;
    auto& binaryInputStates = state.binaryInputStates;
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
            binaryInputStates[biStateIndex] = static_cast<BinaryInputStateValue>(reader.getValueAsInt());
            continue;
        }
      }
      {
        VdcElementReader reader = biStateReader["value"];
        if (reader.isValid()) {
           binaryInputStates[biStateIndex] =
                reader.getValueAsBool() ? BinaryInputStateValue::Active : BinaryInputStateValue::Inactive;
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

    for (auto it = binaryInputStates.begin(); it != binaryInputStates.end(); ++it ) {
      Logger::getInstance()->log("VdcHelper::getState: device " + dsuid2str(_device) +
          ": " + intToString(it->first) + "=" + intToString((int)it->second), lsDebug);
    }
    for (std::vector<std::pair<std::string, std::string> >::const_iterator it = deviceStates.begin();
        it != deviceStates.end(); ++it ) {
      Logger::getInstance()->log("VdcHelper::getState: device " + dsuid2str(_device) +
          ": " + it->first + "=" + it->second, lsDebug);
    }
    return state;
  }

  vdcapi::Message VdcHelper::callLearningFunction(dsuid_t vdc, bool establish, int64_t timeout, const vdcapi::PropertyElement& params)
  {
    DeviceBusInterface* deviceBusInterface = DSS::getInstance()->getApartment().getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> learningCall;
    vdcapi::PropertyElement* param0 = learningCall.Add();
    param0->set_name("establish");
    param0->mutable_value()->set_v_bool(establish);
    if (timeout >= 0) {
      vdcapi::PropertyElement* param1 = learningCall.Add();
      param1->set_name("timeout");
      param1->mutable_value()->set_v_int64(timeout);
    }
    if (params.elements_size()) {
      vdcapi::PropertyElement* paramx = learningCall.Add();
      paramx->set_name("params");
      *paramx->mutable_elements() = params.elements();
    }
    return VdcConnection::genericRequest(vdc, vdc, "pair", learningCall);
  }

  vdcapi::Message VdcHelper::callFirmwareFunction(dsuid_t vdc, bool checkOnly, bool clearSettings, const vdcapi::PropertyElement& params)
  {
    DeviceBusInterface* deviceBusInterface = DSS::getInstance()->getApartment().getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> firmwareCall;
    vdcapi::PropertyElement* param0 = firmwareCall.Add();
    param0->set_name("checkonly");
    param0->mutable_value()->set_v_bool(checkOnly);
    vdcapi::PropertyElement* param1 = firmwareCall.Add();
    param1->set_name("clearsettings");
    param1->mutable_value()->set_v_bool(clearSettings);
    if (params.elements_size()) {
      vdcapi::PropertyElement* paramx = firmwareCall.Add();
      paramx->set_name("params");
      *paramx->mutable_elements() = params.elements();
    }
    return VdcConnection::genericRequest(vdc, vdc, "firmwareUpdate", firmwareCall);
  }

  vdcapi::Message VdcHelper::callAuthenticate(dsuid_t vdc, const std::string& authData, const std::string& authScope, const vdcapi::PropertyElement& params)
  {
    DeviceBusInterface* deviceBusInterface = DSS::getInstance()->getApartment().getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> authenticateCall;
    vdcapi::PropertyElement* param0 = authenticateCall.Add();
    param0->set_name("authData");
    param0->mutable_value()->set_v_string(authData);
    vdcapi::PropertyElement* param1 = authenticateCall.Add();
    param1->set_name("authScope");
    param1->mutable_value()->set_v_string(authScope);
    if (params.elements_size()) {
      vdcapi::PropertyElement* paramx = authenticateCall.Add();
      paramx->set_name("params");
      *paramx->mutable_elements() = params.elements();
    }
    return VdcConnection::genericRequest(vdc, vdc, "authenticate", authenticateCall);
  }

  vdcapi::Message VdcConnection::genericRequest(const dsuid_t& vdcId, const dsuid_t& targetId,
    const std::string& methodName,
    const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& params)
  {
    char dsuid_string[DSUID_STR_LEN];
    dsuid_to_string(&targetId, dsuid_string);

    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_GENERIC_REQUEST);
    auto genericRequest = message.mutable_vdsm_request_generic_request();
    genericRequest->set_dsuid(dsuid_string);
    genericRequest->set_methodname(methodName);
    *genericRequest->mutable_params() = params;
    uint8_t arrayOut[REQUEST_LEN];
    if (!message.SerializeToArray(arrayOut, sizeof(arrayOut))) {
      throw std::runtime_error("SerializeToArray failed");
    }

    uint8_t arrayIn[RESPONSE_LEN];
    uint16_t arrayInSize;
    DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
        vdcId, message.ByteSize(), arrayOut, &arrayInSize, arrayIn);

    if (!message.ParseFromArray(arrayIn, arrayInSize)) {
      throw std::runtime_error("ParseFromArray failed");
    }
    if (message.type() != vdcapi::GENERIC_RESPONSE) {
      throw std::runtime_error("Invalid vdc response");
    }
    const vdcapi::GenericResponse& response = message.generic_response();
    if (response.code() != vdcapi::ERR_OK) {
      throw std::runtime_error(std::string("Vdc error code:") + intToString(response.code())
          + " message:" + response.description());
    }
    return message;
  }

  vdcapi::Message VdcConnection::setProperty(const dsuid_t& vdcId, const dsuid_t& targetId,
      const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties)
  {
    char dsuid_string[DSUID_STR_LEN];
    dsuid_to_string(&targetId, dsuid_string);

    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_SET_PROPERTY);
    vdcapi::vdsm_RequestSetProperty* setPropertyRquest = message.mutable_vdsm_request_set_property();
    setPropertyRquest->set_dsuid(dsuid_string);
    *setPropertyRquest->mutable_properties() = properties;
    uint8_t arrayOut[REQUEST_LEN];
    if (!message.SerializeToArray(arrayOut, sizeof(arrayOut))) {
      throw std::runtime_error("SerializeToArray failed");
    }

    uint8_t arrayIn[RESPONSE_LEN];
    uint16_t arrayInSize;
    DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
        vdcId, message.ByteSize(), arrayOut, &arrayInSize, arrayIn);

    if (!message.ParseFromArray(arrayIn, arrayInSize)) {
      throw std::runtime_error("ParseFromArray failed");
    }
    if (message.type() != vdcapi::GENERIC_RESPONSE) {
      throw std::runtime_error("Invalid vdc response");
    }
    const vdcapi::GenericResponse& response = message.generic_response();
    if (response.code() != vdcapi::ERR_OK) {
      throw std::runtime_error(std::string("Vdc error code:") + intToString(response.code())
          + " message:" + response.description());
    }

    return message;
  }

  vdcapi::Message VdcConnection::getProperty(const dsuid_t& vdcId, const dsuid_t& targetId,
      const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query)
  {
    char dsuid_string[DSUID_STR_LEN];
    dsuid_to_string(&targetId, dsuid_string);

    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty* getPropertyRequest = message.mutable_vdsm_request_get_property();
    getPropertyRequest->set_dsuid(dsuid_string);
    *getPropertyRequest->mutable_query() = query;
    uint8_t arrayOut[REQUEST_LEN];
    if (!message.SerializeToArray(arrayOut, sizeof(arrayOut))) {
      throw std::runtime_error("SerializeToArray failed");
    }

    uint8_t arrayIn[RESPONSE_LEN];
    uint16_t arrayInSize;
    DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
            vdcId, message.ByteSize(), arrayOut, &arrayInSize, arrayIn);

    if (!message.ParseFromArray(arrayIn, arrayInSize)) {
      throw std::runtime_error("ParseFromArray failed");
    }
    if (message.type() != vdcapi::VDC_RESPONSE_GET_PROPERTY) {
      throw std::runtime_error("Invalid vdc response");
    }
    return message;
  }

}// namespace

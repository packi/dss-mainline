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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <iostream>

#include "base.h"
#include "vdc-connection.h"
#include "util.h"
#include "ds485types.h"
#include "dss.h"
#include "model/apartment.h"
#include "ds485/dsbusinterface.h"
#include "messages/vdc-messages.pb.h"
#include "stringconverter.h"
#include "logger.h"
#include "model-features.h"

namespace dss {

  boost::shared_ptr<VdsdSpec_t> VdcHelper::getSpec(dsuid_t _vdsm, dsuid_t _device) {
    vdcapi::Message message;
    boost::shared_ptr<VdsdSpec_t> ret = boost::make_shared<VdsdSpec_t>();

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

    boost::shared_ptr<std::vector<int> > features = boost::make_shared<std::vector<int> >();
    ret->modelFeatures = features;

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

      if (!el.has_name()) {
        continue;
      }

      // we are only expecting string property values here except for model
      // features
      if (el.name() != "modelFeatures") {
        if (!el.has_value()) {
          continue;
        }
      }

      vdcapi::PropertyValue val = el.value();

      if (el.name() != "modelFeatures") {
        if (!val.has_v_string()) {
          continue;
        }
      }

      StringConverter st("UTF-8", "UTF-8");
      if (el.name() == "hardwareModelGuid") {
        try {
          ret->hardwareModelGuid = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "vendorGuid") {
        try {
          ret->vendorGuid = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "oemGuid") {
        try {
          ret->oemGuid = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "configURL") {
        try {
          ret->configURL = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "hardwareGuid") {
        try {
          ret->hardwareGuid = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "model") {
        try {
          ret->hardwareInfo = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "modelUID") {
        try {
          ret->modelUID = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "hardwareVersion") {
        try {
          ret->hardwareVersion = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "modelFeatures") {
        for (int j = 0; j < el.elements_size(); j++) {
          vdcapi::PropertyElement feature = el.elements(j);
          if (feature.has_value()) {
            vdcapi::PropertyValue fval = feature.value();
            if (fval.has_v_bool() && fval.v_bool() == true) {
              try {
                ret->modelFeatures->push_back(
                  ModelFeatures::getInstance()->nameToFeature(feature.name()));
              } catch (std::runtime_error &ex) {
                Logger::getInstance()->log("Ignoring feature '" +
                                            feature.name() + "' from device " +
                                            dsuid2str(_device));
              }
            }
          }
        }

      } else if (el.name() == "name") {
        try {
          ret->name = st.convert(val.v_string());
        } catch (DSSException& e) {}
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
        } catch (DSSException& e) {}
      } else if (el.name() == "model" && val.has_v_string()) {
        try {
          ret->model = st.convert(val.v_string());
        } catch (DSSException& e) {}
      } else if (el.name() == "hardwareVersion" && val.has_v_string()) {
        try {
          ret->hardwareVersion = st.convert(val.v_string());
        } catch (DSSException& e) {}
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

}// namespace

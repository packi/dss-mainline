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

#include <iostream>

#include "base.h"
#include "vdc-connection.h"
#include "util.h"
#include "ds485types.h"
#include "dss.h"
#include "model/apartment.h"
#include "ds485/dsbusinterface.h"
#include "messages/vdc-messages.pb.h"

namespace dss {

  boost::shared_ptr<VdcSpec_t> VdcHelper::getSpec(dsuid_t _vdsm, dsuid_t _device) {
    vdcapi::Message message;
    boost::shared_ptr<VdcSpec_t> ret(new VdcSpec_t());

    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty *getprop =
                                    message.mutable_vdsm_request_get_property();
    getprop->set_dsuid(dsuid2str(_device));

    vdcapi::PropertyElement *query = getprop->add_query();
    query->set_name("hardwareGuid");
    query = getprop->add_query();
    query->set_name("modelGuid");
    query = getprop->add_query();
    query->set_name("vendorId");
    query = getprop->add_query();
    query->set_name("hardwareInfo");

    uint8_t buffer_in[4096];
    uint8_t buffer_out[4096];
    uint16_t bs;

    memset(buffer_in, 0, sizeof(buffer_in));
    memset(buffer_out, 0, sizeof(buffer_out));

    if (!message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
      throw std::runtime_error("could not serialize message");
    }

    if (DSS::hasInstance()) {
      DSS::getInstance()->getApartment().getBusInterface()
          ->protobufMessageRequest(_vdsm, message.ByteSize(),
                                   buffer_in, &bs, buffer_out);
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

      // we are only expecting string property values here
      if (!el.has_value()) {
        continue;
      }

      vdcapi::PropertyValue val = el.value();
      if (!val.has_v_string()) {
        continue;
      }

      if (el.name() == "hardwareGuid") {
        ret->hardwareGuid = val.v_string();
      } else if (el.name() == "modelGuid") {
        ret->modelGuid = val.v_string();
      } else if (el.name() == "vendorId") {
        ret->vendorId = val.v_string();
      } else if (el.name() == "hardwareInfo") {
        ret->hardwareInfo = val.v_string();
      }
    }

    return ret;
  }

  void VdcHelper::getIcon(dsuid_t _vdsm, dsuid_t _device, size_t *size, uint8_t **data) {
    vdcapi::Message message;
    boost::shared_ptr<VdcSpec_t> ret(new VdcSpec_t());

    *size = 0;
    *data = NULL;

    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty *getprop =
                                    message.mutable_vdsm_request_get_property();
    getprop->set_dsuid(dsuid2str(_device));

    vdcapi::PropertyElement *query = getprop->add_query();
    query->set_name("deviceIcon16");
    query = getprop->add_query();

    uint8_t buffer_in[4096];
    uint8_t buffer_out[4096];
    uint16_t bs;

    memset(buffer_in, 0, sizeof(buffer_in));
    memset(buffer_out, 0, sizeof(buffer_out));

    if (!message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
      throw std::runtime_error("could not serialize message");
    }

    if (DSS::hasInstance()) {
      DSS::getInstance()->getApartment().getBusInterface()
          ->protobufMessageRequest(_vdsm, message.ByteSize(),
                                   buffer_in, &bs, buffer_out);
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

      if (!el.has_name()) {
        continue;
      }
      // we are only expecting string property values here
      if (!el.has_value()) {
        continue;
      }

      vdcapi::PropertyValue val = el.value();
      if (!val.has_v_bytes()) {
        continue;
      }

      if (el.name() != "deviceIcon16") {
        continue;
      }

      std::string icon = val.v_bytes();
      if (icon.length() > 0) {
        *data = (uint8_t *)malloc(sizeof(uint8_t) * icon.length());
        if (*data == NULL) {
          throw std::runtime_error("could not allocate memory for buffer");
        }
        memcpy(*data, icon.data(), icon.length());
        *size = icon.length();
      }
      break;
    }

    return;
  }

}// namespace
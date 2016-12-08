/*
    Copyright (c) 2011 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "dsbusinterfaceobj.h"

#include "src/ds485/dsbusinterface.h"
#include "src/messages/vdc-messages.pb.h"

namespace dss {

  boost::recursive_mutex DSBusInterfaceObj::m_DSMApiHandleMutex;

  DSBusInterfaceObj::DSBusInterfaceObj() : m_DSMApiHandle(NULL) { }

  void DSBusInterfaceObj::setDSMApiHandle(DsmApiHandle_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    m_DSMApiHandle = _value;
  }

  vdcapi::Message DSBusInterfaceObj::getVdcProperty(const dsuid_t& _dsuid, const dsuid_t& _meterDsuid,
      const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
    vdcapi::vdsm_RequestGetProperty* getPropertyRequest = message.mutable_vdsm_request_get_property();
    getPropertyRequest->set_dsuid(dsuid2str(_dsuid));
    *getPropertyRequest->mutable_query() = query;
    uint8_t arrayOut[REQUEST_LEN];
    if (!message.SerializeToArray(arrayOut, sizeof(arrayOut))) {
      throw std::runtime_error("SerializeToArray failed");
    }
    uint8_t arrayIn[RESPONSE_LEN];
    uint16_t arrayInSize;
    int ret = UserProtobufMessageRequest(m_DSMApiHandle, _meterDsuid,
                                         message.ByteSize(), arrayOut,
                                         &arrayInSize, arrayIn);
    DSBusInterface::checkResultCode(ret);
    if (!message.ParseFromArray(arrayIn, arrayInSize)) {
      throw std::runtime_error("ParseFromArray failed");
    }
    if (message.type() != vdcapi::VDC_RESPONSE_GET_PROPERTY) {
      throw std::runtime_error("Invalid vdc response");
    }
    return message;
  }

  void DSBusInterfaceObj::setVdcProperty(const dsuid_t& _dsuid, const dsuid_t& _meterDsuid,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    vdcapi::Message message;
    message.set_type(vdcapi::VDSM_REQUEST_SET_PROPERTY);
    vdcapi::vdsm_RequestSetProperty* setPropertyRquest = message.mutable_vdsm_request_set_property();
    setPropertyRquest->set_dsuid(dsuid2str(_dsuid));
    *setPropertyRquest->mutable_properties() = properties;
    uint8_t arrayOut[REQUEST_LEN];
    if (!message.SerializeToArray(arrayOut, sizeof(arrayOut))) {
      throw std::runtime_error("SerializeToArray failed");
    }
    uint8_t arrayIn[RESPONSE_LEN];
    uint16_t arrayInSize;
    int ret = UserProtobufMessageRequest(m_DSMApiHandle, _meterDsuid,
                                         message.ByteSize(), arrayOut,
                                         &arrayInSize, arrayIn);
    DSBusInterface::checkResultCode(ret);
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
  }
    
} // namespace dss


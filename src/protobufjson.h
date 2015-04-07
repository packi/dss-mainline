/*
    Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland

    Author: Author: Remy Mahler, remy.mahler@digitalstrom.com

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

#ifndef __DSS_PROTOBUFJSON_H__
#define __DSS_PROTOBUFJSON_H__

#include "config.h"
#include "ds485types.h"
#include <string>
#include <propertyquery.h>
#include "messages/vdc-messages.pb.h"
#include "web/webrequests.h"

namespace dss {

  /// converts a protobuffer vdcapi message of type "property" to a json message
  class ProtobufToJSon
  {
  public:
    static bool protoPropertyToJson(const vdcapi::Message& _message,
                                    JSONWriter& _writer);

  private:
    static void processGenericResponse(const ::vdcapi::GenericResponse& _response,
                                       JSONWriter& _writer);

    static void processElements(const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& _elements,
                                JSONWriter& _writer);

  private:
    // can not be instantiated
    ProtobufToJSon();
    ProtobufToJSon(const ProtobufToJSon& _other); // non construction-copyable
    ProtobufToJSon& operator=(const ProtobufToJSon& _other); // non copyable
  };

  /// converts a property list to a property message
  class PropertyContainerToProtobuf
  {
  public:

    typedef struct ProtoData {
      vdcapi::Message message;
      bool setProperty;
      dsuid_t deviceDsuid;
    } ProtoData;

    static ProtoData convertPropertyContainerToProtobuf(std::vector<PropertyContainer> _parts);

    static vdcapi::Message propertyContainerToProtobuf(const bool _bSetProperty,
                                                       std::vector<PropertyContainer> _parts,
                                                       const dsuid_t& _deviceDsuid);

  private:
    static void assignValue(vdcapi::PropertyElement* _element,
                            std::string& _value);

    static void setupMessage(vdcapi::PropertyElement* _rootProp,
                             std::vector<PropertyContainer> _parts);

    static void startSetupMessage(vdcapi::vdsm_RequestSetProperty* _prop,
                                  std::vector<PropertyContainer> _parts);

    static void startSetupMessage(vdcapi::vdsm_RequestGetProperty* _prop,
                                  std::vector<PropertyContainer> _parts);

  private:
    // can not be instantiated
    PropertyContainerToProtobuf();
    PropertyContainerToProtobuf(const PropertyContainerToProtobuf& _other); // non construction-copyable
    PropertyContainerToProtobuf& operator=(const PropertyContainerToProtobuf& _other); // non copyable
  };

} // namespace

#endif

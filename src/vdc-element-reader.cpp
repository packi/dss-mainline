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

#include "vdc-element-reader.h"

#include "base.h"

namespace dss {

VdcElementReader::VdcElementReader() : m_element(vdcapi::PropertyElement::default_instance()), m_isValid(false) {}
VdcElementReader::VdcElementReader(const vdcapi::PropertyElement& element) : m_element(element), m_isValid(true) {}

std::string VdcElementReader::getValueAsString(const std::string& defaultValue) const {
  if (!m_element.has_value()) {
    return defaultValue;
  }
  const vdcapi::PropertyValue& value = m_element.value();
  if (value.has_v_string()) {
    return value.v_string();
  } else if (value.has_v_bool()) {
    return value.v_bool() ? "true" : "false";
  } else if (value.has_v_double()) {
    return doubleToString(value.v_double());
  } else if (value.has_v_int64()) {
    //double conversion intentional for compatibility with numbers precision in text json
    return doubleToString(double(value.v_int64()));
  } else if (value.has_v_uint64()) {
    //double conversion intentional for compatibility with numbers precision in text json
    return doubleToString(double(value.v_uint64()));
  }
  return defaultValue;
}

double VdcElementReader::getValueAsDouble(double defaultValue) const {
  if (!m_element.has_value()) {
    return defaultValue;
  }
  const vdcapi::PropertyValue& value = m_element.value();
  if (value.has_v_double()) {
    return value.v_double();
  } else if (value.has_v_int64()) {
    return double(value.v_int64());
  } else if (value.has_v_uint64()) {
    return double(value.v_uint64());
  } else if (value.has_v_string()) {
    return strToDouble(value.v_string());
  }
  return defaultValue;
}

int VdcElementReader::getValueAsInt(int defaultValue) const {
  if (!m_element.has_value()) {
    return defaultValue;
  }
  const vdcapi::PropertyValue& value = m_element.value();
  if (value.has_v_double()) {
    return int(value.v_double());
  } else if (value.has_v_int64()) {
    return int(value.v_int64());
  } else if (value.has_v_uint64()) {
    return int(value.v_uint64());
  } else if (value.has_v_string()) {
    return strToInt(value.v_string());
  }
  return defaultValue;
}

int VdcElementReader::getValueAsBool(bool defaultValue) const {
  if (!m_element.has_value()) {
    return defaultValue;
  }
  const vdcapi::PropertyValue& value = m_element.value();
  if (value.has_v_double()) {
    return value.v_double() != 0;
  } else if (value.has_v_int64()) {
    return value.v_int64() != 0;
  } else if (value.has_v_uint64()) {
    return value.v_uint64() != 0;
  } else if (value.has_v_string()) {
    return !value.v_string().empty();
  }
  return defaultValue;
}

VdcElementReader VdcElementReader::operator [](const char* key) const {
  const google::protobuf::RepeatedPtrField<vdcapi::PropertyElement>& childElements = m_element.elements();
  for (google::protobuf::RepeatedPtrField<vdcapi::PropertyElement>::const_iterator it = childElements.begin(); it != childElements.end(); it++) {
    const vdcapi::PropertyElement& childElement = *it;
    if (childElement.name() == key) {
      return VdcElementReader(childElement);
    }
  }
  return VdcElementReader();
}

VdcElement::VdcElement(const vdcapi::PropertyElement& element) { m_element.CopyFrom(element); }
VdcElement::VdcElement(const google::protobuf::RepeatedPtrField<vdcapi::PropertyElement>& elements) {
  m_element.mutable_elements()->CopyFrom(elements);
}

}// namespace

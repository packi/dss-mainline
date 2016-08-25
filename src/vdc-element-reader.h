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

#ifndef __DSS_VDC_ELEMENT_READER_H__
#define __DSS_VDC_ELEMENT_READER_H__

#include <src/messages/vdcapi.pb.h>


namespace dss {

  // Helper class for reading vdcapi::PropertyElement.
  class VdcElementReader {
  public:
    VdcElementReader();
    VdcElementReader(const vdcapi::PropertyElement& element);

    bool isValid() { return m_isValid; }
    const std::string& getName() { return m_element.name(); }
    std::string getValueAsString(const std::string& defaultValue = std::string()) const;
    double getValueAsDouble(double defaultValue = 0) const;
    int getValueAsInt(int defaultValue = 0) const;
    int getValueAsBool(bool defaultValue = false) const;

    VdcElementReader operator [](const char* key) const;

    class iterator {
    public:
      iterator(google::protobuf::RepeatedPtrField<vdcapi::PropertyElement>::const_iterator it) : m_it(it) {}
      VdcElementReader operator*() { return VdcElementReader(*m_it); }
      iterator& operator++() { m_it++; return *this; }
      iterator operator++(int) { return iterator(m_it++); }
      bool operator==(const iterator& other) { return m_it == other.m_it; }
      bool operator!=(const iterator& other) { return m_it != other.m_it; }

    private:
      google::protobuf::RepeatedPtrField<vdcapi::PropertyElement>::const_iterator m_it;
    };

    iterator begin() const { return m_element.elements().begin(); }
    iterator end() const { return m_element.elements().end(); }

  private:
    const ::vdcapi::PropertyElement& m_element;
    bool m_isValid;
  };

  // Creates PropertyElement from list of elements and owns it.
  class VdcElement {
  public:
    VdcElement(const vdcapi::PropertyElement& element);
    VdcElement(const google::protobuf::RepeatedPtrField<vdcapi::PropertyElement>& elements);
    VdcElementReader getReader() { return VdcElementReader(m_element); }
  private:
    vdcapi::PropertyElement m_element;
  };

} // namespace dss

#endif

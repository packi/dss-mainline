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

#include "json.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "src/foreach.h"
#include "src/base.h"

namespace dss {


  std::string jsonEncode(const std::string& _value);

  std::ostream& operator<<(std::ostream& stream, const JSONObject &obj) {
    obj.writeTo(stream);
    return stream;
  }

  //================================================== JSONElement

  void JSONElement::addElement(const std::string& _name, boost::shared_ptr<JSONElement> _element) {
    m_Elements.push_back(std::make_pair(_name, _element));
  } // addElement

  std::string JSONElement::toString() {
    std::stringstream sstream;
    writeTo(sstream);
    return sstream.str();
  }

  void JSONElement::writeElementsTo(std::ostream& _out) const {
    bool first = true;
    foreach(const NamedElement& namedElement, m_Elements) {
      if(!first) {
        _out << ',';
      }
      first = false;
      _out << '"' << jsonEncode(namedElement.first) << '"' << ':';
      namedElement.second->writeTo(_out);
    }
  }

  void JSONElement::writeElementsNoNamesTo(std::ostream& _out) const {
    bool first = true;
    foreach(const NamedElement& namedElement, m_Elements) {
      if(!first) {
        _out << ',';
      }
      first = false;
      namedElement.second->writeTo(_out);
    }
  }

  boost::shared_ptr<JSONElement> JSONElement::getElementByName(const std::string& _name) {
    foreach(NamedElement& namedElement, m_Elements) {
      if(namedElement.first == _name) {
        return namedElement.second;
      }
    }
    return boost::shared_ptr<JSONElement>();
  } // getElementByName

  boost::shared_ptr<JSONElement> JSONElement::getElement(const int _index) {
    return m_Elements.at(_index).second;
  } // getElement

  const std::string& JSONElement::getElementName(const int _index) {
    return m_Elements.at(_index).first;
  } // getElementName

  int JSONElement::getElementCount() const {
    return m_Elements.size();
  } // getElementCount


  //================================================== JSONObject

  void JSONObject::addProperty(const std::string& _name, const std::string& _value) {
    boost::shared_ptr<JSONValue<std::string> > elem(new JSONValue<std::string>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::addProperty(const std::string& _name, const char* _value) {
    boost::shared_ptr<JSONValue<std::string> > elem(new JSONValue<std::string>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::addProperty(const std::string& _name, const int _value) {
    boost::shared_ptr<JSONValue<int> > elem(new JSONValue<int>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::addProperty(const std::string& _name, const unsigned long int _value) {
    boost::shared_ptr<JSONValue<unsigned long int> > elem(new JSONValue<unsigned long int>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::addProperty(const std::string& _name, const unsigned long long _value) {
    boost::shared_ptr<JSONValue<unsigned long long> > elem(new JSONValue<unsigned long long>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::addProperty(const std::string& _name, const bool _value) {
    boost::shared_ptr<JSONValue<bool> > elem(new JSONValue<bool>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::addProperty(const std::string& _name, const double _value) {
    boost::shared_ptr<JSONValue<double> > elem(new JSONValue<double>(_value));
    addElement(_name, elem);
  } // addProperty

  void JSONObject::writeTo(std::ostream& _out) const {
    _out << "{";
    writeElementsTo(_out);
    _out << "}";
  }


  //================================================== JSONValue

  template<>
  void JSONValue<int>::writeTo(std::ostream& _out) const {
    _out << m_Value;
  }

  template<>
  void JSONValue<unsigned long int>::writeTo(std::ostream& _out) const {
    _out << m_Value;
  }

  template<>
  void JSONValue<unsigned long long>::writeTo(std::ostream& _out) const {
    _out << m_Value;
  }

  template<>
  void JSONValue<std::string>::writeTo(std::ostream& _out) const {
    _out << '"' << jsonEncode(m_Value) << '"';
  }

  template<>
  void JSONValue<bool>::writeTo(std::ostream& _out) const {
    if(m_Value) {
      _out << "true";
    } else {
      _out << "false";
    }
  }

  template<>
  void JSONValue<double>::writeTo(std::ostream& _out) const {
    std::streamsize ss = _out.precision();
    _out << std::setprecision(16) << m_Value;
    _out << std::setprecision(ss);
  }

  //================================================== JSONArrayBase

  void JSONArrayBase::writeTo(std::ostream& _out) const {
    _out << "[";
    writeElementsNoNamesTo(_out);
    _out << "]";
  }


  //================================================== Helpers

  inline int findFirstNeedingEscape(const std::string& _value) {
    int iChar = 0;
    int end = _value.size();
    for(; iChar < end; iChar++) {
      char c = _value[iChar];
      if ((c == '\\') || (c == '"')) {
        break;
      } else if (c == '\b') {
        break;
      } else if (c == '\f') {
        break;
      } else if (c == '\n') {
        break;
      } else if (c == '\r') {
        break;
      } else if (c == '\t') {
        break;
      } else {
        if(c < ' ') {
          break;
        }
      }
    }
    if(iChar == end) {
      return -1;
    } else {
      return iChar;
    }
  }

  std::string jsonEncode(const std::string& _value) {
    int firstToEscape = findFirstNeedingEscape(_value);
    if(firstToEscape == -1) {
      return _value;
    } else {
      std::ostringstream sstream;
      sstream << _value.substr(0, firstToEscape);
      for(int iChar = firstToEscape, end = _value.size(); iChar < end; iChar++) {
        unsigned char c = _value[iChar];
        if ((c == '\\') || (c == '"')) {
          sstream << '\\' << c;
        } else if (c == '\b') {
          sstream << "\\b";
        } else if (c == '\f') {
          sstream << "\\f";
        } else if (c == '\n') {
          sstream << "\\n";
        } else if (c == '\r') {
          sstream << "\\r";
        } else if (c == '\t') {
          sstream << "\\t";
        } else {
          if(c < ' ') {
            sstream << "\\u";
            sstream.fill('0');
            sstream.width(4);
            sstream << std::hex << std::uppercase;
            sstream << static_cast<int>(c);
            sstream << std::nouppercase;
            sstream.width(1);
          } else {
            sstream << c;
          }
        }
      }
      return sstream.str();
    }
  } // jsonEncode

} // namespace dss

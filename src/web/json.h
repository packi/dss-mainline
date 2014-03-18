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

#ifndef JSON_H_
#define JSON_H_

#include <string>
#include <vector>
#include <iosfwd>

#include <boost/shared_ptr.hpp>


namespace dss {

  class JSONElement {
  public:
    void addElement(const std::string& _name, boost::shared_ptr<JSONElement> _object);
    boost::shared_ptr<JSONElement> getElementByName(const std::string& _name);
    boost::shared_ptr<JSONElement> getElement(const int _index);
    const std::string& getElementName(const int _index);
    int getElementCount() const;
    virtual ~JSONElement() {}
    std::string toString();
  protected:
    virtual void writeTo(std::ostream& _out) = 0;
    virtual void writeElementsTo(std::ostream& _out);
    /** same as writeElements but skip empty names field */
    virtual void writeElementsNoNamesTo(std::ostream& _out);
  private:
    typedef std::pair<std::string, boost::shared_ptr<JSONElement> > NamedElement;
    std::vector<NamedElement> m_Elements;
  };

  class JSONObject : public JSONElement {
  public:
    virtual void addProperty(const std::string& _name, const char* _value);
    virtual void addProperty(const std::string& _name, const std::string& _value);
    virtual void addProperty(const std::string& _name, const int _value);
    virtual void addProperty(const std::string& _name, const unsigned long int _value);
    virtual void addProperty(const std::string& _name, const unsigned long long _value);
    virtual void addProperty(const std::string& _name, const bool _value);
    virtual void addProperty(const std::string& _name, const double _value);
  protected:
    virtual void writeTo(std::ostream& _out);
  };

  template<class T>
  class JSONValue : public JSONElement {
  public:
    JSONValue(T _value) : m_Value(_value) {}

    const T& getValue() const {
      return m_Value;
    }
  protected:
    virtual void writeTo(std::ostream& _out);
  private:
    T m_Value;
  };

  class JSONArrayBase : public JSONElement {
  protected:
    virtual void writeTo(std::ostream& _out);
  }; // JSONArrayBase

  template<class T>
  class JSONArray : public JSONArrayBase {
  public:
    void add(T _value) {
      boost::shared_ptr<JSONValue<T> > elem(new JSONValue<T>(_value));
      addElement("", elem);
    }
  };

} // namespace dss

#endif /* JSON_H_ */

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


#include <iostream>

#include <boost/pointer_cast.hpp>
#include <boost/assert.hpp>

#include "core/web/json.h"

namespace dss {

  class WebFixture {
  public:

    void testOkIs(boost::shared_ptr<JSONObject> _response, bool _value) {
      boost::shared_ptr<JSONElement> okElem = _response->getElementByName("ok");
      if(okElem == NULL) {
        BOOST_ASSERT(false);
        return;
      }
      JSONValue<bool>* val = dynamic_cast<JSONValue<bool>*>(okElem.get());
      if(val == NULL) {
        BOOST_ASSERT(false);
        return;
      }
      BOOST_ASSERT(val->getValue() == _value);
    }

    boost::shared_ptr<JSONObject> getSubObject(const std::string& _name, boost::shared_ptr<JSONElement> _elem) {
      boost::shared_ptr<JSONElement> resultElem = _elem->getElementByName(_name);
      if(resultElem == NULL) {
        BOOST_ASSERT(false);
      }
      boost::shared_ptr<JSONObject> result = boost::dynamic_pointer_cast<JSONObject>(resultElem);
      BOOST_ASSERT(result != NULL);
      return result;
    }

    boost::shared_ptr<JSONObject> getResultObject(boost::shared_ptr<JSONObject> _response) {
      testOkIs(_response, true);
      boost::shared_ptr<JSONObject> result = getSubObject("result", _response);
      BOOST_ASSERT(result != NULL);
      return result;
    }

    template<class t>
    void checkPropertyEquals(const std::string& _name, const t& _expected, boost::shared_ptr<JSONObject> _obj) {
      boost::shared_ptr<JSONElement> elem = _obj->getElementByName(_name);
      if(elem == NULL) {
        std::cout << "No element found " << _name << std::endl;
        BOOST_CHECK(false);
        return;
      }
      JSONValue<t>* val = dynamic_cast<JSONValue<t>*>(elem.get());
      if(val == NULL) {
        std::cout << "Not a value" << std::endl;
        BOOST_CHECK(false);
        return;
      }
      BOOST_CHECK_EQUAL(val->getValue(), _expected);
    }
  }; // WebFixture

}


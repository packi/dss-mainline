/*
 *  Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <cstdio>

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include "foreach.h"

#include "src/model/state.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(states)

BOOST_AUTO_TEST_CASE(test_setState)
{
  boost::shared_ptr<State> role = boost::make_shared<State>("role");

  State::ValueRange_t values;
  values.push_back("good");
  values.push_back("bad");
  values.push_back("ugly");
  role->setValueRange(values);

  foreach (std::string cur, values) {
    role->setState(coTest, cur);
    BOOST_CHECK_EQUAL(role->toString(), cur);
  }
}

BOOST_AUTO_TEST_SUITE_END()

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

#include <boost/test/unit_test.hpp>

#include <digitalSTROM/dsuid.h>

BOOST_AUTO_TEST_SUITE(dsuid)

BOOST_AUTO_TEST_CASE(test_string_basic)
{
  DSUID_STR(dsuid_str);
  dsuid_t dsuid1, dsuid2;

  BOOST_CHECK_EQUAL(dsuid_from_string("0102030405060708090a0b0c0d0e0f1011",
                                      &dsuid1), 0);

  dsuid2 = dsuid1;
  BOOST_CHECK(dsuid_equal(&dsuid1, &dsuid2));

  dsuid1.id[2] = 0xff;
  BOOST_CHECK(!dsuid_equal(&dsuid1, &dsuid2));

  BOOST_CHECK_EQUAL(dsuid_to_string(&dsuid1, dsuid_str), 0);
  BOOST_CHECK_EQUAL(dsuid_str, "0102ff0405060708090a0b0c0d0e0f1011");
  BOOST_CHECK_EQUAL(dsuid_to_string(&dsuid2, dsuid_str), 0);
  BOOST_CHECK_EQUAL(dsuid_str, "0102030405060708090a0b0c0d0e0f1011");
}

BOOST_AUTO_TEST_SUITE_END()

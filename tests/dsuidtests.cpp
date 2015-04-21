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
#include "src/ds485types.h"

using namespace dss;

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

BOOST_AUTO_TEST_CASE(test_copy_dsuid)
{
  dsuid_t dsuid;

  dsuid = DSUID_NULL;
  BOOST_CHECK(dsuid_is_null(&dsuid));
  dsuid = DSUID_BROADCAST;
  BOOST_CHECK(dsuid_is_broadcast(&dsuid));
}

BOOST_AUTO_TEST_CASE(test_operator_equal)
{
  dsuid_t dsuid1(DSUID_NULL), dsuid2(DSUID_NULL);

  BOOST_CHECK(dsuid1 == DSUID_NULL);
  BOOST_CHECK(dsuid1 != DSUID_BROADCAST);
  BOOST_CHECK(dsuid1 == dsuid2);
}

class ClassWithDSID {
public:
  ClassWithDSID() : m_dsuid(DSUID_NULL) {}
  dsuid_t getDSID() { return m_dsuid; }
  void setDSID(const dsuid_t& id) { m_dsuid = id; }
private:
  dsuid_t m_dsuid;
};

BOOST_AUTO_TEST_CASE(test_operator_equal_temporary)
{
  ClassWithDSID klass1, klass2;

  /*
   * gcc error:  error: taking address of temporary [-fpermissive]
   * dsuid_equal(&klass1.getDSID(), &klass2.getDSID()));
   */

  BOOST_CHECK(klass1.getDSID() == klass2.getDSID());

  klass1.setDSID(DSUID_BROADCAST);
  BOOST_CHECK(klass1.getDSID() != klass2.getDSID());
}

BOOST_AUTO_TEST_SUITE_END()

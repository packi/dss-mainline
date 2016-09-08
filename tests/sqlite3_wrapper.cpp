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

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "config.h"
#include "src/base.h"
#include "src/sqlite3_wrapper.h"

#include "tests/util/dss_instance_fixture.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(SQL_WRAPPER)

BOOST_FIXTURE_TEST_CASE(testSimple, DSSInstanceFixture) {
  // db will be erased with each test run
  SQLite3 db(DSS::getInstance()->getDatabaseDirectory() + "/sqlite_wrapper.db", true);
  std::string stmts = readFile(TEST_STATIC_DATADIR + "/db-fetch-ok.sql");

  db.exec(stmts);
  SQLite3::query_result res = db.prepare("SELECT name FROM foo where id = 7").fetchAll();
  BOOST_CHECK_EQUAL(res.size(), 1);
  BOOST_CHECK_EQUAL(res[0][0].data, "bar");
}

BOOST_AUTO_TEST_SUITE_END()

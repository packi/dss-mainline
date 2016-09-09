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

static std::string sql_dump_ok = R"sql(
begin transaction;
drop table if exists "foo";
create table "foo"(id primary key,name);
insert into foo values(3 , "bar3");
insert into foo values(5 , "bar5");
insert into foo values(7 , "bar7");
insert into foo values(11 , "bar11");
commit;
)sql";

BOOST_FIXTURE_TEST_CASE(testSimple, DSSInstanceFixture) {
  // db will be erased with each test run
  SQLite3 db(DSS::getInstance()->getDatabaseDirectory() + "/sqlite_wrapper.db", true);
  db.exec(sql_dump_ok);

  SQLite3::query_result res = db.prepare("SELECT name FROM foo WHERE id=7").fetchAll();
  BOOST_CHECK_EQUAL(res.size(), 1);
  BOOST_CHECK_EQUAL(res[0][0].data, "bar7");
}

BOOST_AUTO_TEST_SUITE_END()

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
  SQLite3 db(DSS::getInstance()->getDatabaseDirectory() + "/sqlite_wrapper.db", SQLite3::Mode::ReadWrite);
  db.exec(sql_dump_ok);

  SQLite3::query_result res = db.prepare("SELECT name FROM foo WHERE id=7").fetchAll();
  BOOST_CHECK_EQUAL(res.size(), 1);
  BOOST_CHECK_EQUAL(res[0][0].data, "bar7");
}

BOOST_FIXTURE_TEST_CASE(testBindArgs, DSSInstanceFixture) {
  // db will be erased with each test run
  SQLite3 db(DSS::getInstance()->getDatabaseDirectory() + "/sqlite_wrapper.db", SQLite3::Mode::ReadWrite);
  db.exec(sql_dump_ok);

  auto find_by_key = db.prepare("SELECT name FROM foo WHERE id=?");
  {
    auto bindScope = find_by_key.bind(7);
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::ROW);
    BOOST_CHECK_EQUAL(find_by_key.getColumn<std::string>(0), "bar7");
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
    find_by_key.reset(); //< implicit call after 3.6.23.1,

    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::ROW);
    BOOST_CHECK_EQUAL(find_by_key.getColumn<std::string>(0), "bar7");
    find_by_key.reset(); //< implicit call after 3.6.23.1,
  }

  {
    // query is reusable
    auto bindScope = find_by_key.bind(11);
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::ROW);
    BOOST_CHECK_EQUAL(find_by_key.getColumn<std::string>(0), "bar11");
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
    find_by_key.reset();
  }

  {
    // no results without binding args
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
    find_by_key.reset();
  }

  {
    // if bindScope isn't held, behavior is same as without binding
    /* auto bindScope = */ find_by_key.bind(11);
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
    find_by_key.reset();
  }

  {
    // reset bindScope by creating new binding
    auto bindScope = find_by_key.bind(11);
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::ROW);
    BOOST_CHECK_EQUAL(find_by_key.getColumn<std::string>(0), "bar11");
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
    find_by_key.reset();

#if 0
    // TODO(soon) doesn't work
    bindScope = find_by_key.bind(3);
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
    BOOST_CHECK_EQUAL(find_by_key.getColumn<std::string>(0), "bar3");
    BOOST_CHECK(find_by_key.step() == SqlStatement::StepResult::DONE);
#endif
  }

  auto find_by_name = db.prepare("SELECT id FROM foo WHERE name=?");
  // same with string as argument, int as reply
  {
    auto bindScope = find_by_name.bind("bar7");
    BOOST_CHECK(find_by_name.step() == SqlStatement::StepResult::ROW);
    BOOST_CHECK_EQUAL(find_by_name.getColumn<int>(0), 7);
    BOOST_CHECK(find_by_name.step() == SqlStatement::StepResult::DONE);
    find_by_name.reset();
  }

  auto find_by_id_and_name = db.prepare("SELECT id FROM foo WHERE id=? AND name=?");
  // same with two args
  {
    auto bindScope = find_by_id_and_name.bind(7, "bar7");
    BOOST_CHECK(find_by_id_and_name.step() == SqlStatement::StepResult::ROW);
    BOOST_CHECK_EQUAL(find_by_id_and_name.getColumn<int>(0), 7);
    BOOST_CHECK(find_by_id_and_name.step() == SqlStatement::StepResult::DONE);
    find_by_id_and_name.reset();
  }
}

static std::string sql_update = R"sql(
begin transaction;
drop table if exists "foo";
create table "foo"(id primary key,name);
insert into foo values(31 , "bar3");
insert into foo values(37 , "bar5");
insert into foo values(41 , "bar7");
insert into foo values(47 , "bar11");
commit;
)sql";

void createDb(const std::string &filename, const std::string sql_dump) {
  SQLite3 db(filename, SQLite3::Mode::ReadWrite);
  db.exec(sql_dump);
}

BOOST_FIXTURE_TEST_CASE(testConcurrentModification, DSSInstanceFixture) {
  std::string filename =
    DSS::getInstance()->getDatabaseDirectory() + "/sqlite_wrapper.db";

  createDb(filename, sql_dump_ok);

  SQLite3 db2(filename, SQLite3::Mode::ReadWrite);
  // use separate db handle, no common mutex

  {
    // update db, possible before step is called
    SQLite3 db1(filename, SQLite3::Mode::ReadOnly);
    SqlStatement find = db1.prepare("SELECT id FROM foo WHERE id>7 ORDER BY id");
    BOOST_CHECK_NO_THROW(db2.exec("DROP table foo;"));
    BOOST_CHECK_THROW(find.step(), std::exception);
    // SQL ERROR: SQL logic error or missing database
  }

  // restore db
  createDb(filename, sql_dump_ok);

  {
    // update db prevent after step is called
    SQLite3 db1(filename, SQLite3::Mode::ReadOnly);
    SqlStatement find = db1.prepare("SELECT id FROM foo WHERE id>7 ORDER BY id");
    BOOST_CHECK(find.step() == SqlStatement::StepResult::ROW);
    // no modifications after stop executed

    BOOST_CHECK_THROW(db2.exec(sql_update), std::exception);
    // no modification possible, if step executed
    db2.exec("rollback;");

    BOOST_CHECK_EQUAL(find.getColumn<int>(0), 11);
    BOOST_CHECK(find.step() == SqlStatement::StepResult::DONE);
    BOOST_CHECK_NO_THROW(db2.exec(sql_update));
    // works after step been called
  }
}

BOOST_AUTO_TEST_SUITE_END()

/*
  Copyright (c) 2011, 2015 aizo ag, Zurich, Switzerland

  Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

  This file is part of digitalSTROM Server (dSS), originally imported from
  dS Assistant.

  dSS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  dSS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "sqlite3_wrapper.h"
#include "src/logger.h"

namespace dss {

void SQLite3::Deleter::operator()(::sqlite3* ptr) {
  sqlite3_close_v2(ptr);
}

SQLite3::SQLite3(std::string db_file, Mode mode)
{
  boost::mutex::scoped_lock lock(m_mutex);

  if (!sqlite3_threadsafe()) {
    throw std::runtime_error("Need a threadsafe SQLite version, aborting");
  }

  if (access(db_file.c_str(), R_OK | W_OK) != 0 && errno != ENOENT) {
    throw std::runtime_error(
        "Error while accessing sqlite database file " +
        db_file + ": " + strerror(errno));
  }

  int flags = SQLITE_OPEN_FULLMUTEX;
  if (mode == Mode::ReadWrite) {
    flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  } else {
    flags |= SQLITE_OPEN_READONLY;
  }

  sqlite3* ptr = NULL;
  int ret = sqlite3_open_v2(db_file.c_str(), &ptr, flags, NULL);
  m_ptr.reset(ptr); // call sqlite3_close_v2 in every case
  if (ret != SQLITE_OK) {
    throw std::runtime_error("Could not open database " + db_file + ": " +
             sqlite3_errmsg(*this));
  }
}

void SqlStatement::Deleter::operator()(::sqlite3_stmt* ptr) {
  sqlite3_finalize(ptr);
}

SqlStatement::SqlStatement(SQLite3& db, const std::string &sql) : m_db(db) {
  boost::mutex::scoped_lock lock(m_db.m_mutex);
  sqlite3_stmt* ptr = NULL;
  const char *rem = NULL;
  int ret = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &ptr, &rem);
  m_ptr.reset(ptr); // call sqlite3_finalize in every case
  if (ret != SQLITE_OK) {
    throw std::runtime_error(std::string(__func__) + ": " + sqlite3_errstr(ret));
  }

  if (rem && *rem != '\0') {
    Logger::getInstance()->log(std::string(__func__) + ": sql injection? :'" + std::string(rem) + "'", lsWarning);
  }
}

void SqlStatement::reset() {
  int ret = sqlite3_reset(*this);
  if (ret != SQLITE_OK) {
    throw std::runtime_error(std::string(__func__) + ": " + sqlite3_errstr(ret));
  }
}

void SqlStatement::BindDeleter::operator()(::sqlite3_stmt* ptr) {
  int ret = sqlite3_clear_bindings(ptr);
  if (ret != SQLITE_OK) {
    // Ignore errors. It is bad idea to throw exceptions in destructor.
    Logger::getInstance()->log(std::string(__func__) + ": " + sqlite3_errstr(ret), lsWarning);
  }
}

SqlStatement::StepResult SqlStatement::step() {
  int ret = sqlite3_step(*this);
  if (ret == SQLITE_DONE) {
    return StepResult::DONE;
  }
  if (ret != SQLITE_ROW) {
    throw std::runtime_error(std::string(__func__) + ": " + sqlite3_errstr(ret));
  }
  return StepResult::ROW;
}

void SqlStatement::bindAt(int index, int value) {
  int ret = sqlite3_bind_int(*this, index, value);
  if (ret != SQLITE_OK) {
    throw std::runtime_error(std::string(__func__) + ": " + sqlite3_errstr(ret));
  }
}

void SqlStatement::bindAt(int index, const std::string &value) {
  int ret = sqlite3_bind_text(*this, index, value.c_str(), value.size(), SQLITE_STATIC);
  if (ret != SQLITE_OK) {
    throw std::runtime_error(std::string(__func__) + ": " + sqlite3_errstr(ret));
  }
}

template <>
std::string SqlStatement::getColumn<std::string>(int i) {
  return std::string(reinterpret_cast<const char *>(sqlite3_column_text(*this, i)));
}

template <>
int SqlStatement::getColumn<int>(int i) {
  return sqlite3_column_int(*this, i);
}

SQLite3::query_result SqlStatement::fetchAll()
{
  SQLite3::query_result results;
  int ret;

  boost::mutex::scoped_lock lock(m_db.m_mutex);
  int columns = sqlite3_column_count(*this);
  do {
    ret = sqlite3_step(*this);
    if (ret == SQLITE_ROW) {
      results.push_back(SQLite3::row_result());
      SQLite3::row_result &row(results.back());

      for (int i = 0; i < columns; i++) {
        int type = sqlite3_column_type(*this, i);
        const unsigned char *text = sqlite3_column_text(*this, i);
        std::string name = sqlite3_column_name(*this, i);
        std::string data = text ? (const char *)text : "";

        row.push_back(SQLite3::cell());
        SQLite3::cell &cell(row.back());

        cell.name = name;
        cell.data = data;
        cell.type = type;
      }
    }
  } while (ret == SQLITE_ROW);

  if (ret != SQLITE_DONE) {
    Logger::getInstance()->log("sqlite3_wrapper: sqlite3_step ret:" + std::string(sqlite3_errstr(ret)), lsWarning);
  }

  return results;
}

// CAUTION: this function does not lock the mutex! it can be used as a helper
// for API functions that need to perform more than one db operation
// sequentially
void SQLite3::execInternal(std::string sql)
{
  char *errmsg = NULL;
  int ret;

  ret = sqlite3_exec(*this, sql.c_str(), NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    std::string msg = errmsg;
    sqlite3_free(errmsg);
    throw std::runtime_error(msg);
  }
}

void SQLite3::exec(std::string sql)
{
  boost::mutex::scoped_lock lock(m_mutex);
  execInternal(sql);
}

std::string SQLite3::escape(std::string str, bool quotes)
{
  char *q;
  if (quotes) {
    q = sqlite3_mprintf("'%Q'", (str.empty() ? "" : str.c_str()));
  } else {
    q = sqlite3_mprintf("'%q'", (str.empty() ? "" : str.c_str()));
  }
  std::string ret = q;
  sqlite3_free(q);
  return ret;
}

} // namespace

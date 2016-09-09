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

#ifndef __SQLITE3_WRAPPER_H__
#define __SQLITE3_WRAPPER_H__

#include <sqlite3.h>
#include <string>
#include <map>
#include <memory>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace dss
{

class SqlStatement;

class SQLite3 {

public:
  /// \brief Initialize database using the specified location.
  ///
  /// If the database does not exist, it will be automatically created.
  SQLite3(std::string db_file, bool readwrite = false);

  /// \brief represents a "column cell" in the table, first pair element is
  /// the column name, second pair element is the actual value
  typedef struct {
      std::string name;
      std::string data;
      int type;
  } cell;

  /// \brief represents a row in the database table
  typedef std::vector<cell> row_result;

  /// \brief represents a result of the query
  typedef std::vector<row_result> query_result;

  /// \brief Send a query to the database
  /// \param q valid SQL query like:
  ///     "SELECT \"value\" FROM \"dsa_internal\" where \"key\" = \"version\";
  SqlStatement prepare(const std::string &sql);

  /// \brief Execute SQL on the active database, no response expected.
  ///
  /// Will throw an exception if anything goes wrong
  void exec(std::string sql);

  /// \brief Helper function for proper escaping of strings.
  ///
  /// All names or user input should be run through this function.
  ///
  /// \param str string that should be escaped
  /// \param quotes adds quotes around the string (convenience parameter)
  std::string escape(std::string str, bool quotes = false);

  operator ::sqlite3*() { return m_ptr.get(); }
  ///< Default cast to raw sqlite3*.
  ///< Allows to use this class in sqlite3 api not wrapped here.
private:
  struct Deleter {
    void operator()(::sqlite3*);
  };
  std::unique_ptr<sqlite3, Deleter> m_ptr;

  boost::mutex m_mutex;

  void execInternal(std::string sql);

  friend class SqlStatement;
};

class SqlStatement {
public:
  SqlStatement(SQLite3& db, const std::string &sql);
  SqlStatement(const SqlStatement &that);

  void reset();
  ///< Calls sqlite3_reset

  struct BindDeleter {
    void operator()(::sqlite3_stmt*);
  };
  typedef std::unique_ptr<sqlite3_stmt, BindDeleter> BindScope;
  ///< Calls sqlite3_clear_bindings after leaving your current scope

  template <typename... Args>
  BindScope bind(Args&&... args) __attribute__((warn_unused_result));
  // TODO(soon) warn_unused_result ignore
  ///< Binds arguments to statement.
  ///< @return BindScope unbinding arguments in its descructor.
  ///< All binded strings and blobs must be valid till BindScope is destroyed.
  ///< BindScope must be destroyed before another call to bind().

  enum class StepResult { ROW, DONE };
  StepResult step();
  ///< Calls sqlite3_step and returns StepResults. All other return values throw exception.

  SQLite3::query_result fetchAll();
  ///< using step functions involves less copying

  template <typename T>
  T getColumn(int i);

  operator sqlite3_stmt*() { return m_ptr.get(); }
  ///< Default cast to raw sqlite3_stmt*.
  ///< Allows to use this class in sqlite3 api not wrapped here

private:
  struct Deleter {
    void operator()(::sqlite3_stmt*);
  };
  std::unique_ptr<sqlite3_stmt, Deleter> m_ptr;

  void bindRecursive(int column) {}

  template <typename T, typename... Args>
  void bindRecursive(int index, T&& value, Args&&... args) {
    bindAt(index, std::forward<T>(value));
    bindRecursive(index + 1, std::forward<Args>(args)...);
  }

  // Explicit bind overrided for types we care about.
  void bindAt(int index, int value);
  void bindAt(int index, const std::string &value);

  SQLite3 &m_db; // needed for m_lock
};

inline SqlStatement SQLite3::prepare(const std::string &sql) {
  return SqlStatement(*this, sql);
}

template <typename... Args>
SqlStatement::BindScope SqlStatement::bind(Args&&... args) {
  BindScope bindScope(*this);
  bindRecursive(1, std::forward<Args>(args)...);
  return std::move(bindScope);
}

} // namespace

#endif//__SQLITE3_WRAPPER_H__

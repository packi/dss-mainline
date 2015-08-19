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
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace dss
{

class SQLite3
{

public:
    /// \brief Initialize database using the specified location.
    ///
    /// If the database does not exist, it will be automatically created.
    SQLite3(std::string db_file);
    ~SQLite3();

    /// \brief represents a "column cell" in the table, first pair element is
    /// the column name, second pair element is the actual value
    typedef struct
    {
        std::string name;
        std::string data;
        int type;
    } cell;

    /// \brief represents a row in the database table
    typedef std::vector<boost::shared_ptr<cell> > row_result;

    /// \brief represents a result of the query
    typedef std::vector<boost::shared_ptr<row_result> > query_result;

    /// \brief Send a query to the database
    /// \param q valid SQL query like:
    ///     "SELECT \"value\" FROM \"dsa_internal\" where \"key\" = \"version\";
    boost::shared_ptr<query_result> query(std::string q);

    /// \brief Execute SQL on the active database, no response expected.
    ///
    /// Will throw an exception if anything goes wrong
    void exec(std::string sql);

    /// \brief Execute SQL on the active database, no response expected.
    ///
    /// Will throw an exception if anything goes wrong
    long long int execAndGetRowId(std::string sql);

    /// \brief Helper function for proper escaping of strings.
    ///
    /// All names or user input should be run through this function.
    ///
    /// \param str string that should be escaped
    /// \param quotes adds quotes around the string (convenience parameter)
    std::string escape(std::string str, bool quotes = false);

    /// \brief Returns the Id of the last inserted row
    long long int getLastInsertedRowId();

    static bool isFatal(int error);
private:
    sqlite3 *m_db;
    boost::mutex m_mutex;

    static int execCallback(void *arg, int columns, char **data, char **name);
    void execInternal(std::string sql);
};

} // namespace

#endif//__SQLITE3_WRAPPER_H__

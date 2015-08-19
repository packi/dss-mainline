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

namespace dss
{

SQLite3::SQLite3(std::string db_file, bool readonly)
{
    boost::mutex::scoped_lock lock(m_mutex);

    if (!sqlite3_threadsafe())
    {
        throw std::runtime_error("Need a threadsafe SQLite version, aborting");
    }

    if (access(db_file.c_str(), R_OK | W_OK) != 0 && errno != ENOENT)
    {
        throw std::runtime_error(
                "Error while accessing sqlite database file " +
                db_file + ": " + strerror(errno));
    }

    int flags = 0;

    if (readonly)
    {
        flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX;
    }
    else
    {
        flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    }

    int ret = sqlite3_open_v2(db_file.c_str(), &m_db, flags, NULL);
    if (ret != SQLITE_OK)
    {
        throw std::runtime_error("Could not open database " + db_file + ": " +
                         sqlite3_errmsg(m_db));
    }
}

boost::shared_ptr<SQLite3::query_result> SQLite3::query(std::string q)
{
    sqlite3_stmt *statement;

    int ret;

    boost::mutex::scoped_lock lock(m_mutex);
    boost::shared_ptr<SQLite3::query_result> results(
                                                new SQLite3::query_result());

    ret = sqlite3_prepare_v2(m_db, q.c_str(), -1, &statement, 0);
    if (ret != SQLITE_OK)
    {
        std::string msg = sqlite3_errmsg(m_db);
        throw std::runtime_error(msg);
    }

    int columns = sqlite3_column_count(statement);
    do
    {
        ret = sqlite3_step(statement);
        if (ret == SQLITE_ROW)
        {
            boost::shared_ptr<SQLite3::row_result> row(
                                                    new SQLite3::row_result());
            for (int i = 0; i < columns; i++)
            {
                int type = sqlite3_column_type(statement, i);
                const unsigned char *text = sqlite3_column_text(statement, i);
                std::string name = sqlite3_column_name(statement, i);
                std::string data = text ? (const char *)text : "";

                boost::shared_ptr<SQLite3::cell> cell(
                    new SQLite3::cell());

                cell->name = name;
                cell->data = data;
                cell->type = type;

                row->push_back(cell);
            }
            results->push_back(row);
        }
    } while (ret == SQLITE_ROW);

    sqlite3_finalize(statement);

    return results;
}

// CAUTION: this function does not lock the mutex! it can be used as a helper
// for API functions that need to perform more than one db operation
// sequentially
void SQLite3::execInternal(std::string sql)
{
    char *errmsg = NULL;
    int ret;

    ret = sqlite3_exec(m_db, sql.c_str(), NULL, NULL, &errmsg);
    if (ret != SQLITE_OK)
    {
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

long long int SQLite3::execAndGetRowId(std::string sql)
{
    boost::mutex::scoped_lock lock(m_mutex);
    execInternal(sql);
    return (long long int)sqlite3_last_insert_rowid(m_db);
}

std::string SQLite3::escape(std::string str, bool quotes)
{
    char *q;
    if (quotes)
    {
        q = sqlite3_mprintf("'%Q'", (str.empty() ? "" : str.c_str()));
    }
    else
    {
        q = sqlite3_mprintf("'%q'", (str.empty() ? "" : str.c_str()));
    }
    std::string ret = q;
    sqlite3_free(q);
    return ret;
}

long long int SQLite3::getLastInsertedRowId()
{
    boost::mutex::scoped_lock lock(m_mutex);
    return (long long int)sqlite3_last_insert_rowid(m_db);
}

bool SQLite3::isFatal(int error)
{
    switch (error)
    {
        case SQLITE_READONLY:
        case SQLITE_IOERR:
        case SQLITE_CORRUPT:
        case SQLITE_FULL:
        case SQLITE_NOTADB:
            return true;
            break;
        case SQLITE_OK:
        default:
            return false;
    }
}

SQLite3::~SQLite3()
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_db)
    {
        sqlite3_close(m_db);
    }
}

} // namespace

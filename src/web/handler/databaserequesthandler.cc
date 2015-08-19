/*
    Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <boost/make_shared.hpp>

#include "databaserequesthandler.h"
#include "src/stringconverter.h"
#include "src/sqlite3_wrapper.h"

namespace dss {

  //=========================================== DatabaseRequestHandler

  WebServerResponse DatabaseRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    StringConverter st("UTF-8", "UTF-8");
    if (_request.getMethod() == "query") {
      std::string database = st.convert(_request.getParameter("database"));
      if (database.empty()) {
        return JSONWriter::failure("Missing parameter 'database'");
      }

      if (database.length() != strcspn(database.c_str(), "./\\$")) {
        return JSONWriter::failure("Database name contains invalid characters");
      }

      database = DSS::getInstance()->getDatabaseDirectory() + database + ".db";

      std::string sql = st.convert(_request.getParameter("sql"));
      if (sql.empty()) {
        return JSONWriter::failure("Missing parameter 'sql'");
      }

      boost::shared_ptr<SQLite3> sqlite =
          boost::make_shared<SQLite3>(database, true);

      boost::shared_ptr<SQLite3::query_result> q = sqlite->query(sql);

      JSONWriter json;
      json.startArray("data");

      for (size_t i = 0; i < q->size(); i++) {
        boost::shared_ptr<SQLite3::row_result> rr = q->at(i);
        // row
        json.startObject();
          for (size_t j = 0; j < rr->size(); j++) {
            boost::shared_ptr<SQLite3::cell> cell = rr->at(j);
            json.add(cell->name, cell->data);
          }
        json.endObject();
      }

      json.endArray();

      return json.successJSON();
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest
} // namespace dss

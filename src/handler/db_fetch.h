/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
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

#pragma once

#include <string>

#include "src/event.h"
#include "src/taskprocessor.h"

namespace dss {

  /**
   * DbFetch - fetches an sql dump and (re-)creates a database
   *
   * Will emit event DatabaseImported:
   * - success: 0/1 (1 is success)
   * - id: opaque string selected by user
   * - url: location of sqldump
   * - desc: in case of error short error description
   */
  class DbFetch : public Task {
  public:
    /**
     * @_dbName - fish name of the db, usually used as the file name
     * @_url - where to download the update
     * @_reqID - the notification event will contain this id, opaque string
     */
    DbFetch(const std::string &_dbName, const std::string &_url, const std::string &_reqId);
    virtual ~DbFetch() {}
    virtual void run();

  private:
    void raiseNotificationEvent(bool _success, const std::string &_desc = "");

  private:
    std::string m_dbName;
    std::string m_url;
    std::string m_reqId;
  };

  /**
   * configuration via property node
   * pcn_dbu = property config node - database update
   */
  extern const std::string pcn_dbu;

  /**
   * pcn_dbu_list -- list of db to be updated
   *
   *   <config>
   *   |- <pcn_dbu>
   *   .  \- <list>
   *   .     |-  <service1> -> <string:/config/service1/db>
   *   .     \-  <service2> -> <string:/config/service2/db>
   *   |- <service1>
   *   .  \- <db>
   *   .     |- <name> -> <string:vdc-db>
   *   .     \- <url>  -> <string:http://db.aizo.net/vdc-db.php>
   *   \- <service2>
   *      \- ..
   */
  extern const std::string  pcn_dbu_list;

  class DbUpdatePlugin : public EventInterpreterPlugin {
  public:
    DbUpdatePlugin(EventInterpreter* _pInterpreter);
    virtual ~DbUpdatePlugin() {};
    virtual void subscribe();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };
}

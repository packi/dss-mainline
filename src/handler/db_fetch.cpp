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

#include "config.h"
#include "src/handler/db_fetch.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

#include "src/event.h"
#include "src/logger.h"
#include "src/sqlite3_wrapper.h"

namespace dss {

DbFetch::DbFetch(const std::string &_dbName, const std::string &_url,
                 const std::string &_reqId)
  : Task(), m_dbName(_dbName), m_url(_url), m_reqId(_reqId) {
}

void DbFetch::raiseNotificationEvent(bool _success, const std::string &_desc) {
  boost::shared_ptr<Event> pEvent =
    boost::make_shared<Event>(EventName::DatabaseImported);
  pEvent->setProperty("success", _success ? "1" : "0");
  pEvent->setProperty("id", m_reqId);
  pEvent->setProperty("url", m_url);
  pEvent->setProperty("desc", _desc);
  DSS::getInstance()->getEventQueue().pushEvent(pEvent);
}

void DbFetch::run() {
  if (m_reqId.empty() || m_url.empty()) {
    Logger::getInstance()->log("DbFetch: missing id or request id", lsError);
    return;
  }

  HttpClient http;
  std::string result;

  // TODO check upstream if update necessary, skip if not needed

  long req = http.get(m_url, &result);
  if (req != 200 && req != 0) {
    // file:// will return 0/-1 upon success/failure
    Logger::getInstance()->log("DbFetch: invalid URL " + m_url + ": HTTP code " + intToString(req), lsWarning);
    raiseNotificationEvent(false, "invalid url");
    return;
  }

  if (result.empty()) {
    Logger::getInstance()->log("DbFetch: empty sql dump from " + m_url + " ignoring", lsWarning);
    raiseNotificationEvent(false, "fetched sql dump is empty");
    return;
  }

  std::string database = DSS::getInstance()->getDatabaseDirectory() + m_dbName;
  if (!boost::algorithm::ends_with(database, ".db")) {
    database += ".db";
  }

  try {

    SQLite3 sqlite(database, true, 0);
    sqlite.exec(result);
    raiseNotificationEvent(true);

  } catch (std::runtime_error &ex) {

    Logger::getInstance()->log("DbFetch: db import failed: " + std::string(ex.what()), lsWarning);
    raiseNotificationEvent(false, "failed to import sql dump");
  }
}

const std::string pcn_dbu = "/config/db_update_service";
const std::string  pcn_dbu_list = pcn_dbu + "/list";

DbUpdatePlugin::DbUpdatePlugin(EventInterpreter* _pInterpreter)
    : EventInterpreterPlugin("dss_db_update", _pInterpreter)
{
}

void DbUpdatePlugin::subscribe() {
  boost::shared_ptr<EventSubscription> subscription =
    boost::make_shared<EventSubscription>(EventName::DatabaseImported,
                                          getName(),
                                          boost::ref(getEventInterpreter()),
                                          boost::make_shared<SubscriptionOptions>());
  getEventInterpreter().subscribe(subscription);
}

void DbUpdatePlugin::handleEvent(Event& _event, const EventSubscription& _subscription) {
  PropertySystem &props(DSS::getInstance()->getPropertySystem());
  Logger::getInstance()->log("handleEvent", lsWarning);

  if (!props.getProperty(pcn_dbu_list) ||
      !props.getProperty(pcn_dbu_list)->getChildCount()) {
    // nothing to do
    return;
  }

  // start new poll loop
  bool pollNext = (_event.getName() == EventName::CheckDssDbUpdate);

  for (int i = 0; i < props.getProperty(pcn_dbu_list)->getChildCount(); ++i) {
    PropertyNodePtr ref = props.getProperty(pcn_dbu_list)->getChild(i);
    PropertyNodePtr db = props.getProperty(ref->getStringValue());

    if (_event.getName() == EventName::DatabaseImported && !pollNext) {
      // last db poll completed, need to find the url coming next
      pollNext = _event.getPropertyByName("id") == ref->getStringValue();
      continue;
    }

    if (!db) {
      Logger::getInstance()->log("DSS-DB: non-existent db node <" + ref->getStringValue() + ">", lsNotice);
      continue;
    }

    if (!db->getProperty("url") || !db->getProperty("name")) {
      Logger::getInstance()->log("DSS-DB: missing db name or url <" + ref->getStringValue() + ">", lsNotice);
      continue;
    }

    std::string name(db->getProperty("name")->getStringValue());
    std::string url(db->getProperty("url")->getStringValue());
    Logger::getInstance()->log("DSS-DB> name:" + name + " url:" + url, lsInfo);

    TaskProcessor &tp(DSS::getInstance()->getModelMaintenance().getTaskProcessor());
    tp.addEvent(boost::make_shared<DbFetch>(name, url, ref->getStringValue()));
    break;
  }
}

}

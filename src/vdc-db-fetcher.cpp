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

#include "vdc-db-fetcher.h"
#include <ds/log.h>
#include "src/logger.h"
#include "src/sqlite3_wrapper.h"

#include <atomic>
#include "dss.h"
#include "vdc-db.h"
#include "model/modelmaintenance.h"

typedef boost::system::error_code error_code;

namespace dss {

__DEFINE_LOG_CHANNEL__(VdcDbFetcher, lsInfo);

VdcDbFetcher::VdcDbFetcher(DSS &dss) :
    m_dss(dss),
    m_timer(dss.getIoService()),
    m_configNode(dss.getPropertySystem().createProperty("/config/vdcDbFetcher")),
    m_enabled(m_configNode->getOrCreateChildValue<bool>("enabled", false)),
    m_period(boost::chrono::seconds(m_configNode->getOrCreateChildValue<int>("periodSeconds", 24 * 60 * 60))),
    m_url(m_configNode->getOrCreateChildValue<std::string>("url", "http://db.aizo.net/vdc-db.php")) {
  log("VdcDbFetcher m_enabled:" + intToString(m_enabled) + " m_period:" + intToString(m_period.count())
      + " m_url:" + m_url, lsNotice);
  VdcDb::recreate(dss);
  asyncLoop();
}

void VdcDbFetcher::asyncLoop() {
  DS_ASSERT(m_dss.getIoService().thisThreadMatches());
  if (!m_enabled) {
    log("VdcDbFetcher disabled", lsNotice);
    return;
  }
  assert(m_period.count() != 0);
  m_timer.expires_from_now(m_period);
  m_timer.async_wait([=](const error_code &e) {
    if (e) {
      return; // aborted
    }
    asyncLoop();
  });

  auto task = makeTask([this]() {
    fetchTask();
  });
  m_taskScope = TaskScope(task); // cancel the old task if possible
  m_dss.getModelMaintenance().getTaskProcessor().addEvent(std::move(task));
  log(std::string() + "Task scheduled", lsInfo);
}

void VdcDbFetcher::fetchTask() {
  // ATTENTION: runs in another thread (task processor thread)
  log(std::string() + "ENTER" + __PRETTY_FUNCTION__, lsDebug);
  try {
    HttpClient http;
    std::string result;

    long code = http.get(m_url, &result);
    if (code != 200 && code!= 0) { // file:// will return 0/-1 upon success/failure
      throw std::runtime_error(std::string("Http request failed. url:") + m_url + " code:" + intToString(code));
    }

    VdcDb db(m_dss, SQLite3::Mode::ReadWrite);
    db.getDb().exec(result);
    log(std::string("Database updated"), lsNotice);
  } catch (std::runtime_error &e) {
    log(std::string() + "Fetch failed. e.what():" + e.what(), lsError);
      //This is development feature. No special error recovery. We wait for next period.
  }
}

} // namespace dss

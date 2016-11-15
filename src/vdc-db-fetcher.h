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

#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include "logger.h"
#include "taskprocessor.h"
#include "propertysystem.h"
#include "sqlite3_wrapper.h"
#include "vdc-db.h"

namespace dss {
  class DSS;

  /// Initialize vdc-db in constructor and schedule vdc-db updates after start and periodically.
  class VdcDbFetcher {
  public:
    static void recreateDb(DSS &dss);

    VdcDbFetcher(DSS &dss);

  private:
    __DECL_LOG_CHANNEL__;
    DSS& m_dss;
    boost::asio::basic_waitable_timer<boost::chrono::steady_clock>  m_timer;
    TaskScope m_taskScope;
    PropertyNodePtr m_configNode;
    bool m_enabled;
    boost::chrono::seconds m_period;
    std::string m_url;

    void asyncLoop();
    void fetchTask();
  };
}

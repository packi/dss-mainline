/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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
#include "status.h"

#include "ds/random.h"
#include "ds/log.h"

#include "base.h"
#include "foreach.h"
#include "status-field.h"
#include "group.h"
#include "apartment.h"
#include "dss.h"

namespace dss {

__DEFINE_LOG_CHANNEL__(Status, lsNotice);

boost::chrono::seconds Status::PUSH_PERIOD = boost::chrono::minutes(90);

Status::Status(Group& group)
    : m_group(group)
    , m_periodicPushTimer(group.getApartment().getDss()->getIoService())
    , m_malfunctionField(*this, StatusFieldType::MALFUNCTION)
    , m_serviceField(*this, StatusFieldType::SERVICE) {
  // Start periodic push loop to recover from broadcast failures,
  // to spread the value to new devices.
  // It makes no sense to push immediately,
  // we are likely to even not have dsm-api connection now.
  asyncPushLoopRestart(PUSH_PERIOD);
}

Status::~Status() = default;

StatusField& Status::getField(StatusFieldType type) {
  switch (type) {
    case StatusFieldType::MALFUNCTION: return m_malfunctionField;
    case StatusFieldType::SERVICE: return m_serviceField;
  }
  DS_FAIL_REQUIRE("Unknow status field type.", type);
};

StatusSensorBitset Status::getValueAsBitset() const {
  StatusSensorBitset out;
  out |= m_malfunctionField.getValueAsBitset();
  out |= m_serviceField.getValueAsBitset();
  return out;
}

void Status::push() {
  log(ds::str("push ", *this), lsInfo);
  // moderate push calls
  asyncPushLoopRestart(boost::chrono::seconds(1));
}

void Status::asyncPushLoopRestart(ds::asio::Timer::Duration delay) {
  log(ds::str("asyncPushLoopRestart ", *this), lsDebug);

  m_periodicPushTimer.randomlyExpiresFromNowPercentDown(delay, 25, [this] {
    try {
      auto&& value = getValueAsBitset().to_ulong();
      log(ds::str("push ", *this, " value:", value), lsNotice);
      m_group.pushSensor(coSystem, SAC_UNKNOWN, DSUID_NULL, SensorType::Status, value, "");
    } catch (std::exception &e) {
      log(ds::str("push failed what:", e.what()), lsError);
    }
    asyncPushLoopRestart(PUSH_PERIOD); // async loop
  });
}

std::ostream& operator<<(std::ostream& stream, const Status &x) {
    return stream << x.getGroup();
}

} // namespace dss

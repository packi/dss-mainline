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
    , m_periodicPushTimer(group.getApartment().getDss().getIoService())
    , m_malfunctionField(*this, StatusFieldType::MALFUNCTION)
    , m_serviceField(*this, StatusFieldType::SERVICE) {
  // Start periodic push loop to recover from broadcast failures,
  // to spread the value to new devices.
  // It makes no sense to push immediately,
  // we are likely to even not have dsm-api connection now.
  asyncPushLoop(PUSH_PERIOD);
}

Status::~Status() = default;

StatusField& Status::getField(StatusFieldType type) {
  switch (type) {
    case StatusFieldType::MALFUNCTION: return m_malfunctionField;
    case StatusFieldType::SERVICE: return m_serviceField;
  }
};

StatusSensorBitset Status::getValueAsBitset() const {
  StatusSensorBitset out;
  out |= m_malfunctionField.getValusAsBitset();
  out |= m_serviceField.getValusAsBitset();
  return out;
}

void Status::push() {
  log(ds::str("push ", *this), lsInfo);
  // moderate push calls
  asyncPushLoop(boost::chrono::seconds(1));
}

void Status::asyncPushLoop(ds::asio::Timer::Duration delay) {
  log(ds::str("asyncPushLoop ", *this), lsDebug);

  m_periodicPushTimer.randomlyExpiresFromNowPercentDown(delay, 25, [this] {
    try {
      auto&& value = getValueAsBitset().to_ulong();
      log(ds::str("push ", *this, " value:", value), lsNotice);
      m_group.pushSensor(coSystem, SAC_UNKNOWN, DSUID_NULL, SensorType::Status, value, "");
    } catch (std::exception &e) {
      log(ds::str("push failed what:", e.what()), lsError);
    }
    asyncPushLoop(PUSH_PERIOD); // async loop
  });
}

std::ostream& operator<<(std::ostream& stream, const Status &x) {
    return stream << x.getGroup();
}

} // namespace dss

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

boost::chrono::seconds Status::PUSH_SENSOR_PERIOD = boost::chrono::minutes(90);

Status::Status(Group& group)
    : m_group(group)
    , m_periodicPushTimer(group.getApartment().getDss().getIoService()) {
  asyncPeriodicPush();
}

Status::~Status() = default;

StatusField* Status::tryGetBit(StatusFieldType statusType) {
  auto&& it = m_bits.find(statusType);
  if (it != m_bits.end()) {
    return it->second.get();
  }
  return DS_NULLPTR;
}

void Status::insertBit(StatusFieldType statusType, std::unique_ptr<StatusField> state) {
  m_bits[statusType] = std::move(state);
}

void Status::setBitValue(StatusFieldType type, bool bitValue) {
  auto&& bit = static_cast<int>(type);
  if (m_valueBitset.test(bit) == bitValue) {
    return;
  }
  log(ds::str("setBitValue type:", type, " bitValue:", bitValue), lsInfo);
  m_valueBitset.set(bit, bitValue);
  auto&& value = getValue();
  log(ds::str("this:", m_group.getName(), " changed value:", value), lsNotice);
  push();
}

void Status::push() {
  try {
    auto&& value = getValue();
    log(ds::str("pushSensor ", *this, " value:", value), lsNotice);
    m_group.pushSensor(coSystem, SAC_UNKNOWN, DSUID_NULL, SensorType::Status, value, "");
  } catch (std::exception &e) {
    log(ds::str("setBitValue pushSensor failed what:", e.what()), lsError);
  }
}

void Status::asyncPeriodicPush() {
  log(ds::str("asyncPeriodicPush ", *this), lsDebug);

  m_periodicPushTimer.randomlyExpiresFromNowPercentDown(PUSH_SENSOR_PERIOD, 25, [this] {
      push();
      asyncPeriodicPush(); // async loop
  });
}

std::ostream& operator<<(std::ostream& stream, const Status &x) {
    return stream << x.getGroup();
}

} // namespace dss

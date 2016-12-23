/*
    Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland

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
#pragma once

#include <boost/optional.hpp>
#include <logger.h>
#include "state.h"

namespace dss {

class StatusBit;

/// Status with each bit managed by StatusBit.
/// Status value changes are propagated as sensor value to parent group.
class Status : boost::noncopyable {
public:
  Status(Group& group);
  ~Status();

  Group& getGroup() { return m_group; }

  StatusBit* tryGetBit(StatusBitType statusType);
  void insertBit(StatusBitType statusType, boost::shared_ptr<StatusBit> state);
  unsigned int getValue() const { return m_valueBitset.to_ulong(); }

private:
  __DECL_LOG_CHANNEL__;
  Group& m_group;
  std::bitset<STATUS_BIT_TYPE_MAX + 1> m_valueBitset;
  std::map<StatusBitType, boost::shared_ptr<StatusBit>> m_bits;

  friend class StatusBit;
  void setBitValue(StatusBitType type, bool value);
};

} // namespace dss

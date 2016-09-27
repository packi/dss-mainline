/*
    Copyright (c) 2016 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifndef __VDC_HELPER_H__
#define __VDC_HELPER_H__

#include <string>

namespace dss {

  class Device;
  class JSONWriter;

  enum {
     VdcInfoFilterSpec          = 0,
     VdcInfoFilterStateDesc     = 1,
     VdcInfoFilterPropertyDesc  = 2,
     VdcInfoFilterActionDesc    = 3,
     VdcInfoFilterStdActions    = 4,
     VdcInfoFilterCustomActions = 5
  };

  void GetVdcSpec(const Device& device, JSONWriter& json);
  void GetVdcStateDescriptions(const Device& device, const std::string& langCode, JSONWriter& json);
  void GetVdcPropertyDescriptions(const Device& device, const std::string& langCode, JSONWriter& json);
  void GetVdcActionDescriptions(const Device& device, const std::string& langCode, JSONWriter& json);
  void GetVdcStandardActions(const Device& device, const std::string& langCode, JSONWriter& json);
  void GetVdcCustomActions(Device& device, JSONWriter& json);
  std::bitset<6> ParseVdcInfoFilter(const std::string& filterParam);
  void RenderVdcInfo(Device& device, const std::bitset<6>& filter,
                     const std::string& langCode, JSONWriter& json);

} // namespace

#endif//__VDC_HELPER_H__

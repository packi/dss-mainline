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
  class VdcDb;

  namespace vdcInfo {
    struct Filter {
      bool spec : 1;
      bool stateDesc : 1;
      bool eventDesc : 1;
      bool propertyDesc : 1;
      bool sensorDesc : 1;
      bool actionDesc : 1;
      bool stdActions : 1;
      bool customActions : 1;
      bool operational : 1;
    };

    void addStateDescriptions(VdcDb& db, const std::string& gtin, const std::string& langCode, JSONWriter& json);
    void addEventDescriptions(VdcDb& db, const std::string& gtin, const std::string& langCode, JSONWriter& json);
    void addPropertyDescriptions(VdcDb& db, const std::string& gtin, const std::string& langCode, JSONWriter& json);
    void addSensorDescriptions(VdcDb& db, const std::string& gtin, const std::string& langCode, JSONWriter& json);
    void addActionDescriptions(VdcDb& db, const std::string& gtin, const std::string& langCode, JSONWriter& json);
    void addStandardActions(VdcDb& db, const std::string& gtin, const std::string& langCode, JSONWriter& json);

    void addStateDescriptionsDev(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);
    void addEventDescriptionsDev(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);
    void addPropertyDescriptionsDev(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);
    void addSensorDescriptionsDev(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);
    void addActionDescriptionsDev(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);
    void addStandardActionsDev(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);

    void addSpec(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json);
    void addCustomActions(Device& device, JSONWriter& json);
    void addOperationalValues(VdcDb& db, Device& device, const std::string& langCode, JSONWriter& json);

    Filter parseFilter(const std::string& filterParam);
    void addByFilter(VdcDb& db, Device& device, Filter filter, const std::string& langCode, JSONWriter& json);
    void addByFilter(VdcDb& db, std::string& gtin, Filter filter, const std::string& langCode, JSONWriter& json);

  } // namespace vdcInfo

} // namespace

#endif//__VDC_HELPER_H__

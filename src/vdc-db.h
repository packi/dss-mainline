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
#include <vector>
#include <memory>
#include <mutex>

#include "logger.h"
#include "src/businterface.h"
#include "src/sqlite3_wrapper.h"

namespace dss {

class DSS;

class VdcDb {
public:
  VdcDb(DSS &dss, SQLite3::Mode mode = SQLite3::Mode::ReadOnly);

  static std::string getFilePath(DSS &dss);
  static void recreate(DSS &dss);

  SQLite3& getDb() { return m_db; }

  struct SpecDesc {
    std::string name;
    std::string title; //< translated label
    std::string tags;
    std::string value;
  };

  std::vector<SpecDesc> getSpec(const std::string &gtin, const std::string &langCode = ""); // throws

  struct StateDesc {
    std::string name;
    std::string title; //< translated name
    std::vector<std::pair<std::string, std::string>> values;
    // pair: < value, translated value(=title) >
    std::string tags;
  };

  std::vector<DeviceStateSpec_t> getStatesLegacy(const std::string &gtin); // throws
  std::vector<StateDesc> getStates(const std::string &gtin, const std::string &langCode = ""); // throws

  struct EventDesc {
    std::string name;
    std::string title; //< translated name
  };

  std::vector<EventDesc> getEvents(const std::string &gtin, const std::string &langCode = ""); // throws

  enum propertyTypeId { integer, numeric, enumeration, string };

  struct PropertyDesc {
    std::string name; // technical name
    std::string title; // translated name alt_label
    propertyTypeId typeId;
    std::string defaultValue;
    std::string minValue;
    std::string maxValue;
    std::string resolution;
    std::string siUnit;
    std::string tags;
  };

  std::vector<PropertyDesc> getProperties(const std::string &gtin, const std::string &langCode = "");

  struct ActionDesc {
    std::string name;
    std::string title;
    std::vector<PropertyDesc> params;
  };

  std::vector<ActionDesc> getActions(const std::string &gtin, const std::string &langCode = "");

  struct StandardActionDesc {
    std::string name;
    std::string title;
    std::string action_name;
    std::vector<std::pair<std::string, std::string>> args;
    // pair <argument name, argument value>
  };

  std::vector<StandardActionDesc> getStandardActions(const std::string &gtin, const std::string &langCode = "");
  bool hasActionInterface(const std::string &gtin);

private:
  __DECL_LOG_CHANNEL__;
  SQLite3 m_db;
  static std::mutex s_mutex;
  std::lock_guard<std::mutex> m_lock;

  void extractLangAndCountry(const std::string &langCode, std::string &lang, std::string &country);
};

}

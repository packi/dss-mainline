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

#include "src/businterface.h"
#include "src/sqlite3_wrapper.h"

namespace dss {

/**
 * property node structure to declare trigger
 * pcn_ = property config node
 */
extern const std::string pcn_vdce_db;
extern const std::string  pcn_vdce_db_name;

class VdcDb {
public:
  VdcDb();

  struct StateDesc {
    std::string name;
    std::string title; //< translated name
    std::vector<std::pair<std::string, std::string>> values;
    // pair: < value, translated value(=title) >
  };

  bool lookupStates(const std::string &gtin, std::vector<DeviceStateSpec_t> *out);
  bool lookupStates(const std::string &gtin, std::vector<StateDesc> *out, const std::string &langCode = "");

  struct PropertyDesc {
    std::string name; // technical name
    std::string title; // translated name alt_label
    bool readonly;
  };

  bool lookupProperties(const std::string &gtin, std::vector<PropertyDesc> *out, const std::string &langCode = "");

  struct ActionParameter {
    std::string name;
    std::string title;
    int defaultValue;
  };

  struct ActionDesc {
    std::string name;
    std::string title;
    std::vector<ActionParameter> params;
  };

  bool lookupActions(const std::string &gtin, std::vector<ActionDesc> *out, const std::string &langCode = "");

  struct StandardActionDesc {
    std::string name;
    std::string title;
    std::string action_name;
    std::vector<std::pair<std::string, std::string>> args;
    // pair <argument name, argument value>
  };

  bool lookupStandardActions(const std::string &gtin, std::vector<StandardActionDesc> *out, const std::string &langCode = "");

private:
  std::unique_ptr<SQLite3> m_db;
};

}

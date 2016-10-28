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

#include "config.h"
#include "src/vdc-db.h"

#include <boost/algorithm/string/predicate.hpp>

#include "src/base.h"
#include "src/dss.h"
#include "src/foreach.h"
#include "src/logger.h"
#include "src/propertysystem.h"
#include "src/sqlite3_wrapper.h"

namespace dss {

// Parallel readers and writers would introduce errors returned by sqlite.
// It is possible to recover from these errors, but we don't aim for parallelism here.
// We avoid concurrency errors by locking this static mutex in each VdcDb instance.
std::mutex VdcDb::s_mutex;

VdcDb::VdcDb(SQLite3::Mode mode):
    m_db(getFilePath(), mode),
    m_lock(s_mutex) {
}

std::string VdcDb::getFilePath() {
  return DSS::getInstance()->getDatabaseDirectory() + "/vdc.db";
}

std::vector<DeviceStateSpec_t> VdcDb::getStatesLegacy(const std::string &gtin) {
  std::vector<DeviceStateSpec_t> states;

  foreach (const StateDesc desc, getStates(gtin)) {
    states.push_back(DeviceStateSpec_t());
    states.back().Name = desc.name;
    foreach (auto cur, desc.values) {
      states.back().Values.push_back(cur.first);
      // extract only technical name, drop translation
    }
  }

  return states;
}

std::vector<VdcDb::SpecDesc> VdcDb::getSpec(const std::string &gtin, const std::string &langCode) {
  std::string sql("select * from callGetSpec where (gtin=? or gtin='') and lang_code=?");
  std::string lang;
  if (!langCode.empty()) {
    lang = langCode;
  } else {
    lang = "base";
  }
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);

  std::vector<SpecDesc> specs;
  while (query.step() != SqlStatement::StepResult::DONE) {
    specs.push_back(SpecDesc());
    specs.back().name = query.getColumn<std::string>(0);
    specs.back().title = query.getColumn<std::string>(1);
    specs.back().tags = query.getColumn<std::string>(2);
    specs.back().value = query.getColumn<std::string>(3);
  }
  return specs;
}

std::vector<VdcDb::StateDesc> VdcDb::getStates(const std::string &gtin, const std::string &langCode) {
  std::string sql("select * from callGetStates where gtin=? and lang_code=?");
  std::string lang;
  if (!langCode.empty()) {
    lang = langCode;
  } else {
    lang = "base";
  }
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);

  std::vector<StateDesc> states;
  std::string cur(""); // invalid state name
  while (query.step() == SqlStatement::StepResult::ROW) {
    std::string name = query.getColumn<std::string>(0);
    if (states.empty() || cur != name) {
      // new state
      states.push_back(StateDesc());
      states.back().name = cur = name;
      states.back().title = query.getColumn<std::string>(1);
      states.back().tags = query.getColumn<std::string>(4);
    }
    auto value = std::make_pair(query.getColumn<std::string>(2), query.getColumn<std::string>(3));
    states.back().values.push_back(value);
  }

  return states;
}

std::vector<VdcDb::EventDesc> VdcDb::getEvents(const std::string &gtin, const std::string &langCode) {
  std::string sql("select * from callGetEvents where gtin=? and lang_code=?");
  std::string lang;
  if (!langCode.empty()) {
    lang = langCode;
  } else {
    lang = "base";
  }
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);

  std::vector<EventDesc> events;
  while (query.step() != SqlStatement::StepResult::DONE) {
    events.push_back(EventDesc());
    events.back().name = query.getColumn<std::string>(0);
    events.back().title = query.getColumn<std::string>(1);
  }

  return events;
}

std::vector<VdcDb::PropertyDesc> VdcDb::getProperties(const std::string &gtin, const std::string &langCode) {
  std::string sql("select * from callGetProperties where gtin=? and lang_code=?");
  std::string lang;
  if (!langCode.empty()) {
    lang = langCode;
  } else {
    lang = "base";
  }
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);

  std::vector<PropertyDesc> props;
  while (query.step() != SqlStatement::StepResult::DONE) {
    props.push_back(PropertyDesc());
    props.back().name = query.getColumn<std::string>(0);
    props.back().title = query.getColumn<std::string>(1);
    switch (query.getColumn<int>(2)) {
      case 1:
      case 2:
        props.back().typeId = propertyTypeId::integer;
        break;
      case 3:
        props.back().typeId = propertyTypeId::numeric;
        break;
      case 4:
        props.back().typeId = propertyTypeId::string;
        break;
      case 5:
        props.back().typeId = propertyTypeId::enumeration;
        break;
    }
    props.back().defaultValue = query.getColumn<std::string>(3);
    props.back().minValue = query.getColumn<std::string>(4);
    props.back().maxValue = query.getColumn<std::string>(5);
    props.back().resolution = query.getColumn<std::string>(6);
    props.back().siUnit = query.getColumn<std::string>(7);
    props.back().tags = query.getColumn<std::string>(8);
  }

  return props;
}

std::vector<VdcDb::ActionDesc> VdcDb::getActions(const std::string &gtin, const std::string &langCode) {
  std::string sql("select * from callGetActions where gtin=? and lang_code=?");
  std::string lang;
  if (!langCode.empty()) {
    lang = langCode;
  } else {
    lang = "base";
  }
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);

  std::vector<ActionDesc> actions;
  std::string cur("");
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    if (actions.empty() || cur != name) {
      actions.push_back(ActionDesc());
      actions.back().name = cur = name;
      actions.back().title = query.getColumn<std::string>(1);
    }
    if (sqlite3_column_type(query, 2) != SQLITE_NULL) {
      actions.back().params.push_back(PropertyDesc());
      actions.back().params.back().name = query.getColumn<std::string>(2);
      actions.back().params.back().title = query.getColumn<std::string>(3);
      switch (query.getColumn<int>(4)) {
        case 1:
        case 2:
          actions.back().params.back().typeId = propertyTypeId::integer;
          break;
        case 3:
          actions.back().params.back().typeId = propertyTypeId::numeric;
          break;
        case 4:
          actions.back().params.back().typeId = propertyTypeId::string;
          break;
        case 5:
          actions.back().params.back().typeId = propertyTypeId::enumeration;
          break;
      }
      actions.back().params.back().defaultValue = query.getColumn<std::string>(5);
      actions.back().params.back().minValue = query.getColumn<std::string>(6);
      actions.back().params.back().maxValue = query.getColumn<std::string>(7);
      actions.back().params.back().resolution = query.getColumn<std::string>(8);
      actions.back().params.back().siUnit = query.getColumn<std::string>(9);
      actions.back().params.back().tags = query.getColumn<std::string>(10);
    }
  }

  return actions;
}

std::vector<VdcDb::StandardActionDesc> VdcDb::getStandardActions(const std::string &gtin, const std::string &langCode) {
  std::string sql("select * from callGetStandardActions where gtin=? and lang_code=?");
  std::string lang;
  if (!langCode.empty()) {
    lang = langCode;
  } else {
    lang = "base";
  }
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);

  std::vector<StandardActionDesc> desc;
  std::string cur("");
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    if (desc.empty() || cur != name) {
      desc.push_back(StandardActionDesc());
      desc.back().name = cur = name;
      desc.back().title = query.getColumn<std::string>(1);
      desc.back().action_name = query.getColumn<std::string>(2);
    }
    if (sqlite3_column_type(query, 3) != SQLITE_NULL) {
      auto arg = std::make_pair(query.getColumn<std::string>(3), query.getColumn<std::string>(4));
      desc.back().args.push_back(arg);
    }
  }

  return desc;
}

bool VdcDb::hasActionInterface(const std::string &gtin) {
  Logger::getInstance()->log(std::string(__func__) + "check action interface for GTIN " + gtin);

  bool result = false;
  std::string sql = "SELECT name FROM device WHERE gtin=?";
  SqlStatement find = m_db.prepare(sql);
  SqlStatement::BindScope scope;
  scope = find.bind(gtin);

  while (find.step() == SqlStatement::StepResult::ROW) {
    std::string name = find.getColumn<std::string>(0);
    result = true;

    Logger::getInstance()->log(std::string(__func__) + "found name for GTIN " + gtin + ": ", name);
  }

  return result;
}

} // dss namespace

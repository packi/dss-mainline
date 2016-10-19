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

std::vector<VdcDb::StateDesc> VdcDb::getStates(const std::string &gtin, const std::string &langCode) {
  std::vector<StateDesc> states;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT s.name, n_s.name, e.value, n_e.name, s.tags "
        "FROM device AS d "
        "INNER JOIN (device_status AS s "
        "INNER JOIN name_device_status AS n_s ON s.id=n_s.reference_id) "
        "INNER JOIN (device_status_enum AS e "
        "INNER JOIN name_device_status_enum AS n_e ON e.id=n_e.reference_id) "
        "ON d.id=s.device_id AND s.id=e.device_id WHERE d.gtin=? AND n_e.lang_code=? ORDER BY s.name;";
  } else {
    sql = "SELECT s.name, s.name, e.value, e.value, s.tags "
        "FROM device AS d INNER JOIN device_status AS s INNER JOIN device_status_enum AS e "
        "ON d.id == s.device_id AND s.id == e.device_id WHERE d.gtin=? ORDER BY s.name;";
  }

  SqlStatement findStates = m_db.prepare(sql);

  SqlStatement::BindScope scope;
  if (langCode.empty()) {
    scope = findStates.bind(gtin);
  } else {
    scope = findStates.bind(gtin, langCode);
  }

  std::string cur(""); // invalid state name
  while (findStates.step() == SqlStatement::StepResult::ROW) {
    std::string name = findStates.getColumn<std::string>(0);
    if (states.empty() || cur != name) {
      // new state
      states.push_back(StateDesc());
      states.back().name = cur = name;
      states.back().title = findStates.getColumn<std::string>(1);
      states.back().tags = findStates.getColumn<std::string>(4);
    }
    auto value = std::make_pair(findStates.getColumn<std::string>(2), findStates.getColumn<std::string>(3));
    states.back().values.push_back(value);
  }

  return states;
}

std::vector<VdcDb::EventDesc> VdcDb::getEvents(const std::string &gtin, const std::string &langCode) {
  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT p.name, n_p.name "
        "FROM (device AS dev "
          "INNER JOIN (device_events AS p "
            "INNER JOIN name_device_events AS n_p ON p.id=n_p.reference_id) ON dev.id=p.device_id) "
        "WHERE dev.gtin=? AND n_p.lang_code=?";
  } else {
    sql = "SELECT p.name, p.name FROM device AS dev INNER JOIN device_events AS p ON dev.id=p.device_id WHERE dev.gtin=?";
  }
  SqlStatement findEvents = m_db.prepare(sql);

  SqlStatement::BindScope scope;
  if (langCode.empty()) {
    scope = findEvents.bind(gtin);
  } else {
    scope = findEvents.bind(gtin, langCode);
  }

  std::vector<EventDesc> events;
  while (findEvents.step() != SqlStatement::StepResult::DONE) {
    events.push_back(EventDesc());
    events.back().name = findEvents.getColumn<std::string>(0);
    events.back().title = findEvents.getColumn<std::string>(1);
  }

  return events;
}

std::vector<VdcDb::PropertyDesc> VdcDb::getProperties(const std::string &gtin, const std::string &langCode) {
  std::vector<PropertyDesc> props;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT p.name, n_p.alt_label, p.type_id, p.default_value, p.min_value, p.max_value, p.resolution, p.si_unit, p.tags "
        "FROM (device AS dev "
          "INNER JOIN (device_properties AS p "
          "INNER JOIN name_device_properties AS n_p ON p.id=n_p.reference_id) "
            "ON dev.id=p.device_id) "
        "WHERE dev.gtin=? AND n_p.lang_code=?";
  } else {
    sql = "SELECT p.name, p.name, p.type_id, p.default_value, p.min_value, p.max_value, p.resolution, p.si_unit, p.tags "
        "FROM device AS dev "
          "INNER JOIN device_properties AS p ON dev.id=p.device_id "
        "WHERE dev.gtin=?";
  }
  SqlStatement findProps = m_db.prepare(sql);

  SqlStatement::BindScope scope;
  if (langCode.empty()) {
    scope = findProps.bind(gtin);
  } else {
    scope = findProps.bind(gtin, langCode);
  }

  while (findProps.step() != SqlStatement::StepResult::DONE) {
    props.push_back(PropertyDesc());
    props.back().name = findProps.getColumn<std::string>(0);
    props.back().title = findProps.getColumn<std::string>(1);
    switch (findProps.getColumn<int>(2)) {
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
    props.back().defaultValue = findProps.getColumn<std::string>(3);
    props.back().minValue = findProps.getColumn<std::string>(4);
    props.back().maxValue = findProps.getColumn<std::string>(5);
    props.back().resolution = findProps.getColumn<std::string>(6);
    props.back().siUnit = findProps.getColumn<std::string>(7);
    props.back().tags = findProps.getColumn<std::string>(8);
  }

  return props;
}

std::vector<VdcDb::ActionDesc> VdcDb::getActions(const std::string &gtin, const std::string &langCode) {
  std::vector<ActionDesc> actions;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT a.command, n_a.name, p.name, n_p.name, p.type_id, p.default_value, p.min_value, p.max_value, p.resolution, p.si_unit, p.tags, CASE WHEN p.type_id is NULL THEN \"true\" ELSE \"false\" END AS paramexists "
        "FROM (device AS dev "
          "INNER JOIN (device_actions AS a INNER JOIN name_device_actions AS n_a ON a.id=n_a.reference_id) ON dev.id=a.device_id) "
          "LEFT JOIN (device_actions_parameter AS p INNER JOIN name_device_actions_parameter AS n_p ON p.id=n_p.reference_id) ON a.id=p.device_actions_id "
        "WHERE dev.gtin=? AND n_a.lang_code=? AND (n_p.lang_code=? OR paramexists=\"true\") "
        "ORDER BY a.command";
  } else {
    sql = "SELECT a.command, a.command, p.name, p.name, p.type_id, p.default_value, p.min_value, p.max_value, p.resolution, p.si_unit, p.tags FROM device AS d "
          "JOIN (device_actions AS a "
          "LEFT JOIN device_actions_parameter AS p ON a.id=p.device_actions_id) ON d.id=a.device_id "
        "WHERE gtin=? ORDER BY a.command";
  }

  // SqlStatement has broken move semantics
  SqlStatement findActions = m_db.prepare(sql);

  SqlStatement::BindScope scope;
  if (langCode.empty()) {
    scope = findActions.bind(gtin);
  } else {
    scope = findActions.bind(gtin, langCode, langCode);
  }

  std::string cur("");
  while (findActions.step() != SqlStatement::StepResult::DONE) {
    std::string name = findActions.getColumn<std::string>(0);
    if (actions.empty() || cur != name) {
      actions.push_back(ActionDesc());
      actions.back().name = cur = name;
      actions.back().title = findActions.getColumn<std::string>(1);
    }
    if (sqlite3_column_type(findActions, 2) != SQLITE_NULL) {
      actions.back().params.push_back(PropertyDesc());
      actions.back().params.back().name = findActions.getColumn<std::string>(2);
      actions.back().params.back().title = findActions.getColumn<std::string>(3);
      switch (findActions.getColumn<int>(4)) {
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
      actions.back().params.back().defaultValue = findActions.getColumn<std::string>(5);
      actions.back().params.back().minValue = findActions.getColumn<std::string>(6);
      actions.back().params.back().maxValue = findActions.getColumn<std::string>(7);
      actions.back().params.back().resolution = findActions.getColumn<std::string>(8);
      actions.back().params.back().siUnit = findActions.getColumn<std::string>(9);
      actions.back().params.back().tags = findActions.getColumn<std::string>(10);
    }
  }

  return actions;
}

std::vector<VdcDb::StandardActionDesc> VdcDb::getStandardActions(const std::string &gtin, const std::string &langCode) {
  std::vector<StandardActionDesc> desc;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT p.name, n_p.name, a.command, ap.name, pp.value, CASE WHEN ap.type_id is NULL THEN \"true\" ELSE \"false\" END AS paramexists "
        "FROM device AS d INNER JOIN ((device_actions AS a "
          "INNER JOIN (device_actions_predefined AS p INNER JOIN name_device_actions_predefined AS n_p ON p.id=n_p.reference_id) ON a.id=p.device_actions_id) "
          "LEFT JOIN (device_actions_parameter AS ap INNER JOIN device_actions_predefined_parameter AS pp ON pp.device_actions_parameter_id=ap.id) "
          "ON a.id=p.device_actions_id AND p.id=pp.device_actions_predefined_id) "
          "ON d.id=a.device_id "
        "WHERE gtin=? AND (n_p.lang_code=? OR paramexists=\"true\") "
        "ORDER BY a.command;";
  } else {
    sql = "SELECT p.name, p.name, a.command, ap.name, pp.value "
        "FROM device AS d INNER JOIN ((device_actions AS a "
          "INNER JOIN device_actions_predefined AS p ON a.id=p.device_actions_id) "
          "LEFT JOIN (device_actions_predefined_parameter AS pp "
          "INNER JOIN device_actions_parameter AS ap ON pp.device_actions_parameter_id=ap.id) "
          "ON a.id=p.device_actions_id AND p.id=pp.device_actions_predefined_id) "
          "ON d.id=a.device_id "
        "WHERE gtin=? ORDER BY a.command;";
  }

  SqlStatement find = m_db.prepare(sql);

  SqlStatement::BindScope scope;
  if (langCode.empty()) {
    scope = find.bind(gtin);
  } else {
    scope = find.bind(gtin, langCode);
  }

  std::string cur("");
  while (find.step() != SqlStatement::StepResult::DONE) {
    std::string name = find.getColumn<std::string>(0);
    if (desc.empty() || cur != name) {
      desc.push_back(StandardActionDesc());
      desc.back().name = cur = name;
      desc.back().title = find.getColumn<std::string>(1);
      desc.back().action_name = find.getColumn<std::string>(2);
    }
    if (sqlite3_column_type(find, 3) != SQLITE_NULL) {
      auto arg = std::make_pair(find.getColumn<std::string>(3), find.getColumn<std::string>(4));
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

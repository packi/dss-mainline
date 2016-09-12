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
#include "src/model/vdce_db.h"

#include <boost/algorithm/string/predicate.hpp>

#include "src/base.h"
#include "src/dss.h"
#include "src/foreach.h"
#include "src/logger.h"
#include "src/propertysystem.h"
#include "src/sqlite3_wrapper.h"

namespace dss {

const std::string pcn_vdce_db = "/config/vdce/db";
const std::string pcn_vdce_db_name = pcn_vdce_db + "/name";

static std::string get_dbfile() {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();

  if (!propSystem.getProperty(pcn_vdce_db_name)) {
    return "";
  }

  std::string dbName = propSystem.getProperty(pcn_vdce_db_name)->getAsString();
  if (!boost::algorithm::ends_with(dbName, ".db")) {
    dbName += ".db";
  }

  return propSystem.getProperty("/config/databasedirectory")->getAsString() + dbName;
}

static std::string get_fallback() {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  return propSystem.getProperty("/config/datadirectory")->getAsString() + "vdc-db.sql";
}

VdcDb::VdcDb() {
  try {
    m_db.reset(new SQLite3(get_dbfile(), false));
  } catch (std::exception &e) {
    // database missing or corrupt
    Logger::getInstance()->log(std::string(__func__) + " <" + e.what() + ">", lsWarning);
    Logger::getInstance()->log(std::string(__func__) + "creating db from fallback sqldump: " + get_fallback(), lsWarning);

    SQLite3 tmp(get_dbfile(), true);
    // open in read-write mode
    tmp.exec(readFile(get_fallback()));
    // esceptions are not catched

    m_db.reset(new SQLite3(get_dbfile(), false));
    // retry or throw exception
  }
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
    sql = "SELECT s.name, n_s.name, e.value, n_e.name FROM device AS d INNER JOIN (device_status AS s INNER JOIN name_device_status AS n_s ON s.id=n_s.reference_id) INNER JOIN (device_status_enum AS e INNER JOIN name_device_status_enum AS n_e ON e.id=n_e.reference_id) ON d.id=s.device_id AND s.id=e.device_id WHERE d.gtin=? AND n_e.lang_code=? ORDER BY s.name;";
  } else {
    sql = "SELECT s.name, s.name, e.value, e.value FROM device AS d INNER JOIN device_status AS s INNER JOIN device_status_enum AS e ON d.id == s.device_id AND s.id == e.device_id WHERE d.gtin=? ORDER BY s.name;";
  }

  SqlStatement findStates = m_db->prepare(sql);

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
    }
    auto value = std::make_pair(findStates.getColumn<std::string>(2), findStates.getColumn<std::string>(3));
    states.back().values.push_back(value);
  }

  if (states.size() == 0) {
    throw std::runtime_error("no states for device: " + gtin);
  }

  return states;
}

std::vector<VdcDb::PropertyDesc> VdcDb::getProperties(const std::string &gtin, const std::string &langCode) {
  std::vector<PropertyDesc> props;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT p.name, n_p.alt_label, p.readonly FROM (device AS dev INNER JOIN (device_properties AS p INNER JOIN name_device_properties AS n_p ON p.id=n_p.reference_id) ON dev.id=p.device_id) where dev.gtin=? AND n_p.lang_code=?";
  } else {
    sql = "SELECT p.name, p.name, p.readonly FROM device AS dev INNER JOIN device_properties AS p ON dev.id=p.device_id WHERE dev.gtin=?";
  }
  SqlStatement findProps = m_db->prepare(sql);

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
    props.back().readonly = static_cast<bool>(findProps.getColumn<int>(2));
  }

  if (props.empty()) {
    throw std::runtime_error("no properties for device: " + gtin);
  }

  return props;
}

std::vector<VdcDb::ActionDesc> VdcDb::getActions(const std::string &gtin, const std::string &langCode) {
  std::vector<ActionDesc> actions;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT a.command, n_a.name, p.name, n_p.name, p.default_value FROM (device AS dev INNER JOIN (device_actions AS a INNER JOIN name_device_actions AS n_a ON a.id=n_a.reference_id) ON dev.id=a.device_id) INNER JOIN (device_actions_parameter AS p INNER JOIN name_device_actions_parameter AS n_p ON p.id=n_p.reference_id) on a.id = p.device_actions_id where dev.gtin=? AND n_a.lang_code=? AND n_p.lang_code=? ORDER BY a.command";
  } else {
    sql = "SELECT a.command, a.command, p.name, p.name, p.default_value FROM device AS d JOIN device_actions AS a JOIN device_actions_parameter AS p WHERE gtin=? AND d.id=a.device_id AND a.id=p.device_actions_id ORDER BY a.command";
  }

  // SqlStatement has broken move semantics
  SqlStatement findActions = m_db->prepare(sql);

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
    actions.back().params.push_back(ActionParameter());
    actions.back().params.back().name = findActions.getColumn<std::string>(2);
    actions.back().params.back().title = findActions.getColumn<std::string>(3);
    actions.back().params.back().defaultValue = findActions.getColumn<int>(4);
  }

  if (actions.empty()) {
    throw std::runtime_error("no actions for device: " + gtin);
  }

  return actions;
}

std::vector<VdcDb::StandardActionDesc> VdcDb::getStandardActions(const std::string &gtin, const std::string &langCode) {
  std::vector<StandardActionDesc> desc;

  std::string sql;
  if (!langCode.empty()) {
    sql = "SELECT p.name, n_p.name, a.command, ap.name, pp.value FROM device AS d INNER JOIN device_actions AS a INNER JOIN (device_actions_predefined AS p INNER JOIN name_device_actions_predefined AS n_p) INNER JOIN device_actions_predefined_parameter AS pp INNER JOIN device_actions_parameter AS ap ON d.id=a.device_id AND a.id=p.device_actions_id AND p.id=pp.device_actions_predefined_id AND pp.device_actions_parameter_id=ap.id AND p.id=n_p.reference_id WHERE gtin=? AND n_p.lang_code=? ORDER BY a.command;";
  } else {
    sql = "SELECT p.name, p.name, a.command, ap.name, pp.value FROM device AS d INNER JOIN device_actions AS a INNER JOIN device_actions_predefined AS p INNER JOIN device_actions_predefined_parameter AS pp INNER JOIN device_actions_parameter AS ap ON d.id=a.device_id AND a.id=p.device_actions_id AND p.id=pp.device_actions_predefined_id AND pp.device_actions_parameter_id=ap.id WHERE gtin=? ORDER BY a.command;";
  }

  // SqlStatement has broken move semantics
  SqlStatement find = m_db->prepare(sql);

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
    auto arg = std::make_pair(find.getColumn<std::string>(3), find.getColumn<std::string>(4));
    desc.back().args.push_back(arg);
  }

  if (desc.empty()) {
    throw std::runtime_error("no standard actions for device: " + gtin);
  }

  return desc;
}

} // dss namespace

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

__DEFINE_LOG_CHANNEL__(VdcDb, lsInfo);

// Parallel readers and writers would introduce errors returned by sqlite.
// It is possible to recover from these errors, but we don't aim for parallelism here.
// We avoid concurrency errors by locking this static mutex in each VdcDb instance.
std::mutex VdcDb::s_mutex;

VdcDb::VdcDb(DSS &dss, SQLite3::Mode mode):
    m_db(getFilePath(dss), mode),
    m_lock(s_mutex) {
}

std::string VdcDb::getFilePath(DSS &dss) {
  return dss.getDatabaseDirectory() + "/vdc.db";
}

void VdcDb::recreate(DSS &dss) {
  try {
    // Check that VdcDb is usable, (re)create if not.
    //
    // TODO(someday): refactor this initial VdcDb recreating into separate class?
    // It is independent to the database fetcher and must be active even when fetcher is inactive.
    VdcDb db(dss);
    log(std::string() + "Database present", lsNotice);
  } catch (std::exception &e) {
    try {
      VdcDb db(dss, SQLite3::Mode::ReadWrite);
      db.getDb().exec(readFile(dss.getDataDirectory() + "/vdc-db.sql"));
      log(std::string() + "Database (re)created. e.what():" + e.what(), lsNotice);
    } catch (std::exception &e2) {
      // Opening db read-write leaves empty file.
      // Delete the file so that read only open fails.
      ::remove(VdcDb::getFilePath(dss).c_str());
      throw std::runtime_error(std::string() + "Database missing or corrupt, recreate failed. e2.what():"
          + e2.what() + " e.what():" + e.what());
    }
  }
}

void VdcDb::extractLangAndCountry(const std::string &langCode, std::string &lang, std::string &country)
{
  if (langCode.length() < 2) {
    lang = "base";
    return;
  }
  if (langCode.length() >= 2) {
    lang = langCode.substr(0, 2);
  }
  if (langCode.length() >= 5) {
    country = langCode.substr(3, 2);
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

std::vector<VdcDb::SpecDesc> VdcDb::getSpec(const std::string &gtin, const std::string &langCode) {
  std::string lang, country;
  extractLangAndCountry(langCode, lang, country);

  // name, tags, gtin
  std::string sql0("select * from callGetSpecBase where (gtin=? or gtin='')");
  SqlStatement query0 = m_db.prepare(sql0);
  SqlStatement::BindScope scope0 = query0.bind(gtin);

  std::vector<SpecDesc> specs;
  while (query0.step() != SqlStatement::StepResult::DONE) {
    specs.push_back(SpecDesc());
    specs.back().name = query0.getColumn<std::string>(0);
    specs.back().title = query0.getColumn<std::string>(0);
    specs.back().tags = query0.getColumn<std::string>(1);
    specs.back().value.erase();
  }

  // name, title, tags, value, lang_code, gtin
  std::string sql = R"sqlquery(select name, title, tags, value, gtin from callGetSpec "
                               "where (gtin=? or gtin='') and (lang=?) and (country=? or country="ZZ"))sqlquery";
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang);
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    foreach (auto &it, specs) {
      if (name == it.name) {
        it.title = query.getColumn<std::string>(1);
        it.value = query.getColumn<std::string>(3);
      }
    }
  }

  return specs;
}

std::vector<VdcDb::StateDesc> VdcDb::getStates(const std::string &gtin, const std::string &langCode) {
  std::string lang, country;
  extractLangAndCountry(langCode, lang, country);

  // name, value, tags, gtin
  std::string sql0("select * from callGetStatesBase where gtin=?");
  SqlStatement query0 = m_db.prepare(sql0);
  SqlStatement::BindScope scope0 = query0.bind(gtin);

  std::vector<StateDesc> states;
  std::string cur(""); // invalid state name
  while (query0.step() == SqlStatement::StepResult::ROW) {
    std::string name = query0.getColumn<std::string>(0);
    if (states.empty() || cur != name) {
      states.push_back(StateDesc());
      states.back().name = cur = name;
      states.back().title = query0.getColumn<std::string>(0);
      states.back().tags = query0.getColumn<std::string>(2);
    }
    auto value = std::make_pair(query0.getColumn<std::string>(1), query0.getColumn<std::string>(1));
    states.back().values.push_back(value);
  }

  // name, name:1, value, name:2, tags, gtin, lang_code
  std::string sql = R"sqlquery(select distinct name,displayName,value,enumName,tags,gtin from callGetStates "
                              "where gtin=? and (lang=?) and (country=? or country="ZZ"))sqlquery";
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang, country);
  while (query.step() == SqlStatement::StepResult::ROW) {
    std::string name = query.getColumn<std::string>(0);
    std::string svalue = query.getColumn<std::string>(2);
    foreach (auto &it, states) {
      if (name == it.name) {
        it.title = query.getColumn<std::string>(1);
      }
      foreach (auto &vit, it.values) {
        if (vit.first == svalue) {
          vit.second = query.getColumn<std::string>(3);
        }
      }
    }
  }

  return states;
}

std::vector<VdcDb::EventDesc> VdcDb::getEvents(const std::string &gtin, const std::string &langCode) {
  std::string lang, country;
  extractLangAndCountry(langCode, lang, country);

  // name, gtin
  std::string sql0("select * from callGetEventsBase where gtin=?");
  SqlStatement query0 = m_db.prepare(sql0);
  SqlStatement::BindScope scope0 = query0.bind(gtin);

  std::vector<EventDesc> events;
  while (query0.step() != SqlStatement::StepResult::DONE) {
    events.push_back(EventDesc());
    events.back().name = query0.getColumn<std::string>(0);
    events.back().title = query0.getColumn<std::string>(0);
  }

  // name, displayName, gtin, lang_code
  std::string sql = R"sqlquery(select distinct name,displayName,gtin,lang,country from callGetEvents "
                              "where gtin=? and (lang=?) and (country=? or country="ZZ"))sqlquery";
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang, country);
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    foreach (auto &it, events) {
      if (name == it.name) {
        it.title = query.getColumn<std::string>(1);
      }
    }
  }

  return events;
}

std::vector<VdcDb::PropertyDesc> VdcDb::getProperties(const std::string &gtin, const std::string &langCode) {
  std::string lang, country;
  extractLangAndCountry(langCode, lang, country);

  // name, type_id, default_value, min_value, max_value, resolution, si_unit, tags, gtin
  std::string sql0("select * from callGetPropertiesBase where gtin=?");
  SqlStatement query0 = m_db.prepare(sql0);
  SqlStatement::BindScope scope0 = query0.bind(gtin);

  std::vector<PropertyDesc> props;
  while (query0.step() != SqlStatement::StepResult::DONE) {
    props.push_back(PropertyDesc());
    props.back().name = query0.getColumn<std::string>(0);
    props.back().title = query0.getColumn<std::string>(0);
    switch (query0.getColumn<int>(1)) {
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
    props.back().defaultValue = query0.getColumn<std::string>(2);
    props.back().minValue = query0.getColumn<std::string>(3);
    props.back().maxValue = query0.getColumn<std::string>(4);
    props.back().resolution = query0.getColumn<std::string>(5);
    props.back().siUnit = query0.getColumn<std::string>(6);
    props.back().tags = query0.getColumn<std::string>(7);
  }

  // name, alt_label, type_id, default_value, min_value, max_value, resolution, si_unit, tags, gtin, lang_code
  std::string sql = R"sqlquery(select distinct name, alt_label, type_id, default_value, min_value, max_value, resolution, si_unit from callGetProperties "
                              "where gtin=? and (lang=?) and (country=? or country="ZZ"))sqlquery";
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang, country);
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    foreach (auto &it, props) {
      if (name == it.name) {
        it.title = query.getColumn<std::string>(1);
      }
    }
  }

  return props;
}

std::vector<VdcDb::ActionDesc> VdcDb::getActions(const std::string &gtin, const std::string &langCode) {
  std::string lang, country;
  extractLangAndCountry(langCode, lang, country);

  // command, parameterName, type_id, default_value, min_value, max_value, resolution, si_unit, tags, gtin
  std::string sql0("select * from callGetActionsBase where gtin=?");
  SqlStatement query0 = m_db.prepare(sql0);
  SqlStatement::BindScope scope0 = query0.bind(gtin);

  std::vector<ActionDesc> actions;
  std::string cur("");
  while (query0.step() != SqlStatement::StepResult::DONE) {
    std::string name = query0.getColumn<std::string>(0);
    if (actions.empty() || cur != name) {
      actions.push_back(ActionDesc());
      actions.back().name = cur = name;
      actions.back().title = name;
    }
    if (sqlite3_column_type(query0, 1) != SQLITE_NULL) {
      actions.back().params.push_back(PropertyDesc());
      actions.back().params.back().name = query0.getColumn<std::string>(1);
      actions.back().params.back().title = query0.getColumn<std::string>(1);
      switch (query0.getColumn<int>(2)) {
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
      actions.back().params.back().defaultValue = query0.getColumn<std::string>(3);
      actions.back().params.back().minValue = query0.getColumn<std::string>(4);
      actions.back().params.back().maxValue = query0.getColumn<std::string>(5);
      actions.back().params.back().resolution = query0.getColumn<std::string>(6);
      actions.back().params.back().siUnit = query0.getColumn<std::string>(7);
      actions.back().params.back().tags = query0.getColumn<std::string>(8);
    }
  }

  // command, actionName, parameterName, parameterDisplayName, type_id, default_value, min_value, max_value, resolution, si_unit, tags, paramexists, gtin, lang_code
  std::string sql = R"sqlquery(select distinct command, actionName, parameterName, parameterDisplayName, type_id, default_value, min_value, max_value, resolution, si_unit, tags, paramexists from callGetActions "
                              "where gtin=? and (lang=?) and (country=? or country="ZZ"))sqlquery";
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang, country);
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    std::string pname;
    if (sqlite3_column_type(query, 2) != SQLITE_NULL) {
      pname = query.getColumn<std::string>(2);
    }
    foreach (auto &it, actions) {
      if (name == it.name) {
        it.title = query.getColumn<std::string>(1);
      }
      foreach (auto &pit, it.params) {
        if (pname == pit.name) {
          pit.title = query.getColumn<std::string>(3);
        }
      }
    }
  }

  return actions;
}

std::vector<VdcDb::StandardActionDesc> VdcDb::getStandardActions(const std::string &gtin, const std::string &langCode) {
  std::string lang, country;
  extractLangAndCountry(langCode, lang, country);

  // predefName, command, name:1, value, gint
  std::string sql0("select * from callGetStandardActionsBase where gtin=?");
  SqlStatement query0 = m_db.prepare(sql0);
  SqlStatement::BindScope scope0 = query0.bind(gtin);

  std::vector<StandardActionDesc> desc;
  std::string cur("");
  while (query0.step() != SqlStatement::StepResult::DONE) {
    std::string name = query0.getColumn<std::string>(0);
    if (desc.empty() || cur != name) {
      desc.push_back(StandardActionDesc());
      desc.back().name = cur = name;
      desc.back().title = query0.getColumn<std::string>(0);
      desc.back().action_name = query0.getColumn<std::string>(1);
    }
    if (sqlite3_column_type(query0, 2) != SQLITE_NULL) {
      auto arg = std::make_pair(query0.getColumn<std::string>(2), query0.getColumn<std::string>(3));
      desc.back().args.push_back(arg);
    }
  }

  // predefName, displayPredefname, command, name:2, value, paramexists, gtin, lang_code
  std::string sql = R"sqlquery(select distinct predefName, displayPredefName, command, paramName,gtin from callGetStandardActions "
                              "where gtin=? and (lang=?) and (country=? or country="ZZ"))sqlquery";
  SqlStatement query = m_db.prepare(sql);
  SqlStatement::BindScope scope = query.bind(gtin, lang, country);
  while (query.step() != SqlStatement::StepResult::DONE) {
    std::string name = query.getColumn<std::string>(0);
    foreach (auto &it, desc) {
      if (name == it.name) {
        it.title = query.getColumn<std::string>(1);
      }
    }
  }

  return desc;
}

bool VdcDb::hasActionInterface(const std::string &gtin) {
  Logger::getInstance()->log(std::string(__func__) + "check action interface for GTIN " + gtin);

  bool result = false;
  std::string sql = "SELECT name FROM device WHERE gtin=?";
  SqlStatement find = m_db.prepare(sql);
  SqlStatement::BindScope scope = find.bind(gtin);

  while (find.step() == SqlStatement::StepResult::ROW) {
    std::string name = find.getColumn<std::string>(0);
    result = true;

    Logger::getInstance()->log(std::string(__func__) + "found name for GTIN " + gtin + ": ", name);
  }

  return result;
}

} // dss namespace

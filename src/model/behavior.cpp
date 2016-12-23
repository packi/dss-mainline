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

#include "behavior.h"
#include "base.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <map>
#include <vector>

namespace dss {

using rapidjson::Document;
using rapidjson::Value;
using rapidjson::StringBuffer;
using rapidjson::Writer;

Behavior::Behavior(int configuration) : m_configuration(configuration) {}

Behavior::~Behavior() {}

DefaultBehavior::DefaultBehavior() : Behavior(0) {}

DefaultBehavior::DefaultBehavior(int configuration) : Behavior(configuration) {}

DefaultBehavior::~DefaultBehavior() {}

std::string DefaultBehavior::serializeConfiguration(int configuration) const { return "{}"; }

int DefaultBehavior::deserializeConfiguration(const std::string& jsonConfiguration) const {
  Document d;
  d.Parse(jsonConfiguration);

  // for DefaultBehavior we do not use any configuration but we check that the call itself was valid
  if (!d.IsObject()) {
    throw std::runtime_error("Error during Json parsing");
  }

  return 0;
}

int DefaultBehavior::getNextScene(int currentScene) { return SceneHelper::getNextScene(currentScene); }

int DefaultBehavior::getPreviousScene(int currentScene) { return SceneHelper::getPreviousScene(currentScene); }

// By default the ventilation configuration activates all basic scenes (0 value is active)
VentilationBehavior::VentilationBehavior() : Behavior(0) {}

VentilationBehavior::VentilationBehavior(int configuration) : Behavior(configuration) {}

VentilationBehavior::~VentilationBehavior() {}

std::string VentilationBehavior::serializeConfiguration(int configuration) const {
  // mapping bit offsets to sceneId
  const std::vector<int> sceneIds = {SceneOff, Scene1, Scene2, Scene3, Scene4};

  Document d;
  d.SetObject();

  Value activeBasicScenes;
  activeBasicScenes.SetArray();

  for (unsigned int i = 0; i < sceneIds.size(); ++i) {
    if ((configuration & (1 << i)) == 0) {
      activeBasicScenes.PushBack(Value().SetInt(sceneIds.at(i)), d.GetAllocator());
    }
  }
  d.AddMember("activeBasicScenes", activeBasicScenes, d.GetAllocator());

  StringBuffer sb;
  Writer<StringBuffer> writer(sb);
  d.Accept(writer);

  return sb.GetString();
}

int VentilationBehavior::deserializeConfiguration(const std::string& jsonConfiguration) const {
  // mapping sceneId to bit offset
  const std::map<int, int> sceneIds = {{SceneOff, 0}, {Scene1, 1}, {Scene2, 2}, {Scene3, 3}, {Scene4, 4}};
  int configuration = 0;

  Document d;
  d.Parse(jsonConfiguration);

  if (!d.IsObject()) {
    throw std::runtime_error("Error during Json parsing");
  }

  // if activeBasicScenes array is provided use it to set configuration, if not use default configuration value
  if (d.HasMember("activeBasicScenes")) {
    const auto& activeBasicScenes = d["activeBasicScenes"];
    if (!activeBasicScenes.IsArray()) {
      throw std::runtime_error("No valid 'activeBasicScenes' array found");
    }

    // we set all scenes as inactive, and mark only the active ones
    configuration = 0x1F;

    for (rapidjson::SizeType i = 0; i < activeBasicScenes.Size(); i++) {
      int basicSceneId = activeBasicScenes[i].GetInt();
      if (sceneIds.find(basicSceneId) != sceneIds.end()) {
        configuration &= ~(1 << sceneIds.at(basicSceneId));
      } else {
        throw std::runtime_error(ds::str("Basic SceneId:", basicSceneId, " not valid (should be ", (int)SceneOff, ", ",
            (int)Scene1, ", ", (int)Scene2, ", ", (int)Scene3, " or ", (int)Scene4, ")"));
      }
    }
  }

  return configuration;
}

int VentilationBehavior::getNextScene(int currentScene) {
  // TODO(soon): Add implementation of Ventilation specific state machine
  throw std::runtime_error("Not implemented");
}

int VentilationBehavior::getPreviousScene(int currentScene) {
  // TODO(soon): Add implementation of Ventilation specific state machine
  throw std::runtime_error("Not implemented");
}

} /* namespace dss */

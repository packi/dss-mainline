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

#include "foreach.h"

namespace dss {

using rapidjson::Document;
using rapidjson::Value;
using rapidjson::StringBuffer;
using rapidjson::Writer;

Behavior::Behavior(PropertyNodePtr& propertyNode, int currentScene, int configuration)
    : m_pPropertyNode(propertyNode), m_currentScene(currentScene), m_configuration(configuration) {}

Behavior::~Behavior() {}

DefaultBehavior::DefaultBehavior(PropertyNodePtr& propertyNode, int currentScene)
    : Behavior(propertyNode, currentScene, 0) {}

DefaultBehavior::~DefaultBehavior() {}

void DefaultBehavior::serializeConfiguration(uint32_t configuration, JSONWriter& writer) const {
  // do nothing - leave empty JSON
}

uint32_t DefaultBehavior::deserializeConfiguration(const std::string& jsonConfiguration) const {
  Document d;
  d.Parse(jsonConfiguration.c_str());

  // for DefaultBehavior we do not use any configuration but we check that the call itself was valid
  if (!d.IsObject()) {
    throw std::runtime_error("Error during Json parsing");
  }

  return 0u;
}

int DefaultBehavior::getNextScene() { return SceneHelper::getNextScene(m_currentScene); }

int DefaultBehavior::getPreviousScene() { return SceneHelper::getPreviousScene(m_currentScene); }

std::vector<int> DefaultBehavior::getAvailableScenes() {
  return SceneHelper::getAvailableScenes();
}

void DefaultBehavior::publishToPropertyTree() {}

void DefaultBehavior::removeFromPropertyTree() {}

// mapping bit offsets to sceneId
const std::vector<int> VentilationBehavior::offsetToSceneId = {SceneOff, Scene1, Scene2, Scene3, Scene4, SceneBoost};

// By default the ventilation configuration activates all basic scenes (0 value is active)
VentilationBehavior::VentilationBehavior(PropertyNodePtr& propertyNode, int currentScene)
    : Behavior(propertyNode, SceneOff, 0) {
  // make sure the current scene is valid
  setCurrentScene(currentScene);
  publishToPropertyTree();
}

VentilationBehavior::~VentilationBehavior() { removeFromPropertyTree(); }

void VentilationBehavior::setCurrentScene(int currentScene) {
  auto&& activeScenes = getActiveBasicScenes();

  // if no scenes are allowed we force off state
  if (activeScenes.size() == 0) {
    m_currentScene = SceneOff;
  } else {
    // if this scene is an allowed basic scene just set it
    if (find(activeScenes.begin(), activeScenes.end(), currentScene) != activeScenes.end()) {
      m_currentScene = currentScene;
    } else {
      switch (currentScene) {
        case SceneFire:
        case SceneSmoke:
        case SceneGas:
          m_currentScene = getActiveBasicScenes().front();
          break;
        default:
          // ignore all other scenes as they do not change ventilation level
          break;
      }
    }
  }
}

void VentilationBehavior::serializeConfiguration(uint32_t configuration, JSONWriter& writer) const {
  writer.startArray("activeBasicScenes");
  foreach (auto scene, getActiveBasicScenes()) { writer.add(scene); }
  writer.endArray();
}

uint32_t VentilationBehavior::deserializeConfiguration(const std::string& jsonConfiguration) const {
  uint32_t configuration = 0u;

  Document d;
  d.Parse(jsonConfiguration.c_str());

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
    configuration = 0x3F;

    for (rapidjson::SizeType i = 0; i < activeBasicScenes.Size(); i++) {
      int basicSceneId = activeBasicScenes[i].GetInt();
      auto sceneIt = find(offsetToSceneId.begin(), offsetToSceneId.end(), basicSceneId);

      if (sceneIt != offsetToSceneId.end()) {
        configuration &= ~(1 << (sceneIt - offsetToSceneId.begin()));
      } else {
        throw std::runtime_error(ds::str("Basic SceneId:", basicSceneId, " not valid (should be ", (int)SceneOff, ", ",
            (int)Scene1, ", ", (int)Scene2, ", ", (int)Scene3, ", ", (int)Scene4, " or ", (int)SceneBoost, ")"));
      }
    }
  }

  return configuration;
}

int VentilationBehavior::getNextScene() {
  auto&& activeScenes = getActiveBasicScenes();

  if (activeScenes.size() == 0) {
    // if no scenes are active we cannot find next scene
    throw std::runtime_error("Could not find next active scene");
  } else {
    auto sceneIt = find(activeScenes.begin(), activeScenes.end(), m_currentScene);

    // if we could not find current scene in allowed scenes, or this is already the biggest possible return it
    if ((sceneIt == activeScenes.end()) || (m_currentScene == activeScenes.back())) {
      return activeScenes.back();
    } else {
      return *(++sceneIt);
    }
  }
}

int VentilationBehavior::getPreviousScene() {
  auto&& activeScenes = getActiveBasicScenes();

  if (activeScenes.size() == 0) {
    // if no scenes are active we cannot find previous scene
    throw std::runtime_error("Could not find previous active scene");
  } else {
    auto sceneIt = find(activeScenes.begin(), activeScenes.end(), m_currentScene);

    // if we could not find current scene in allowed scenes, or this is already the lowest possible return it
    if ((sceneIt == activeScenes.end()) || (m_currentScene == activeScenes.front())) {
      return activeScenes.front();
    } else {
      return *(--sceneIt);
    }
  }
}

std::string VentilationBehavior::getPropertyActiveBasicScenes() const {
  std::string ret = "";
  bool first = true;

  foreach (auto scene, getActiveBasicScenes()) {
    if (first) {
      first = false;
    } else {
      ret.append(",");
    }
    ret.append(ds::str(scene));
  }

  return ret;
}

std::vector<int> VentilationBehavior::getActiveBasicScenes() const {
  std::vector<int> ret;

  for (unsigned int i = 0; i < offsetToSceneId.size(); ++i) {
    if ((m_configuration & (1 << i)) == 0) {
      ret.push_back(offsetToSceneId.at(i));
    }
  }

  return ret;
}

std::vector<int> VentilationBehavior::getAvailableScenes() {
  return getActiveBasicScenes();
}

void VentilationBehavior::publishToPropertyTree() {
  if (m_pPropertyNode != NULL) {
    m_pPropertyNode->createProperty("activeBasicScenes")
        ->linkToProxy(PropertyProxyMemberFunction<VentilationBehavior, std::string, false>(
            *this, &VentilationBehavior::getPropertyActiveBasicScenes));
    m_pPropertyNode->createProperty("lastCalledBasicScene")
        ->linkToProxy(
            PropertyProxyMemberFunction<VentilationBehavior, int>(*this, &VentilationBehavior::getCurrentScene));
  }
}

void VentilationBehavior::removeFromPropertyTree() {
  if (m_pPropertyNode != NULL) {
    auto&& childNode = m_pPropertyNode->getProperty("activeBasicScenes");
    if (childNode != NULL) {
      m_pPropertyNode->removeChild(childNode);
    }
    childNode = m_pPropertyNode->getProperty("lastCalledBasicScene");
    if (childNode != NULL) {
      m_pPropertyNode->removeChild(childNode);
    }
  }
}

} /* namespace dss */

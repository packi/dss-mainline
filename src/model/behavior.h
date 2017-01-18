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

#pragma once

#include <string>
#include "scenehelper.h"
#include <ds/str.h>
#include <web/webrequests.h>
#include <propertysystem.h>

namespace dss {

class Behavior {
protected:
  uint32_t m_configuration;
  PropertyNodePtr& m_pPropertyNode;
public:
  Behavior(PropertyNodePtr& propertyNode, int configuration);
  virtual ~Behavior();

  uint32_t getConfiguration() const { return m_configuration; }
  void setConfiguration(uint32_t configuration) { m_configuration = configuration; }

  virtual void serializeConfiguration(uint32_t configuration, JSONWriter& writer) const = 0;
  virtual uint32_t deserializeConfiguration(const std::string& jsonConfiguration) const = 0;

  virtual int getNextScene(int currentScene) = 0;
  virtual int getPreviousScene(int currentScene) = 0;

  virtual void publishToPropertyTree() = 0;
  virtual void removeFromPropertyTree() = 0;
};

class DefaultBehavior : public Behavior {
public:
  DefaultBehavior(PropertyNodePtr& propertyNode);
  DefaultBehavior(PropertyNodePtr& propertyNode, int configuration);
  ~DefaultBehavior();

  void serializeConfiguration(uint32_t configuration, JSONWriter& writer) const DS_OVERRIDE;
  uint32_t deserializeConfiguration(const std::string& jsonConfiguration) const DS_OVERRIDE;

  int getNextScene(int currentScene) DS_OVERRIDE;
  int getPreviousScene(int currentScene) DS_OVERRIDE;

  void publishToPropertyTree() DS_OVERRIDE;
  void removeFromPropertyTree() DS_OVERRIDE;
};

class VentilationBehavior : public Behavior {
private:
  static const std::vector<int> offsetToSceneId;
  static const std::map<int, int> sceneIdTooffset;

  std::string getActiveBasicScenes() const;
public:
  VentilationBehavior(PropertyNodePtr& propertyNode);
  VentilationBehavior(PropertyNodePtr& propertyNode, int configuration);
  ~VentilationBehavior();

  void serializeConfiguration(uint32_t configuration, JSONWriter& writer) const DS_OVERRIDE;
  uint32_t deserializeConfiguration(const std::string& jsonConfiguration) const DS_OVERRIDE;

  int getNextScene(int currentScene) DS_OVERRIDE;
  int getPreviousScene(int currentScene) DS_OVERRIDE;

  void publishToPropertyTree() DS_OVERRIDE;
  void removeFromPropertyTree() DS_OVERRIDE;
};

} /* namespace dss */

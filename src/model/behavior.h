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

namespace dss {

class Behavior {
private:
  uint32_t m_configuration;

public:
  Behavior(int configuration);
  virtual ~Behavior();

  uint32_t getConfiguration() { return m_configuration; }
  void setConfiguration(uint32_t configuration) { m_configuration = configuration; }

  virtual void serializeConfiguration(uint32_t configuration, JSONWriter& writer) const = 0;
  virtual uint32_t deserializeConfiguration(const std::string& jsonConfiguration) const = 0;

  virtual int getNextScene(int currentScene) = 0;
  virtual int getPreviousScene(int currentScene) = 0;
};

class DefaultBehavior : public Behavior {
public:
  DefaultBehavior();
  DefaultBehavior(int configuration);
  ~DefaultBehavior();

  void serializeConfiguration(uint32_t configuration, JSONWriter& writer) const DS_OVERRIDE;
  uint32_t deserializeConfiguration(const std::string& jsonConfiguration) const DS_OVERRIDE;

  int getNextScene(int currentScene) DS_OVERRIDE;
  int getPreviousScene(int currentScene) DS_OVERRIDE;
};

class VentilationBehavior : public Behavior {
public:
  VentilationBehavior();
  VentilationBehavior(int configuration);
  ~VentilationBehavior();

  void serializeConfiguration(uint32_t configuration, JSONWriter& writer) const DS_OVERRIDE;
  uint32_t deserializeConfiguration(const std::string& jsonConfiguration) const DS_OVERRIDE;

  int getNextScene(int currentScene) DS_OVERRIDE;
  int getPreviousScene(int currentScene) DS_OVERRIDE;
};

} /* namespace dss */

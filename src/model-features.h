/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey Jin Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifndef __DSS_MODEL_FEATURES_H__
#define __DSS_MODEL_FEATURES_H__

#include "config.h"

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace dss {

/* documentation:
   http://redmine.digitalstrom.org/attachments/download/6076/model-features.pdf
   http://redmine.digitalstrom.org/projects/dss/wiki/Model_Features
 */

enum class ModelFeatureId {
  dontcare,
  blink,
  ledauto,
  leddark,
  transt,
  outmode,
  outmodeswitch,
  outvalue8,
  pushbutton,
  pushbdevice,
  pushbsensor,
  pushbarea,
  pushbadvanced,
  pushbcombined,
  shadeprops,
  shadeposition,
  motiontimefins,
  optypeconfig,
  shadebladeang,
  highlevel,
  consumption,
  jokerconfig,
  akmsensor,
  akminput,
  akmdelay,
  twowayconfig,
  outputchannels,
  heatinggroup,
  heatingoutmode,
  heatingprops,
  pwmvalue,
  valvetype,
  extradimmer,
  umvrelay,
  blinkconfig,
  umroutmode,
  locationconfig,
  windprotectionconfigawning,
  windprotectionconfigblind,
  impulseconfig,
  outmodegeneric,
  outconfigswitch,
  temperatureoffset
};

class ModelFeatures
{
public:
  static ModelFeatures* createInstance();
  static ModelFeatures* getInstance();
  void setFeatures(int _color, std::string model, boost::shared_ptr<std::vector<ModelFeatureId> > _features);
  std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<ModelFeatureId> > > > > getFeatures(int _color);
  std::string getFeatureName(ModelFeatureId feature);
  ModelFeatureId nameToFeature(std::string _name);
  std::string colorToString(int _color);
  boost::shared_ptr<std::vector<ModelFeatureId> > getAvailableFeatures();
private:
  ModelFeatures();
  static ModelFeatures *m_instance;
  // color vector, each index represents one color according to definitions of
  // modelconst.h
  std::vector<std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<ModelFeatureId> > > > > > m_features;
  boost::mutex m_lock;
};

}; // namespace

#endif//__DSS_MODEL_FEATURES_H__

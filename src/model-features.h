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
  dontcare = 0,
  blink = 1,
  ledauto = 2,
  leddark = 3,
  transt = 4,
  outmode = 5,
  outmodeswitch = 6,
  outvalue8 = 7,
  pushbutton = 8,
  pushbdevice = 9,
  pushbsensor = 10,
  pushbarea = 11,
  pushbadvanced = 12,
  pushbcombined = 13,
  shadeprops = 14,
  shadeposition = 15,
  motiontimefins = 16,
  optypeconfig = 17,
  shadebladeang = 18,
  highlevel = 19,
  consumption = 20,
  jokerconfig = 21,
  akmsensor = 22,
  akminput = 23,
  akmdelay = 24,
  twowayconfig = 25,
  outputchannels = 26,
  heatinggroup = 27,
  heatingoutmode = 28,
  heatingprops = 29,
  pwmvalue = 30,
  valvetype = 31,
  extradimmer = 32,
  umvrelay = 33,
  blinkconfig = 34,
  umroutmode = 35,
  locationconfig = 36,
  windprotectionconfigawning = 37,
  windprotectionconfigblind = 38,
  impulseconfig = 39,
  outmodegeneric = 40,
  outconfigswitch = 41,
  temperatureoffset = 42,
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
  static boost::shared_ptr<std::vector<ModelFeatureId> > getAvailableFeatures();
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

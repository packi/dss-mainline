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

#include "model/modelconst.h"

namespace dss {

enum {
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_leddark,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_pushbcombined,
  mf_shadeprops,
  mf_shadeposition,
  mf_motiontimefins,
  mf_optypeconfig,
  mf_shadebladeang,
  mf_highlevel,
  mf_consumption,
  mf_jokerconfig,
  mf_akmsensor,
  mf_akminput,
  mf_akmdelay,
  mf_twowayconfig,
  mf_outputchannels,
  mf_heatinggroup,
  mf_heatingoutmode,
  mf_heatingprops,
  mf_pwmvalue,
  mf_valvetype,
  mf_extradimmer
};

class ModelFeatures
{
public:
  static ModelFeatures* createInstance();
  static ModelFeatures* getInstance();
  void setFeatures(int _color, std::string model, boost::shared_ptr<std::vector<int> > _features);
  std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > > > getFeatures(int _color);
  std::string getFeatureName(int _feature);
  int nameToFeature(std::string _name);
  std::string colorToString(int _color);
  boost::shared_ptr<std::vector<int> > getAvailableFeatures();
private:
  ModelFeatures();
  static ModelFeatures *m_instance;
  // color vector, each index represents one color according to definitions of
  // modelconst.h
  std::vector<std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > > > > m_features;
  boost::mutex m_lock;
};

}; // namespace

#endif//__DSS_MODEL_FEATURES_H__

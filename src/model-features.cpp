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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "model/modelconst.h"
#include "model-features.h"

#define MF_ARRAY_SIZE(mf) mf + (sizeof(mf) / sizeof(mf[0]))

namespace dss {

ModelFeatures * ModelFeatures::m_instance = NULL;

// model strings
const char *KM220 =     "KM:220";
const char *KM200 =     "KM:200";
const char *KM2 =       "KM:2"; // wildcard for all KM-2*
const char *KL200 =     "KL:200";
const char *KL220 =     "KL:220";
const char *KL230 =     "KL:230";
const char *KL2 =       "KL:2"; // wildcard for all KL-2*
const char *TKM210 =    "TKM:210";
const char *TKM200 =    "TKM:200";
const char *TKM2 =      "TKM:2"; // wildcard for all TKM-2*
const char *SDM200 =    "SDM:200";
const char *SDM2 =      "SDM:2"; // wildcard for all SDM-2*
const char *SDS210 =    "SDS:210";
const char *SDS20 =     "SDS:20"; // wildcard for SDS-20*
const char *SDS22 =     "SDS:22"; // wildcard for SDS-22*
const char *SDS2 =      "SDS:2"; // wildcard for all SDS-2*
const char *ZWS2 =      "ZWS:2"; // wildcard for all ZWS-2*
const char *UMV204 =    "UMV:204";
const char *UMV200 =    "UMV:200";
const char *UMR200 =    "UMR:200";
const char *AKM2 =      "AKM:2"; // wildcard for all AKM-2*

// all available features
const int MF_AVAILABLE[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_leddark,
  mf_transt,
  mf_outmode,
  mf_outmodeswitch,
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
  mf_extradimmer,
  mf_umvrelay,
  mf_blinkconfig,
  mf_umroutmode,
  mf_pushbsensor
};

// model features
const int MF_GE_KM220[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_KM2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_KL200[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmodeswitch,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_TKM210[] =
{
  mf_dontcare,
  mf_blink,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_TKM200[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_SDM200[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_SDS210[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_twowayconfig
};

const int MF_GE_SDS20[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_pushbcombined,
  mf_twowayconfig
};

const int MF_GE_SDS22[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_pushbcombined,
  mf_twowayconfig
};

const int MF_GE_SDS2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_twowayconfig
};


const int MF_GE_ZWS2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_UMV204[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_GE_UMV200[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_extradimmer,
  mf_umvrelay
};

const int MF_GN_KM2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8
};

const int MF_GN_TKM2[] =
{
  mf_dontcare,
  mf_blink,
  mf_transt,
  mf_outmode,
  mf_outvalue8
};

const int MF_RT_KM2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outmode,
  mf_outvalue8
};

const int MF_RT_TKM2[] =
{
  mf_dontcare,
  mf_blink,
  mf_transt,
  mf_outmode,
  mf_outvalue8
};

const int MF_RT_SDM2[] =
{
  mf_blink,
  mf_leddark,
  mf_outmode,
  mf_outvalue8
};

const int MF_GR_KL220[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_shadeprops,
  mf_shadeposition,
  mf_motiontimefins,
  mf_shadebladeang,
  mf_locationconfig,
  mf_windprotectionconfig
};

const int MF_GR_KL230[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_shadeprops,
  mf_shadeposition,
  mf_motiontimefins,
  mf_shadebladeang,
  mf_locationconfig,
  mf_windprotectionconfig
};

const int MF_GR_KL2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_shadeprops,
  mf_shadeposition,
  mf_locationconfig,
  mf_windprotectionconfig
};

const int MF_GR_TKM2[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_BL_KM200[] =
{
  mf_dontcare,
  mf_ledauto,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbadvanced,
  mf_heatinggroup,
  mf_heatingoutmode,
  mf_heatingprops,
  mf_pwmvalue,
  mf_valvetype
};

const int MF_BL_SDS200[] =
{
  mf_dontcare,
  mf_ledauto,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbadvanced,
  mf_heatinggroup,
  mf_heatingoutmode,
  mf_heatingprops,
  mf_pwmvalue,
  mf_valvetype
};

const int MF_BL_KL2[] =
{
  mf_dontcare,
  mf_ledauto
};

const int MF_TK_TKM2[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea
};

const int MF_GE_TKM2[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea,
  mf_pushbadvanced
};

const int MF_SW_TKM2[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_highlevel,
  mf_jokerconfig,
  mf_twowayconfig
};

const int MF_SW_KL2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_optypeconfig,
  mf_highlevel,
  mf_consumption,
  mf_jokerconfig
};

const int MF_SW_ZWS2[] =
{
  mf_dontcare,
  mf_blink,
  mf_ledauto,
  mf_transt,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbdevice,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_optypeconfig,
  mf_highlevel,
  mf_consumption,
  mf_jokerconfig
};

const int MF_SW_SDS20[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_highlevel,
  mf_jokerconfig,
  mf_twowayconfig
};

const int MF_SW_SDS22[] =
{
  mf_blink,
  mf_leddark,
  mf_pushbutton,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_highlevel,
  mf_jokerconfig,
  mf_twowayconfig
};

const int MF_SW_AKM2[] =
{
  mf_akmsensor,
  mf_akminput,
  mf_akmdelay
};

const int MF_SW_UMR200[] =
{
  mf_blink,
  mf_dontcare,
  mf_outvalue8,
  mf_pushbutton,
  mf_pushbsensor,
  mf_pushbarea,
  mf_pushbadvanced,
  mf_highlevel,
  mf_jokerconfig,
  mf_twowayconfig,
  mf_akminput,
  mf_akmsensor,
  mf_akmdelay,
  mf_blinkconfig,
  mf_umroutmode
};

ModelFeatures* ModelFeatures::createInstance()
{
    if (m_instance == NULL)
    {
        m_instance = new ModelFeatures();
        if (m_instance == NULL)
        {
            throw std::runtime_error("could not create ModelFeatures class");
        }
    }

    return m_instance;
}

ModelFeatures* ModelFeatures::getInstance()
{
    if (m_instance == NULL)
    {
        throw std::runtime_error("ModelFeatures not initialized");
    }

    return m_instance;
}

ModelFeatures::ModelFeatures() : m_features(ColorIDBlack + 1) {
  // initialize "our" devices
  boost::shared_ptr<std::vector<int> > fv;
  fv = boost::make_shared<std::vector<int> >();

  fv->assign(MF_GE_KM220, MF_ARRAY_SIZE(MF_GE_KM220));
  setFeatures(ColorIDYellow, KM220, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_KM2, MF_ARRAY_SIZE(MF_GE_KM2));
  setFeatures(ColorIDYellow, KM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_KL200, MF_ARRAY_SIZE(MF_GE_KL200));
  setFeatures(ColorIDYellow, KL200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_TKM210, MF_ARRAY_SIZE(MF_GE_TKM210));
  setFeatures(ColorIDYellow, TKM210, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_TKM200, MF_ARRAY_SIZE(MF_GE_TKM200));
  setFeatures(ColorIDYellow, TKM200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_SDM200, MF_ARRAY_SIZE(MF_GE_SDM200));
  setFeatures(ColorIDYellow, SDM200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_SDS210, MF_ARRAY_SIZE(MF_GE_SDS210));
  setFeatures(ColorIDYellow, SDS210, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_SDS20, MF_ARRAY_SIZE(MF_GE_SDS20));
  setFeatures(ColorIDYellow, SDS20, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_SDS22, MF_ARRAY_SIZE(MF_GE_SDS22));
  setFeatures(ColorIDYellow, SDS22, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_SDS2, MF_ARRAY_SIZE(MF_GE_SDS2));
  setFeatures(ColorIDYellow, SDS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_ZWS2, MF_ARRAY_SIZE(MF_GE_ZWS2));
  setFeatures(ColorIDYellow, ZWS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_UMV204, MF_ARRAY_SIZE(MF_GE_UMV204));
  setFeatures(ColorIDYellow, UMV204, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_UMV200, MF_ARRAY_SIZE(MF_GE_UMV200));
  setFeatures(ColorIDYellow, UMV200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GN_KM2, MF_ARRAY_SIZE(MF_GN_KM2));
  setFeatures(ColorIDGreen, KM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GN_TKM2, MF_ARRAY_SIZE(MF_GN_TKM2));
  setFeatures(ColorIDGreen, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_RT_KM2, MF_ARRAY_SIZE(MF_RT_KM2));
  setFeatures(ColorIDRed, KM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_RT_TKM2, MF_ARRAY_SIZE(MF_RT_TKM2));
  setFeatures(ColorIDRed, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_RT_SDM2, MF_ARRAY_SIZE(MF_RT_SDM2));
  setFeatures(ColorIDRed, SDM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GR_KL220, MF_ARRAY_SIZE(MF_GR_KL220));
  setFeatures(ColorIDGray, KL220, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GR_KL230, MF_ARRAY_SIZE(MF_GR_KL230));
  setFeatures(ColorIDGray, KL230, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GR_KL2, MF_ARRAY_SIZE(MF_GR_KL2));
  setFeatures(ColorIDGray, KL2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GR_TKM2, MF_ARRAY_SIZE(MF_GR_TKM2));
  setFeatures(ColorIDGray, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_BL_KM200, MF_ARRAY_SIZE(MF_BL_KM200));
  setFeatures(ColorIDBlue, KM200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_BL_SDS200, MF_ARRAY_SIZE(MF_BL_SDS200));
  setFeatures(ColorIDBlue, SDS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_BL_KL2, MF_ARRAY_SIZE(MF_BL_KL2));
  setFeatures(ColorIDBlue, KL2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_GE_TKM2, MF_ARRAY_SIZE(MF_GE_TKM2));
  setFeatures(ColorIDYellow, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_TK_TKM2, MF_ARRAY_SIZE(MF_TK_TKM2));
  setFeatures(ColorIDCyan, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_TKM2, MF_ARRAY_SIZE(MF_SW_TKM2));
  setFeatures(ColorIDBlack, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_KL2, MF_ARRAY_SIZE(MF_SW_KL2));
  setFeatures(ColorIDBlack, KL2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_ZWS2, MF_ARRAY_SIZE(MF_SW_ZWS2));
  setFeatures(ColorIDBlack, ZWS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_SDS20, MF_ARRAY_SIZE(MF_SW_SDS20));
  setFeatures(ColorIDBlack, SDS20, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_SDS22, MF_ARRAY_SIZE(MF_SW_SDS22));
  setFeatures(ColorIDBlack, SDS22, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_AKM2, MF_ARRAY_SIZE(MF_SW_AKM2));
  setFeatures(ColorIDBlack, AKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<int> >();
  fv->assign(MF_SW_UMR200, MF_ARRAY_SIZE(MF_SW_UMR200));
  setFeatures(ColorIDBlack, UMR200, fv);
  fv.reset();
}

void ModelFeatures::setFeatures(int _color, std::string _model, boost::shared_ptr<std::vector<int> > _features) {
  if (_model.empty()) {
    throw std::runtime_error("can't add model features: missing model key");
  }

  if (_features == NULL) {
    throw std::runtime_error("invalid feature array\n");
  }

  if ((_color < ColorIDYellow) || (_color > ColorIDBlack)) {
    throw std::runtime_error("can not save feature: unsupported device color");
  }
  boost::mutex::scoped_lock lock(m_lock);

  for (size_t i = 0; i < m_features.at(_color).size(); i++) {
    if (m_features.at(_color).at(i)->first == _model) {
      boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > > content = m_features.at(_color).at(i);
      return;
    }
  }

  m_features.at(_color).push_back(boost::make_shared<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > >(_model, _features));
}

std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > > > ModelFeatures::getFeatures(int _color) {
  if ((_color < ColorIDYellow) || (_color > ColorIDBlack)) {
    throw std::runtime_error("can not retrieve features: unsupported device color");
  }

  boost::mutex::scoped_lock lock(m_lock);

  if (m_features.size() == 0) {
    throw std::runtime_error("zero sized features vector!");
  }

  return m_features.at(_color);
}

int ModelFeatures::nameToFeature(std::string _name) {
  if (_name == "dontcare") {
    return mf_dontcare;
  } else if (_name == "blink") {
    return mf_blink;
  } else if (_name == "ledauto") {
    return mf_ledauto;
  } else if (_name == "leddark") {
    return mf_leddark;
  } else if (_name == "transt") {
    return mf_transt;
  } else if (_name == "outmode") {
    return mf_outmode;
  } else if (_name == "outmodeswitch") {
    return mf_outmodeswitch;
  } else if (_name == "outvalue8") {
    return mf_outvalue8;
  } else if (_name == "pushbutton") {
    return mf_pushbutton;
  } else if (_name == "pushbdevice") {
    return mf_pushbdevice;
  } else if (_name == "pushbarea") {
    return mf_pushbarea;
  } else if (_name == "pushbadvanced") {
    return mf_pushbadvanced;
  } else if (_name == "pushbcombined") {
    return mf_pushbcombined;
  } else if (_name == "shadeprops") {
    return mf_shadeprops;
  } else if (_name == "shadeposition") {
    return mf_shadeposition;
  } else if (_name == "motiontimefins") {
    return mf_motiontimefins;
  } else if (_name == "optypeconfig") {
    return mf_optypeconfig;
  } else if (_name == "shadebladeang") {
    return mf_shadebladeang;
  } else if (_name == "highlevel") {
    return mf_highlevel;
  } else if (_name == "consumption") {
    return mf_consumption;
  } else if (_name == "jokerconfig") {
    return mf_jokerconfig;
  } else if (_name == "akmsensor") {
    return mf_akmsensor;
  } else if (_name == "akminput") {
    return mf_akminput;
  } else if (_name == "akmdelay") {
    return mf_akmdelay;
  } else if (_name == "twowayconfig") {
    return mf_twowayconfig;
  } else if (_name == "outputchannels") {
    return mf_outputchannels;
  } else if (_name == "heatinggroup") {
    return mf_heatinggroup;
  } else if (_name == "heatingoutmode") {
    return mf_heatingoutmode;
  } else if (_name == "heatingprops") {
    return mf_heatingprops;
  } else if (_name == "pwmvalue") {
    return mf_pwmvalue;
  } else if (_name == "valvetype") {
    return mf_valvetype;
  } else if (_name == "extradimmer") {
    return mf_extradimmer;
  } else if (_name == "umvrelay") {
    return mf_umvrelay;
  } else if (_name == "blinkconfig") {
    return mf_blinkconfig;
  } else if (_name == "umroutmode") {
    return mf_umroutmode;
  } else if (_name == "pushbsensor") {
    return mf_pushbsensor;
  } else if (_name == "locationconfig") {
    return mf_locationconfig;
  } else if (_name == "windprotectionconfig") {
    return mf_windprotectionconfig;
  }

  throw std::runtime_error("unknown feature encountered");
}

std::string ModelFeatures::getFeatureName(int _feature)
{
  switch (_feature)
  {
    case mf_dontcare:
      return "dontcare";
    case mf_blink:
      return "blink";
    case mf_ledauto:
      return "ledauto";
    case mf_leddark:
      return "leddark";
    case mf_transt:
      return "transt";
    case mf_outmode:
      return "outmode";
    case mf_outmodeswitch:
      return "outmodeswitch";
    case mf_outvalue8:
      return "outvalue8";
    case mf_pushbutton:
      return "pushbutton";
    case mf_pushbdevice:
      return "pushbdevice";
    case mf_pushbarea:
      return "pushbarea";
    case mf_pushbadvanced:
      return "pushbadvanced";
    case mf_pushbcombined:
      return "pushbcombined";
    case mf_shadeprops:
      return "shadeprops";
    case mf_shadeposition:
      return "shadeposition";
    case mf_motiontimefins:
      return "motiontimefins";
    case mf_optypeconfig:
      return "optypeconfig";
    case mf_shadebladeang:
      return "shadebladeang";
    case mf_highlevel:
      return "highlevel";
    case mf_consumption:
      return "consumption";
    case mf_jokerconfig:
      return "jokerconfig";
    case mf_akmsensor:
      return "akmsensor";
    case mf_akminput:
      return "akminput";
    case mf_akmdelay:
      return "akmdelay";
    case mf_twowayconfig:
      return "twowayconfig";
    case mf_outputchannels:
      return "outputchannels";
    case mf_heatinggroup:
      return "heatinggroup";
    case mf_heatingoutmode:
      return "heatingoutmode";
    case mf_heatingprops:
      return "heatingprops";
    case mf_pwmvalue:
      return "pwmvalue";
    case mf_valvetype:
      return "valvetype";
    case mf_extradimmer:
      return "extradimmer";
    case mf_umvrelay:
      return "umvrelay";
    case mf_blinkconfig:
      return "blinkconfig";
    case mf_umroutmode:
      return "umroutmode";
    case mf_pushbsensor:
      return "pushbsensor";
    case mf_locationconfig:
      return "locationconfig";
    case mf_windprotectionconfig:
      return "windprotectionconfig";
    default:
      break;
  }

  throw std::runtime_error("unknown feature encountered");
}

std::string ModelFeatures::colorToString(int _color)
{
  switch (_color)
  {
    case ColorIDYellow:
      return "GE";
    case ColorIDGray:
      return "GR";
    case ColorIDBlue:
      return "BL";
    case ColorIDCyan:
      return "TK";
    case ColorIDViolet:
      return "MG";
    case ColorIDRed:
      return "RT";
    case ColorIDGreen:
      return "GN";
    case ColorIDBlack:
      return "SW";
    default:
      break;
  }

  throw std::runtime_error("unknown color");
}

boost::shared_ptr<std::vector<int> > ModelFeatures::getAvailableFeatures()
{
  boost::shared_ptr<std::vector<int> > ret;
  ret = boost::make_shared<std::vector<int> >();
  ret->assign(MF_AVAILABLE, MF_ARRAY_SIZE(MF_AVAILABLE));
  return ret;
}

}; // namespace


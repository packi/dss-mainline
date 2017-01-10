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

#include "base.h"
#include "model/modelconst.h"
#include "model-features.h"

#define ARRAY_END(mf) (mf + ARRAY_SIZE(mf))

namespace dss {

ModelFeatures * ModelFeatures::m_instance = NULL;

// model strings
const char *KM220 =     "KM:220";
const char *KM200 =     "KM:200";
const char *KM2 =       "KM:2"; // wildcard for all KM-2*
const char *KL200 =     "KL:200";
const char *KL210 =     "KL:210";
const char *KL220 =     "KL:220";
const char *KL230 =     "KL:230";
const char *KL2 =       "KL:2"; // wildcard for all KL-2*
const char *TKM210 =    "TKM:210";
const char *TKM220 =    "TKM:220";
const char *TKM230 =    "TKM:230";
const char *TKM2 =      "TKM:2"; // wildcard for all TKM-2*
const char *SDM20 =     "SDM:20"; // wildcard for all SDM-20*
const char *SDM2 =      "SDM:2"; // wildcard for all SDM-2*
const char *SDS210 =    "SDS:210";
const char *SDS20 =     "SDS:20"; // wildcard for SDS-20*
const char *SDS22 =     "SDS:22"; // wildcard for SDS-22*
const char *SDS2 =      "SDS:2"; // wildcard for all SDS-2*
const char *ZWS2 =      "ZWS:2"; // wildcard for all ZWS-2*
const char *KL213 =     "KL:213";
const char *KL214 =     "KL:214";
const char *UMV204 =    "UMV:204";
const char *UMV200 =    "UMV:200";
const char *UMV210 =    "UMV:210";
const char *UMR200 =    "UMR:200";
const char *SK20 =     "SK:20";
const char *AKM2 =      "AKM:2"; // wildcard for all AKM-2*

// all available features
const ModelFeatureId MF_AVAILABLE[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::leddark,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outmodeswitch,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::pushbcombined,
  ModelFeatureId::shadeprops,
  ModelFeatureId::shadeposition,
  ModelFeatureId::motiontimefins,
  ModelFeatureId::optypeconfig,
  ModelFeatureId::shadebladeang,
  ModelFeatureId::highlevel,
  ModelFeatureId::consumption,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::akmsensor,
  ModelFeatureId::akminput,
  ModelFeatureId::akmdelay,
  ModelFeatureId::twowayconfig,
  ModelFeatureId::outputchannels,
  ModelFeatureId::heatinggroup,
  ModelFeatureId::heatingoutmode,
  ModelFeatureId::heatingprops,
  ModelFeatureId::pwmvalue,
  ModelFeatureId::valvetype,
  ModelFeatureId::extradimmer,
  ModelFeatureId::umvrelay,
  ModelFeatureId::blinkconfig,
  ModelFeatureId::umroutmode,
  ModelFeatureId::pushbsensor,
  ModelFeatureId::locationconfig,
  ModelFeatureId::windprotectionconfigawning,
  ModelFeatureId::windprotectionconfigblind,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::outmodegeneric,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::temperatureoffset
};

// model features
const ModelFeatureId MF_GE_KM220[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_KM2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_KL200[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmodeswitch,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_TKM210[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_TKM220[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced
};

const ModelFeatureId MF_GE_TKM230[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced
};

const ModelFeatureId MF_GE_SDM20[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_SDS210[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::twowayconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_SDS20[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::pushbcombined,
  ModelFeatureId::twowayconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_SDS22[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::pushbcombined,
  ModelFeatureId::twowayconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_SDS2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::twowayconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};


const ModelFeatureId MF_GE_ZWS2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_UMV204[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_UMV200[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::extradimmer,
  ModelFeatureId::umvrelay,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GE_UMV210[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::extradimmer,
  ModelFeatureId::umvrelay,
  ModelFeatureId::outputchannels,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_GN_KM2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8
};

const ModelFeatureId MF_GN_TKM2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8
};

const ModelFeatureId MF_RT_KM2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8
};

const ModelFeatureId MF_RT_TKM2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::transt,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8
};

const ModelFeatureId MF_RT_SDM2[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::outmode,
  ModelFeatureId::outvalue8
};

const ModelFeatureId MF_GR_KL210[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::shadeprops,
  ModelFeatureId::shadeposition,
  ModelFeatureId::locationconfig,
  ModelFeatureId::windprotectionconfigawning
};

const ModelFeatureId MF_GR_KL220[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::shadeprops,
  ModelFeatureId::shadeposition,
  ModelFeatureId::motiontimefins,
  ModelFeatureId::shadebladeang,
  ModelFeatureId::locationconfig,
  ModelFeatureId::windprotectionconfigblind
};

const ModelFeatureId MF_GR_KL230[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::shadeprops,
  ModelFeatureId::shadeposition,
  ModelFeatureId::motiontimefins,
  ModelFeatureId::shadebladeang,
  ModelFeatureId::locationconfig,
  ModelFeatureId::windprotectionconfigblind
};

const ModelFeatureId MF_GR_KL2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::shadeprops,
  ModelFeatureId::shadeposition,
  ModelFeatureId::locationconfig,
  ModelFeatureId::windprotectionconfigblind
};

const ModelFeatureId MF_GR_TKM2[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced
};

const ModelFeatureId MF_BL_KM200[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::ledauto,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::heatinggroup,
  ModelFeatureId::heatingoutmode,
  ModelFeatureId::heatingprops,
  ModelFeatureId::pwmvalue,
  ModelFeatureId::valvetype
};

const ModelFeatureId MF_BL_SDS200[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::ledauto,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::heatinggroup,
  ModelFeatureId::heatingoutmode,
  ModelFeatureId::heatingprops,
  ModelFeatureId::pwmvalue,
  ModelFeatureId::valvetype
};

const ModelFeatureId MF_BL_KL2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::ledauto
};

const ModelFeatureId MF_TK_TKM2[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea
};

const ModelFeatureId MF_GE_TKM2[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_SW_TKM2[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::highlevel,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::twowayconfig
};

const ModelFeatureId MF_SW_KL2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::optypeconfig,
  ModelFeatureId::highlevel,
  ModelFeatureId::consumption,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_SW_ZWS2[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbdevice,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::optypeconfig,
  ModelFeatureId::highlevel,
  ModelFeatureId::consumption,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_SW_KL213[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::optypeconfig,
  ModelFeatureId::highlevel,
  ModelFeatureId::consumption,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_SW_KL214[] =
{
  ModelFeatureId::dontcare,
  ModelFeatureId::blink,
  ModelFeatureId::ledauto,
  ModelFeatureId::transt,
  ModelFeatureId::outvalue8,
  ModelFeatureId::optypeconfig,
  ModelFeatureId::highlevel,
  ModelFeatureId::consumption,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::outconfigswitch,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::blinkconfig
};

const ModelFeatureId MF_SW_SDS20[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::highlevel,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::twowayconfig
};

const ModelFeatureId MF_SW_SDS22[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::leddark,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::highlevel,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::twowayconfig
};

const ModelFeatureId MF_SW_AKM2[] =
{
  ModelFeatureId::akmsensor,
  ModelFeatureId::akminput,
  ModelFeatureId::akmdelay
};

const ModelFeatureId MF_SW_UMR200[] =
{
  ModelFeatureId::blink,
  ModelFeatureId::dontcare,
  ModelFeatureId::outvalue8,
  ModelFeatureId::pushbutton,
  ModelFeatureId::pushbsensor,
  ModelFeatureId::pushbarea,
  ModelFeatureId::pushbadvanced,
  ModelFeatureId::highlevel,
  ModelFeatureId::jokerconfig,
  ModelFeatureId::twowayconfig,
  ModelFeatureId::akminput,
  ModelFeatureId::akmsensor,
  ModelFeatureId::akmdelay,
  ModelFeatureId::blinkconfig,
  ModelFeatureId::impulseconfig,
  ModelFeatureId::umroutmode,
  ModelFeatureId::outconfigswitch,
};

const ModelFeatureId MF_SW_SK20[] =
{
    ModelFeatureId::temperatureoffset
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
  boost::shared_ptr<std::vector<ModelFeatureId> > fv;
  fv = boost::make_shared<std::vector<ModelFeatureId> >();

  fv->assign(MF_GE_KM220, ARRAY_END(MF_GE_KM220));
  setFeatures(ColorIDYellow, KM220, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_KM2, ARRAY_END(MF_GE_KM2));
  setFeatures(ColorIDYellow, KM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_KL200, ARRAY_END(MF_GE_KL200));
  setFeatures(ColorIDYellow, KL200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_TKM210, ARRAY_END(MF_GE_TKM210));
  setFeatures(ColorIDYellow, TKM210, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_TKM220, ARRAY_END(MF_GE_TKM220));
  setFeatures(ColorIDYellow, TKM220, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_TKM230, ARRAY_END(MF_GE_TKM230));
  setFeatures(ColorIDYellow, TKM230, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_SDM20, ARRAY_END(MF_GE_SDM20));
  setFeatures(ColorIDYellow, SDM20, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_SDS210, ARRAY_END(MF_GE_SDS210));
  setFeatures(ColorIDYellow, SDS210, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_SDS20, ARRAY_END(MF_GE_SDS20));
  setFeatures(ColorIDYellow, SDS20, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_SDS22, ARRAY_END(MF_GE_SDS22));
  setFeatures(ColorIDYellow, SDS22, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_SDS2, ARRAY_END(MF_GE_SDS2));
  setFeatures(ColorIDYellow, SDS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_ZWS2, ARRAY_END(MF_GE_ZWS2));
  setFeatures(ColorIDYellow, ZWS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_UMV204, ARRAY_END(MF_GE_UMV204));
  setFeatures(ColorIDYellow, UMV204, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_UMV200, ARRAY_END(MF_GE_UMV200));
  setFeatures(ColorIDYellow, UMV200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_UMV210, ARRAY_END(MF_GE_UMV210));
  setFeatures(ColorIDYellow, UMV210, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GN_KM2, ARRAY_END(MF_GN_KM2));
  setFeatures(ColorIDGreen, KM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GN_TKM2, ARRAY_END(MF_GN_TKM2));
  setFeatures(ColorIDGreen, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_RT_KM2, ARRAY_END(MF_RT_KM2));
  setFeatures(ColorIDRed, KM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_RT_TKM2, ARRAY_END(MF_RT_TKM2));
  setFeatures(ColorIDRed, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_RT_SDM2, ARRAY_END(MF_RT_SDM2));
  setFeatures(ColorIDRed, SDM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GR_KL210, ARRAY_END(MF_GR_KL210));
  setFeatures(ColorIDGray, KL210, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GR_KL220, ARRAY_END(MF_GR_KL220));
  setFeatures(ColorIDGray, KL220, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GR_KL230, ARRAY_END(MF_GR_KL230));
  setFeatures(ColorIDGray, KL230, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GR_KL2, ARRAY_END(MF_GR_KL2));
  setFeatures(ColorIDGray, KL2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GR_TKM2, ARRAY_END(MF_GR_TKM2));
  setFeatures(ColorIDGray, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_BL_KM200, ARRAY_END(MF_BL_KM200));
  setFeatures(ColorIDBlue, KM200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_BL_SDS200, ARRAY_END(MF_BL_SDS200));
  setFeatures(ColorIDBlue, SDS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_BL_KL2, ARRAY_END(MF_BL_KL2));
  setFeatures(ColorIDBlue, KL2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_GE_TKM2, ARRAY_END(MF_GE_TKM2));
  setFeatures(ColorIDYellow, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_TK_TKM2, ARRAY_END(MF_TK_TKM2));
  setFeatures(ColorIDCyan, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_TKM2, ARRAY_END(MF_SW_TKM2));
  setFeatures(ColorIDBlack, TKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_KL2, ARRAY_END(MF_SW_KL2));
  setFeatures(ColorIDBlack, KL2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_ZWS2, ARRAY_END(MF_SW_ZWS2));
  setFeatures(ColorIDBlack, ZWS2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_KL213, ARRAY_END(MF_SW_KL213));
  setFeatures(ColorIDBlack, KL213, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_KL214, ARRAY_END(MF_SW_KL214));
  setFeatures(ColorIDBlack, KL214, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_SDS20, ARRAY_END(MF_SW_SDS20));
  setFeatures(ColorIDBlack, SDS20, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_SDS22, ARRAY_END(MF_SW_SDS22));
  setFeatures(ColorIDBlack, SDS22, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_AKM2, ARRAY_END(MF_SW_AKM2));
  setFeatures(ColorIDBlack, AKM2, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_UMR200, ARRAY_END(MF_SW_UMR200));
  setFeatures(ColorIDBlack, UMR200, fv);
  fv.reset();

  fv = boost::make_shared<std::vector<ModelFeatureId> >();
  fv->assign(MF_SW_SK20, ARRAY_END(MF_SW_SK20));
  setFeatures(ColorIDBlack, SK20, fv);
  fv.reset();
}

void ModelFeatures::setFeatures(int _color, std::string _model, boost::shared_ptr<std::vector<ModelFeatureId> > _features) {
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
      boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<ModelFeatureId> > > > content = m_features.at(_color).at(i);
      return;
    }
  }

  m_features.at(_color).push_back(boost::make_shared<std::pair<std::string, boost::shared_ptr<const std::vector<ModelFeatureId> > > >(_model, _features));
}

std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<ModelFeatureId> > > > > ModelFeatures::getFeatures(int _color) {
  if ((_color < ColorIDYellow) || (_color > ColorIDBlack)) {
    throw std::runtime_error("can not retrieve features: unsupported device color");
  }

  boost::mutex::scoped_lock lock(m_lock);

  if (m_features.size() == 0) {
    throw std::runtime_error("zero sized features vector!");
  }

  return m_features.at(_color);
}

ModelFeatureId ModelFeatures::nameToFeature(std::string _name) {
  if (_name == "dontcare") {
    return ModelFeatureId::dontcare;
  } else if (_name == "blink") {
    return ModelFeatureId::blink;
  } else if (_name == "ledauto") {
    return ModelFeatureId::ledauto;
  } else if (_name == "leddark") {
    return ModelFeatureId::leddark;
  } else if (_name == "transt") {
    return ModelFeatureId::transt;
  } else if (_name == "outmode") {
    return ModelFeatureId::outmode;
  } else if (_name == "outmodeswitch") {
    return ModelFeatureId::outmodeswitch;
  } else if (_name == "outvalue8") {
    return ModelFeatureId::outvalue8;
  } else if (_name == "pushbutton") {
    return ModelFeatureId::pushbutton;
  } else if (_name == "pushbdevice") {
    return ModelFeatureId::pushbdevice;
  } else if (_name == "pushbarea") {
    return ModelFeatureId::pushbarea;
  } else if (_name == "pushbadvanced") {
    return ModelFeatureId::pushbadvanced;
  } else if (_name == "pushbcombined") {
    return ModelFeatureId::pushbcombined;
  } else if (_name == "shadeprops") {
    return ModelFeatureId::shadeprops;
  } else if (_name == "shadeposition") {
    return ModelFeatureId::shadeposition;
  } else if (_name == "motiontimefins") {
    return ModelFeatureId::motiontimefins;
  } else if (_name == "optypeconfig") {
    return ModelFeatureId::optypeconfig;
  } else if (_name == "shadebladeang") {
    return ModelFeatureId::shadebladeang;
  } else if (_name == "highlevel") {
    return ModelFeatureId::highlevel;
  } else if (_name == "consumption") {
    return ModelFeatureId::consumption;
  } else if (_name == "jokerconfig") {
    return ModelFeatureId::jokerconfig;
  } else if (_name == "akmsensor") {
    return ModelFeatureId::akmsensor;
  } else if (_name == "akminput") {
    return ModelFeatureId::akminput;
  } else if (_name == "akmdelay") {
    return ModelFeatureId::akmdelay;
  } else if (_name == "twowayconfig") {
    return ModelFeatureId::twowayconfig;
  } else if (_name == "outputchannels") {
    return ModelFeatureId::outputchannels;
  } else if (_name == "heatinggroup") {
    return ModelFeatureId::heatinggroup;
  } else if (_name == "heatingoutmode") {
    return ModelFeatureId::heatingoutmode;
  } else if (_name == "heatingprops") {
    return ModelFeatureId::heatingprops;
  } else if (_name == "pwmvalue") {
    return ModelFeatureId::pwmvalue;
  } else if (_name == "valvetype") {
    return ModelFeatureId::valvetype;
  } else if (_name == "extradimmer") {
    return ModelFeatureId::extradimmer;
  } else if (_name == "umvrelay") {
    return ModelFeatureId::umvrelay;
  } else if (_name == "blinkconfig") {
    return ModelFeatureId::blinkconfig;
  } else if (_name == "impulseconfig") {
    return ModelFeatureId::impulseconfig;
  } else if (_name == "umroutmode") {
    return ModelFeatureId::umroutmode;
  } else if (_name == "pushbsensor") {
    return ModelFeatureId::pushbsensor;
  } else if (_name == "locationconfig") {
    return ModelFeatureId::locationconfig;
  } else if (_name == "windprotectionconfigblind") {
    return ModelFeatureId::windprotectionconfigblind;
  } else if (_name == "windprotectionconfigawning") {
    return ModelFeatureId::windprotectionconfigawning;
  } else if (_name == "outmodegeneric") {
    return ModelFeatureId::outmodegeneric;
  } else if (_name == "outconfigswitch") {
    return ModelFeatureId::outconfigswitch;
  } else if (_name == "temperatureoffset") {
    return ModelFeatureId::temperatureoffset;
  }

  throw std::runtime_error("unknown feature encountered");
}

std::string ModelFeatures::getFeatureName(ModelFeatureId _feature)
{
  switch (_feature)
  {
    case ModelFeatureId::dontcare:
      return "dontcare";
    case ModelFeatureId::blink:
      return "blink";
    case ModelFeatureId::ledauto:
      return "ledauto";
    case ModelFeatureId::leddark:
      return "leddark";
    case ModelFeatureId::transt:
      return "transt";
    case ModelFeatureId::outmode:
      return "outmode";
    case ModelFeatureId::outmodeswitch:
      return "outmodeswitch";
    case ModelFeatureId::outvalue8:
      return "outvalue8";
    case ModelFeatureId::pushbutton:
      return "pushbutton";
    case ModelFeatureId::pushbdevice:
      return "pushbdevice";
    case ModelFeatureId::pushbarea:
      return "pushbarea";
    case ModelFeatureId::pushbadvanced:
      return "pushbadvanced";
    case ModelFeatureId::pushbcombined:
      return "pushbcombined";
    case ModelFeatureId::shadeprops:
      return "shadeprops";
    case ModelFeatureId::shadeposition:
      return "shadeposition";
    case ModelFeatureId::motiontimefins:
      return "motiontimefins";
    case ModelFeatureId::optypeconfig:
      return "optypeconfig";
    case ModelFeatureId::shadebladeang:
      return "shadebladeang";
    case ModelFeatureId::highlevel:
      return "highlevel";
    case ModelFeatureId::consumption:
      return "consumption";
    case ModelFeatureId::jokerconfig:
      return "jokerconfig";
    case ModelFeatureId::akmsensor:
      return "akmsensor";
    case ModelFeatureId::akminput:
      return "akminput";
    case ModelFeatureId::akmdelay:
      return "akmdelay";
    case ModelFeatureId::twowayconfig:
      return "twowayconfig";
    case ModelFeatureId::outputchannels:
      return "outputchannels";
    case ModelFeatureId::heatinggroup:
      return "heatinggroup";
    case ModelFeatureId::heatingoutmode:
      return "heatingoutmode";
    case ModelFeatureId::heatingprops:
      return "heatingprops";
    case ModelFeatureId::pwmvalue:
      return "pwmvalue";
    case ModelFeatureId::valvetype:
      return "valvetype";
    case ModelFeatureId::extradimmer:
      return "extradimmer";
    case ModelFeatureId::umvrelay:
      return "umvrelay";
    case ModelFeatureId::blinkconfig:
      return "blinkconfig";
    case ModelFeatureId::impulseconfig:
      return "impulseconfig";
    case ModelFeatureId::umroutmode:
      return "umroutmode";
    case ModelFeatureId::pushbsensor:
      return "pushbsensor";
    case ModelFeatureId::locationconfig:
      return "locationconfig";
    case ModelFeatureId::windprotectionconfigawning:
      return "windprotectionconfigawning";
    case ModelFeatureId::windprotectionconfigblind:
      return "windprotectionconfigblind";
    case ModelFeatureId::outmodegeneric:
      return "outmodegeneric";
    case ModelFeatureId::outconfigswitch:
      return "outconfigswitch";
    case ModelFeatureId::temperatureoffset:
      return "temperatureoffset";
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

boost::shared_ptr<std::vector<ModelFeatureId> > ModelFeatures::getAvailableFeatures()
{
  boost::shared_ptr<std::vector<ModelFeatureId> > ret;
  ret = boost::make_shared<std::vector<ModelFeatureId> >();
  ret->assign(MF_AVAILABLE, ARRAY_END(MF_AVAILABLE));
  return ret;
}

}; // namespace


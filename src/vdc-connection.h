/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifndef __DSS_VDC_CONNECTION_H__
#define __DSS_VDC_CONNECTION_H__

#include "config.h"

#include <string>
#include <boost/shared_ptr.hpp>
#include <digitalSTROM/ds.h>

namespace dss {

  class JSONElement;
  class JSONObject;

  typedef struct {
    std::string hardwareModelGuid;
    std::string modelUID;
    std::string vendorGuid;
    std::string oemGuid;
    std::string configURL;
    std::string hardwareGuid;
    std::string hardwareInfo;     // property: model
    std::string hardwareVersion;
    std::string name;
    boost::shared_ptr<std::vector<int> > modelFeatures;
  } VdsdSpec_t;

  typedef struct {
    bool hasDevices;
    bool hasMetering;
    bool hasTemperatureControl;
    std::string model;
    std::string modelVersion;
    std::string hardwareVersion;
  } VdcSpec_t;

  struct VdcHelper
  {
    static boost::shared_ptr<VdsdSpec_t> getSpec(dsuid_t _vdsm, dsuid_t _device);
    static boost::shared_ptr<VdcSpec_t> getCapabilities(dsuid_t _vdsm);
    static void getIcon(dsuid_t _vdsm, dsuid_t _device, size_t *size, uint8_t **data);
  };

} // namespace
#endif//__DSS_VDC_CONNECTION_H__

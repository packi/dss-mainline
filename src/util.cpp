/*
    Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland

    Authors: Christian Hitz, aizo AG <christian.hitz@aizo.com>
             Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include <boost/make_shared.hpp>

#include "dss.h"
#include "util.h"
#include "logger.h"
#include "base.h"
#include "model/apartment.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/cluster.h"
#include "model/device.h"
#include "model/modulator.h"
#include "model/modelconst.h"
#include "ds485/dsdevicebusinterface.h"
#include "foreach.h"
#include "expatparser.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <src/messages/vdcapi.pb.h>


namespace dss {

  void synchronizeGroups(Apartment* _apartment, StructureModifyingBusInterface* _interface) {
    std::vector<boost::shared_ptr<Cluster> > clusters = _apartment->getClusters();
    foreach(boost::shared_ptr<Cluster> pCluster, clusters) {
      if (!pCluster->isSynchronized()) {

        Logger::getInstance()->log("Forced configuration update in cluster " +
            intToString(pCluster->getID()), lsInfo);

        try {
          _interface->clusterSetName(pCluster->getID(), pCluster->getName());
          _interface->clusterSetApplication(pCluster->getID(), pCluster->getApplicationType(), pCluster->getApplicationConfiguration());
          _interface->clusterSetProperties(pCluster->getID(), pCluster->getLocation(),
                                           pCluster->getFloor(), pCluster->getProtectionClass());
          _interface->clusterSetLockedScenes(pCluster->getID(), pCluster->getLockedScenes());
          pCluster->setIsSynchronized(true);
        } catch (BusApiError& e) {
          Logger::getInstance()->log("Error updating configuration for cluster " + intToString(pCluster->getID()) +
              ": " + e.what(), lsWarning);
        }
    }

    std::vector<boost::shared_ptr<Group> > globalApps = _apartment->getGlobalApps();
    foreach (boost::shared_ptr<Group> pGlobalApp, globalApps) {
      if (!pGlobalApp->isSynchronized()) {
        Logger::getInstance()->log(
            "Forced configuration update in global application group " + intToString(pGlobalApp->getID()), lsInfo);

        try {
          _interface->groupSetApplication(
              0, pGlobalApp->getID(), pGlobalApp->getApplicationType(), pGlobalApp->getApplicationConfiguration());

          pGlobalApp->setIsSynchronized(true);
        } catch (BusApiError& e) {
          Logger::getInstance()->log(
              "Error updating global application group " + intToString(pGlobalApp->getID()) + ": " + e.what(),
              lsWarning);
        }
      }
    }

    std::vector<boost::shared_ptr<Zone> > zones = _apartment->getZones();
    foreach(boost::shared_ptr<Zone> pZone, zones) {
      if (pZone->getID() == 0 || !pZone->isConnected()) {
        continue;
      }
      std::vector<boost::shared_ptr<Group> > groups = pZone->getGroups();
      foreach(boost::shared_ptr<Group> pGroup, groups) {
          // Regular-User-Groups
          if (isZoneUserGroup(pGroup->getID())) {
            try {
              _interface->createGroup(pZone->getID(), pGroup->getID(),
                  pGroup->getApplicationType(), pGroup->getApplicationConfiguration(), pGroup->getName());
              _interface->groupSetApplication(pZone->getID(), pGroup->getID(),
                  pGroup->getApplicationType(), pGroup->getApplicationConfiguration());
              pGroup->setIsSynchronized(true);
            } catch (BusApiError& e) {
              Logger::getInstance()->log("Error updating user group configuration in zone " +
                  intToString(pZone->getID()) + " and group " + intToString(pGroup->getID()) +
                  ": " + e.what(), lsWarning);
            }
          }

        }
      }
    }
  }

  std::string escapeHTML(std::string _input) {
    boost::replace_all(_input, "&", "&amp;");
    boost::replace_all(_input, "\"", "&quot;");
    boost::replace_all(_input, "'", "&#039;");
    boost::replace_all(_input, "<", "&lt;");
    boost::replace_all(_input, ">", "&gt;");

    return _input;
  }

  std::string unescapeHTML(std::string _input) {
    boost::replace_all(_input, "&amp;", "&");
    boost::replace_all(_input, "&quot;", "\"");
    boost::replace_all(_input, "&#039;", "'");
    boost::replace_all(_input, "&lt;", "<");
    boost::replace_all(_input, "&gt;", ">");

    return _input;
  }

typedef struct OutputChannelInfo
{
    const char *id;
    uint8_t size;
    uint8_t min;
    double max;
} OutputChannelInfo_t;

static OutputChannelInfo kOutputChannels[] = {
    { NULL,             0,  0,      0       },  // id 0
    { "brightness",     8,  0,      100     },  // id 1,    %
    { "hue",            8,  0,      358.6   },  // id 2,    °
    { "saturation",     8,  0,      100     },  // id 3,    %
    { "colortemp",      8,  100,    1000    },  // id 4,    mired
    { "x",              8,  0,      1.0     },  // id 5
    { "y",              8,  0,      1.0     },  // id 6
    { "verticalpos",    16, 0,      100     },  // id 7,    %
    { "horizontalpos",  16, 0,      100     },  // id 8,    %
    { "openinganglepos",8,  0,      100     },  // id 9,    %
    { "permeability",   8,  0,      100     }   // id 10,   %
};

  std::pair<uint8_t, uint8_t> getOutputChannelIdAndSize(std::string _channelName) {
    for (uint8_t i = MinimumOutputChannelID; i <= MaximumOutputChannelID; i++) {
      if (_channelName == kOutputChannels[i].id) {
        return std::make_pair(i, kOutputChannels[i].size);
      }
    }

    throw std::invalid_argument("invalid channel name: '" + _channelName + "'");
  }

  std::string getOutputChannelName(uint8_t channel) {
    if ((channel < MinimumOutputChannelID) || (channel > MaximumOutputChannelID)) {
      throw std::invalid_argument("invalid channel id: '" +
                                   intToString(channel) + "'");
    }
    return kOutputChannels[channel].id;
  }

  uint16_t convertToOutputChannelValue(uint8_t channel, double value) {
    if ((channel < MinimumOutputChannelID) ||
        (channel > MaximumOutputChannelID)) {
      throw std::invalid_argument("invalid channel id: '" +
                                   intToString(channel) + "'");
    }

    if ((value < kOutputChannels[channel].min) ||
        (value > kOutputChannels[channel].max)) {
        throw std::invalid_argument("invalid value for channel id: '" +
                                        intToString(channel) + "'");
    }

    return (uint16_t)round(((pow(2, kOutputChannels[channel].size) - 1) *
                                        value / kOutputChannels[channel].max));
  }

  double convertFromOutputChannelValue(uint8_t channel, uint16_t value) {
    if ((channel < MinimumOutputChannelID) ||
        (channel > MaximumOutputChannelID)) {
      throw std::invalid_argument("invalid channel id: '" +
                                   intToString(channel) + "'");
    }

    return (kOutputChannels[channel].max * value) /
                    (double)(pow(2, kOutputChannels[channel].size) - 1);
  }

  bool saveValidatedXML(const std::string& _fileName, const std::string& _targetFile) {
    boost::shared_ptr<XMLFileValidator> v = boost::make_shared<XMLFileValidator>();
    int ret = v->validateFile(_fileName);
    if (ret) {
      // move it to the desired location
      ret = rename(_fileName.c_str(), _targetFile.c_str());
      if (ret != 0) {
        Logger::getInstance()->log("Copying to final destination (" +
            _targetFile + ") failed: " +
            std::string(strerror(errno)), lsFatal);
      }
    } else {
      Logger::getInstance()->log("XML not saved! Generated file '" +
            _fileName + "' is invalid, will not overwrite" +
            _targetFile, lsFatal);
      if (DSS::getInstance()->getPropertySystem().getBoolValue("/config/debug/storeInvalidXML")) {
        std::string invalidDir("/var/log/invalid_xml/");
        (void)mkdir(invalidDir.c_str(), 0777); // can fail if the directory already exists
        (void)rename(_fileName.c_str(), (invalidDir + boost::filesystem::path(_fileName).filename().string()).c_str());
      }
    }

    return ret;
  }

  /**
   * filters a bitarray for non-zero bits, and returns a vector
   * containing the indices of the non-zero bits
   *
   *            0         1         2      +0, +10
   * indices:   0123456789012345678901234  0-9 repeating
   * bitmap:    0001110011100000000100100
   * -> result: [ 3, 4, 5, 8, 9, 10, 19, 22 ]
   */
  std::vector<int> parseBitfield(const uint8_t *_bitfield, int _bits) {
    std::vector<int> result;
    for (int i = 0; i < _bits; ++i) {
      if ((_bitfield[i / 8] & (1 << (i % 8))) != 0) {
        result.push_back(i);
      }
    }
    return result;
  }

  std::string propertyValue2String(const vdcapi::PropertyValue &pvalue)
  {
    std::string value;
    if (pvalue.has_v_bool()) {
      value = pvalue.v_bool() ? "active" : "inactive";
    }
    if (pvalue.has_v_double()) {
      value = doubleToString(pvalue.v_double());
    }
    if (pvalue.has_v_int64()) {
      value = intToString(pvalue.v_int64());
    }
    if (pvalue.has_v_uint64()) {
      value = uintToString(pvalue.v_uint64());
    }
    if (pvalue.has_v_string()) {
      value = pvalue.v_string();
    }
    return value;
  }

} // namespace

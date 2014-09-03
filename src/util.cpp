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

#include "util.h"
#include "logger.h"
#include "base.h"
#include "model/apartment.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/device.h"
#include "model/modulator.h"
#include "model/modelconst.h"
#include "ds485/dsdevicebusinterface.h"
#include "structuremanipulator.h"
#include "foreach.h"
#include <boost/algorithm/string/replace.hpp>
#include <math.h>

namespace dss {

  void synchronizeGroups(Apartment* _apartment, StructureModifyingBusInterface* _interface) {
    std::vector<boost::shared_ptr<Zone> > zones = _apartment->getZones();
    foreach(boost::shared_ptr<Zone> pZone, zones) {
      if (pZone->getID() == 0 || !pZone->isConnected()) {
        continue;
      }
      std::vector<boost::shared_ptr<Group> > groups = pZone->getGroups();
      foreach(boost::shared_ptr<Group> pGroup, groups) {
        if (!pGroup->isSynchronized()) {

          Logger::getInstance()->log("Forced user group configuration update in zone " +
              intToString(pZone->getID()) + " and group " + intToString(pGroup->getID()), lsInfo);

          // Special-User-Groups 16..23: get configuration from Zone-0
          if (pGroup->getID() >= 16 && pGroup->getID() <= 23) {
            try {
              boost::shared_ptr<Group> refGroup = _apartment->getGroup(pGroup->getID());
              _interface->groupSetName(pZone->getID(), refGroup->getID(),
                  refGroup->getName());
              _interface->groupSetStandardID(pZone->getID(), refGroup->getID(),
                  refGroup->getStandardGroupID());
              pGroup->setName(refGroup->getName());
              pGroup->setStandardGroupID(refGroup->getStandardGroupID());
              pGroup->setIsSynchronized(true);
            } catch (BusApiError& e) {
              Logger::getInstance()->log("Error updating user group configuration in zone " +
                  intToString(pZone->getID()) + " and group " + intToString(pGroup->getID()) +
                  ": " + e.what(), lsWarning);
            }
          }
          // Regular-User-Groups >=24
          if (pGroup->getID() >= 24 && pGroup->getID() <= 47) {
            try {
              _interface->createGroup(pZone->getID(), pGroup->getID(),
                  pGroup->getStandardGroupID(), pGroup->getName());
              _interface->groupSetName(pZone->getID(), pGroup->getID(),
                  pGroup->getName());
              _interface->groupSetStandardID(pZone->getID(), pGroup->getID(),
                  pGroup->getStandardGroupID());
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
} // namespace

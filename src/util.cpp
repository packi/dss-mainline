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
#include "ds485/dsdevicebusinterface.h"
#include "structuremanipulator.h"
#include "foreach.h"
#include <boost/algorithm/string/replace.hpp>

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

#warning This code needs to be adapted once informatoin about the klemmen becomes available.
  std::pair<uint8_t, uint8_t> getOutputChannelIdAndSize(std::string _channelName) {
    // channel names and meanings are not yet known
    int channel = strToIntDef(_channelName, -1);
    if (channel == -1) {
      throw std::invalid_argument("invalid channel name: '" + _channelName +
                                  "'");
    }

    // so far return the same size for all channels
    return std::make_pair((uint8_t)channel, 8);
  }

  std::string getOutputChannelName(uint8_t channel) {
    return intToString(channel);
  }
}


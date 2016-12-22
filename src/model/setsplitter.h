/*
    Copyright (c) 2016 digitalSTROM AG, Zurich, Switzerland

    Author: Pawel Kochanowski <pkochanowski@digitalstrom.com>

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

#ifndef SRC_MODEL_SETSPLITTER_H_
#define SRC_MODEL_SETSPLITTER_H_

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "set.h"

#include <stdexcept>
#include "src/base.h"
#include "src/util.h"
#include "src/logger.h"
#include "src/foreach.h"

#include "device.h"
#include "group.h"
#include "apartment.h"
#include "zone.h"
#include "modulator.h"

namespace dss {

  // forward declaration of helper class for unit testing
  struct SetSplitterTester;

  class SetSplitter {
    public:
      typedef std::vector<boost::shared_ptr<AddressableModelItem> > ModelItemVector;
      static ModelItemVector splitUp(Set& _set);
    private:
      typedef std::map<boost::shared_ptr<const Zone>, Set> HashMapZoneSet;

      static const bool OptimizerDebug;
      static ModelItemVector bestFit(const Set& _set);
      static ModelItemVector bestFit(const Zone& _zone, const Set& _set);

      static HashMapZoneSet splitByZone(const Set& _set);
      static boost::shared_ptr<Group> findGroupContainingAllDevices(const Set& _set, const Zone& _zone);

      static void log(const std::string& _line) {
        Logger::getInstance()->log(_line);
      }

      // make the SetSplitterUnitTester a friend so tests can access all members
      friend struct SetSplitterTester;
    };
}

#endif /* SRC_MODEL_SETSPLITTER_H_ */

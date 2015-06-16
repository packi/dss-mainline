/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

#include "src/foreach.h"

#include "unix/systeminfo.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(systeminfo)

typedef std::pair<std::string, unsigned> proc_meminfo_elt;

BOOST_AUTO_TEST_CASE(testMemoryInfo) {
  SystemInfo info;

  // if the checks fail we now what is missing
  BOOST_FOREACH(proc_meminfo_elt elt, info.parseProcMeminfo()) {
    std::cout << boost::format("%-12s : %8d\n") % elt.first % elt.second;
  }

  // should be 5 but jenkins/azure is missing MemAvailable
  BOOST_CHECK(info.parseProcMeminfo().size() >= 4);
}

BOOST_AUTO_TEST_SUITE_END()

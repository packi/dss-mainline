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

static char smaps[] =
   "00008000-0040c000 r-xp 00000000 00:0f 8774460    /usr/bin/dss\n"
   "Size:               4112 kB\n"
   "Rss:                1640 kB\n"
   "Pss:                1640 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:      1640 kB\n"
   "Private_Dirty:         0 kB\n"
   "Referenced:         1640 kB\n"
   "Anonymous:             0 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "00414000-00415000 r-xp 00404000 00:0f 8774460    /usr/bin/dss\n"
   "Size:                  4 kB\n"
   "Rss:                   4 kB\n"
   "Pss:                   4 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         4 kB\n"
   "Referenced:            4 kB\n"
   "Anonymous:             4 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "00415000-00418000 rwxp 00405000 00:0f 8774460    /usr/bin/dss\n"
   "Size:                 12 kB\n"
   "Rss:                  12 kB\n"
   "Pss:                  12 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         8 kB\n"
   "Private_Dirty:         4 kB\n"
   "Referenced:           12 kB\n"
   "Anonymous:             4 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "00418000-0041a000 rwxp 00000000 00:00 0          [heap]\n"
   "Size:                  8 kB\n"
   "Rss:                   8 kB\n"
   "Pss:                   8 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         8 kB\n"
   "Referenced:            8 kB\n"
   "Anonymous:             8 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "4000e000-4000f000 rwxp 00000000 00:00 0\n"
   "Size:                  4 kB\n"
   "Rss:                   4 kB\n"
   "Pss:                   4 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         4 kB\n"
   "Referenced:            4 kB\n"
   "Anonymous:             4 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "40010000-4001d000 r-xp 00000000 00:0f 8773511    /usr/lib/libavahi-client.so.3.2.9\n"
   "Size:                 52 kB\n"
   "Rss:                  24 kB\n"
   "Pss:                   7 kB\n"
   "Shared_Clean:         24 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         0 kB\n"
   "Referenced:           24 kB\n"
   "Anonymous:             0 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "4001d000-40024000 ---p 0000d000 00:0f 8773511    /usr/lib/libavahi-client.so.3.2.9\n"
   "Size:                 28 kB\n"
   "Rss:                   0 kB\n"
   "Pss:                   0 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         0 kB\n"
   "Referenced:            0 kB\n"
   "Anonymous:             0 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "40024000-40025000 rwxp 0000c000 00:0f 8773511    /usr/lib/libavahi-client.so.3.2.9\n"
   "Size:                  4 kB\n"
   "Rss:                   4 kB\n"
   "Pss:                   4 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         4 kB\n"
   "Referenced:            4 kB\n"
   "Anonymous:             4 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "40025000-40027000 rwxp 00000000 00:00 0\n"
   "Size:                  8 kB\n"
   "Rss:                   8 kB\n"
   "Pss:                   8 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         8 kB\n"
   "Referenced:            8 kB\n"
   "Anonymous:             8 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "beaa4000-beac5000 rw-p 00000000 00:00 0          [stack]\n"
   "Size:                136 kB\n"
   "Rss:                  12 kB\n"
   "Pss:                  12 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:        12 kB\n"
   "Referenced:           12 kB\n"
   "Anonymous:            12 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n"
   "ffff0000-ffff1000 r-xp 00000000 00:00 0          [vectors]\n"
   "Size:                  4 kB\n"
   "Rss:                   4 kB\n"
   "Pss:                   4 kB\n"
   "Shared_Clean:          0 kB\n"
   "Shared_Dirty:          0 kB\n"
   "Private_Clean:         0 kB\n"
   "Private_Dirty:         4 kB\n"
   "Referenced:            4 kB\n"
   "Anonymous:             0 kB\n"
   "AnonHugePages:         0 kB\n"
   "Swap:                  0 kB\n"
   "KernelPageSize:        4 kB\n"
   "MMUPageSize:           4 kB\n"
   "Locked:                0 kB\n" ;

static int createSmaps(const std::string &fileName) {
  std::ofstream ofs(fileName.c_str());
  ofs << smaps;
  ofs.close();
  return 0;
}

BOOST_AUTO_TEST_CASE(testLoadSmaps) {
  SystemInfo info;
  struct mapinfo *smaps, *it;

  std::string fileName = "/tmp/smaps.test"; // TODO mktemp
  createSmaps(fileName);

  smaps = info.loadMaps(fileName);

  std::vector<std::string> headers;
  BOOST_CHECK(smaps != NULL);
  for (it = smaps; it; it = it->next) {
    // TODO free(it)
    //std::cout << boost::format("%s\n") % it->name;
    headers.push_back(std::string(it->name));
  }
  BOOST_CHECK_EQUAL(headers.size(), 6);
}


BOOST_AUTO_TEST_SUITE_END()

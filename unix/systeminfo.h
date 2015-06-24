/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#ifndef SYSTEMINFO_H_
#define SYSTEMINFO_H_

#include <cstring>
#include <vector>

#include "src/logger.h"

namespace dss {

  struct mapinfo {
    struct mapinfo *next;
    unsigned start;
    unsigned end;
    unsigned size;
    unsigned rss;
    unsigned pss;
    unsigned shared_clean;
    unsigned shared_dirty;
    unsigned private_clean;
    unsigned private_dirty;
    int is_bss;
    int count;
    char name[1];
  };

  class SystemInfo {
  __DECL_LOG_CHANNEL__
  public:
    void collect();
    void memory();
    std::vector<std::string> sysinfo();
    std::vector<std::pair<std::string, unsigned> > parseProcMeminfo();
    struct mapinfo* loadMaps(const std::string &smaps = "/proc/self/smaps");
    struct mapinfo sumSmaps(struct mapinfo *smaps);
  private:
    void enumerateInterfaces();

    int mapIsLibrary(const char *name);
    int parseMapHeader(const char* line, const mapinfo* prev, mapinfo** mi);
    int parseMapField(mapinfo* mi, const char* line);
    void enqueueMapInfo(mapinfo **head, mapinfo *map, int sort_by_address, int coalesce_by_name);
    void updateMemoryUsage();
  };
}

#endif /* SYSTEMINFO_H_ */

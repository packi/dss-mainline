/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2015 digitalSTROM ag, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

    Memory smaps parsing based on showmap tool, licensed under the Apache License v2
    https://android.googlesource.com/platform/system/extras/+/master/showmap/showmap.c

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


#include "systeminfo.h"

#ifdef linux
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

#if defined(__CYGWIN__)
#include <sys/ioctl.h>
#endif


#include <arpa/inet.h>

#ifdef __APPLE__
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#endif

#include <cstring>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "src/foreach.h"
#include "src/logger.h"
#include "src/dss.h"
#include "src/propertysystem.h"
#include "src/base.h"

namespace dss {
  __DEFINE_LOG_CHANNEL__(SystemInfo, lsInfo)

  void SystemInfo::collect() {
    enumerateInterfaces();
  } // collect

  void SystemInfo::memory() {
    updateMemoryUsage();
  } // memory

  std::string ipToString(struct sockaddr* _addr) {
    char ipBuf[INET6_ADDRSTRLEN];
    switch(_addr->sa_family) {
    case AF_INET:
      inet_ntop(AF_INET, &(((struct sockaddr_in *)_addr)->sin_addr),
          ipBuf, INET6_ADDRSTRLEN);
      break;
    case AF_INET6:
      inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)_addr)->sin6_addr),
          ipBuf, INET6_ADDRSTRLEN);
      break;
    default:
      ipBuf[0] = '\0';
      Logger::getInstance()->log("Unknown address type", lsWarning);
    }
    return ipBuf;
  } // ipToString

  int SystemInfo::mapIsLibrary(const char *name) {
    int len = strlen(name);
    return len >= 4 && name[0] == '/'
        && name[len - 3] == '.' && name[len - 2] == 's' && name[len - 1] == 'o';
  }

  int SystemInfo::parseMapHeader(const char* line, const mapinfo* prev, mapinfo** mi) {
    unsigned long start;
    unsigned long end;
    char name[128];
    int name_pos;
    int is_bss = 0;

    *mi = NULL;
    if (sscanf(line, "%lx-%lx %*s %*x %*x:%*x %*d%n", &start, &end, &name_pos) != 2) {
      return -1;
    }
    while (isspace(line[name_pos])) {
      name_pos += 1;
    }
    if (line[name_pos]) {
      strncpy(name, line + name_pos, sizeof(name));
    } else {
      if (prev && start == prev->end && mapIsLibrary(prev->name)) {
        // anonymous mappings immediately adjacent to shared libraries
        // usually correspond to the library BSS segment, so we use the
        // library's own name
        strncpy(name, prev->name, sizeof(name));
        is_bss = 1;
      } else {
        strncpy(name, "[anon]", sizeof(name));
      }
    }
    const int name_size = strlen(name) + 1;
    struct mapinfo* info = (struct mapinfo*) calloc(1, sizeof(mapinfo) + name_size);
    if (info == NULL) {
      return -1;
    }
    info->start = start;
    info->end = end;
    info->is_bss = is_bss;
    info->count = 1;
    strncpy(info->name, name, name_size);
    *mi = info;
    return 0;
  }

  int SystemInfo::parseMapField(mapinfo* mi, const char* line) {
    char field[64];
    int len;
    if (sscanf(line, "%63s %n", field, &len) == 1 && *field && field[strlen(field) - 1] == ':') {
      int size;
      if (sscanf(line + len, "%d kB", &size) == 1) {
        if (!strcmp(field, "Size:")) {
          mi->size = size;
        } else if (!strcmp(field, "Rss:")) {
          mi->rss = size;
        } else if (!strcmp(field, "Pss:")) {
          mi->pss = size;
        } else if (!strcmp(field, "Shared_Clean:")) {
          mi->shared_clean = size;
        } else if (!strcmp(field, "Shared_Dirty:")) {
          mi->shared_dirty = size;
        } else if (!strcmp(field, "Private_Clean:")) {
          mi->private_clean = size;
        } else if (!strcmp(field, "Private_Dirty:")) {
          mi->private_dirty = size;
        }
      }
      return 0;
    }
    return -1;
  }

  void SystemInfo::enqueueMapInfo(mapinfo **head, mapinfo *map, int sort_by_address, int coalesce_by_name) {
    mapinfo *prev = NULL;
    mapinfo *current = *head;
    if (!map) {
      return;
    }
    for (;;) {
      if (current && coalesce_by_name && !strcmp(map->name, current->name)) {
        current->size += map->size;
        current->rss += map->rss;
        current->pss += map->pss;
        current->shared_clean += map->shared_clean;
        current->shared_dirty += map->shared_dirty;
        current->private_clean += map->private_clean;
        current->private_dirty += map->private_dirty;
        current->is_bss &= map->is_bss;
        current->count++;
        free(map);
        break;
      }
      int order_before = 0;
      if (current) {
        if (sort_by_address) {
          order_before = map->start < current->start || (map->start == current->start && map->end < current->end);
        } else {
          order_before = strcmp(map->name, current->name) < 0;
        }
      }
      if (!current || order_before) {
        if (prev) {
          prev->next = map;
        } else {
          *head = map;
        }
        map->next = current;
        break;
      }
      prev = current;
      current = current->next;
    }
  }

  struct mapinfo *SystemInfo::loadMaps()
  {
    FILE *fp;
    struct mapinfo *head = NULL;
    struct mapinfo *current = NULL;
    char line[1024];
    int len;

    fp = fopen("/proc/self/smaps", "r");
    if (NULL == fp) {
      return 0;
    }
    while (fgets(line, sizeof(line), fp) != 0) {
      len = strlen(line);
      if (line[len - 1] == '\n') {
        line[--len] = 0;
      }
      if (current != NULL && !parseMapField(current, line)) {
        continue;
      }
      struct mapinfo *next;
      if (!parseMapHeader(line, current, &next)) {
        enqueueMapInfo(&head, current, 1, 1);
        current = next;
        continue;
      }
      fprintf(stderr, "warning: could not parse map info line: %s\n", line);
    }
    enqueueMapInfo(&head, current, 1, 1);
    fclose(fp);
    if (!head) {
      fprintf(stderr, "could not read /proc/%d/smaps\n", getpid());
      return NULL;
    }
    return head;
  }

  std::vector<std::pair<std::string, unsigned> > SystemInfo::parseProcMeminfo()
  {
    std::vector<std::pair<std::string, unsigned> > result;

    int len;
    char line[1024];
    FILE *fp = fopen("/proc/meminfo", "r");

    if (NULL == fp) {
      return result;
    }

    while (fgets(line, sizeof(line), fp) != 0) {
      len = strlen(line);
      if (line[len - 1] == '\n') {
        line[--len] = 0;
      }
      char field[64];
      int len;
      if (sscanf(line, "%63s %n", field, &len) == 1 && *field && field[strlen(field) - 1] == ':') {
        int size;
        if (sscanf(line + len, "%d kB", &size) == 1) {
          if (!strcmp(field, "MemTotal:")) {
            result.push_back(std::make_pair("MemTotal", size));
          } else if (!strcmp(field, "MemFree:")) {
            result.push_back(std::make_pair("MemFree", size));
          } else if (!strcmp(field, "MemAvailable:")) {
            result.push_back(std::make_pair("MemAvailable", size));
          } else if (!strcmp(field, "Buffers:")) {
            result.push_back(std::make_pair("Buffers", size));
          } else if (!strcmp(field, "Cached:")) {
            result.push_back(std::make_pair("Cached", size));
          }
        }
      }
    }
    return result;
  }

  void SystemInfo::updateMemoryUsage() {
    struct mapinfo *smaps;
    struct mapinfo *mi;
    unsigned shared_dirty = 0;
    unsigned shared_clean = 0;
    unsigned private_dirty = 0;
    unsigned private_clean = 0;
    unsigned rss = 0;
    unsigned pss = 0;
    unsigned size = 0;

    smaps = loadMaps();
    if (smaps == 0) {
      fprintf(stderr,"cannot get smaps\n");
      return;
    }
    for (mi = smaps; mi; ) {
      struct mapinfo *last = mi;
      shared_clean += mi->shared_clean;
      shared_dirty += mi->shared_dirty;
      private_clean += mi->private_clean;
      private_dirty += mi->private_dirty;
      rss += mi->rss;
      pss += mi->pss;
      size += mi->size;
      mi = mi->next;
      free(last);
    }

    PropertySystem& propSys = DSS::getInstance()->getPropertySystem();
    PropertyNodePtr debugNode = propSys.createProperty("/config/debug");
    PropertyNodePtr memoryNode = debugNode->createProperty("memory");
    memoryNode->createProperty("PrivateClean")->setIntegerValue(private_clean);
    memoryNode->createProperty("PrivateDirty")->setIntegerValue(private_dirty);
    memoryNode->createProperty("SharedClean")->setIntegerValue(shared_clean);
    memoryNode->createProperty("SharedDirty")->setIntegerValue(shared_dirty);
    memoryNode->createProperty("PSS")->setIntegerValue(pss);
    memoryNode->createProperty("RSS")->setIntegerValue(rss);
    memoryNode->createProperty("Size")->setIntegerValue(size);

    // BOOST_FOREACH doesn't like it without typedef
    typedef std::pair<std::string, unsigned> proc_meminfo_elt;
    BOOST_FOREACH(proc_meminfo_elt elt, parseProcMeminfo()) {
        memoryNode->createProperty(elt.first)->setIntegerValue(elt.second);
    }
  }

#ifndef __APPLE__
  void SystemInfo::enumerateInterfaces() {
    int sock;

    PropertySystem& propSys = DSS::getInstance()->getPropertySystem();
    PropertyNodePtr hostNode = propSys.createProperty("/system/host");
    PropertyNodePtr interfacesNode = hostNode->createProperty("interfaces");

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
      log("socket()", lsError);
      return;
    }

    //Retrieve the available list of network interface cards and their names and indices
    struct if_nameindex *if_name;

    if_name = if_nameindex();
    if (if_name == NULL) {
      log(std::string("if_nameindex: ") + strerror(errno), lsWarning);
      close(sock);
      return;
    }

    for (int i = 0; if_name[i].if_index || if_name[i].if_name != NULL; i++) {
      struct ifreq ifr;
      log(std::string("intf") + intToString(i) + " : " +
          intToString(if_name[i].if_index) + " " + if_name[i].if_name, lsDebug);
      strncpy(ifr.ifr_name, if_name[i].if_name, IFNAMSIZ-1);
      ifr.ifr_name[IFNAMSIZ-1] = '\0';

      std::string ip;
      if(ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        log(std::string("Could not retrieve IP address of interface: ") +
            std::string(if_name[i].if_name), lsInfo);
      } else {
        struct sockaddr* sa = &(ifr.ifr_addr);
        ip = ipToString(sa);
      }

      char mac[32];
      if(ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        log(std::string("Could not retrieve MAC address of interface: ") +
            std::string(if_name[i].if_name), lsInfo);
      } else {
        for(int j=0, k=0; j<6; j++) {
          k+=snprintf(mac+k, sizeof(mac)-k-1, j ? ":%02X" : "%02X",
              (int)(unsigned int)(unsigned char) ifr.ifr_hwaddr.sa_data[j]);
        }
        mac[sizeof(mac)-1]='\0';
      }

      std::string netmask;
      if(ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) {
        log(std::string("Could not retrieve netmask address of interface: ") +
            std::string(if_name[i].if_name), lsInfo);
      } else {
        struct sockaddr* sa = &(ifr.ifr_netmask);
        netmask = ipToString(sa);
      }

      PropertyNodePtr intfNode = interfacesNode->createProperty(ifr.ifr_name);
      intfNode->createProperty("mac")->setStringValue(mac);
      intfNode->createProperty("ip")->setStringValue(ip);
      intfNode->createProperty("netmask")->setStringValue(netmask);
    }

    if_freenameindex(if_name);
    close(sock);
  } // enumerateInterfaces
#else
void SystemInfo::enumerateInterfaces() {
  PropertySystem& propSys = DSS::getInstance()->getPropertySystem();
  PropertyNodePtr hostNode = propSys.createProperty("/system/host");
  PropertyNodePtr interfacesNode = hostNode->createProperty("interfaces");


  struct ifaddrs *pIfAddrs = NULL;
  if(getifaddrs(&pIfAddrs) == 0) {
    struct ifaddrs* pCurrent = pIfAddrs;
    while(pCurrent != NULL) {
      PropertyNodePtr intfNode = interfacesNode->createProperty(pCurrent->ifa_name);
      if(pCurrent->ifa_addr->sa_family == AF_LINK) {
        struct sockaddr_dl* addrDL = (struct sockaddr_dl*)pCurrent->ifa_addr;
        char* mac = link_ntoa(addrDL);
        if(mac != NULL) {
          std::string macStr = mac;
          std::string ifName = pCurrent->ifa_name;
          if(macStr != ifName) {
            if(beginsWith(macStr, ifName)) {
              macStr = macStr.substr(ifName.length());
              if(beginsWith(macStr, ":")) {
                macStr = macStr.substr(1);
              }
            }
            intfNode->createProperty("mac")->setStringValue(macStr);
          }
        }
      } else {
        intfNode->createProperty("ip")->setStringValue(ipToString(pCurrent->ifa_addr));
        if(pCurrent->ifa_netmask != NULL) {
          intfNode->createProperty("netmask")->setStringValue(ipToString(pCurrent->ifa_netmask));
        }
      }

      pCurrent = pCurrent->ifa_next;
    }
  }
  freeifaddrs(pIfAddrs);
}
#endif

  std::vector<std::string> SystemInfo::sysinfo() {
    std::vector<std::string> version;
    std::string pline;
    std::string filename;
    try {
      filename = "/proc/cpuinfo";
      if (boost::filesystem::exists(filename)) {
        std::ifstream in(filename.c_str());
        if (in.is_open()) {
          while (!in.eof()) {
            std::getline(in, pline);
            if (pline.substr(0, 8) == "Hardware") {
              version.push_back("Hardware:" + pline.substr(pline.find(':') + 1));
            } else if (pline.substr(0, 8) == "Revision") {
              version.push_back("Revision:" + pline.substr(pline.find(':') + 1));
            } else if (pline.substr(0, 6) == "Serial") {
              version.push_back("Serial:" + pline.substr(pline.find(':') + 1));
            }
          }
          in.close();
        }
      }
      filename = "/sys/class/net/eth0/address";
      if (boost::filesystem::exists(filename)) {
        std::ifstream in(filename.c_str());
        if (in.is_open()) {
          std::getline(in, pline);
          in.close();
        }
        version.push_back("EthernetID:" + pline);
      }
      filename = "/var/lib/dbus/machine-id";
      if (boost::filesystem::exists(filename)) {
        std::ifstream in(filename.c_str());
        if (in.is_open()) {
          std::getline(in, pline);
          in.close();
        }
        version.push_back("MachineID:" + pline);
      }
      filename = "/proc/version";
      if (boost::filesystem::exists(filename)) {
        std::ifstream in(filename.c_str());
        if (in.is_open()) {
          std::getline(in, pline);
          in.close();
        }
        version.push_back("Kernel:" + pline);
      }
    } catch (...) {}
    return version;
  }

} // namespace dss

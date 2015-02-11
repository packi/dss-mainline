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

#include "src/logger.h"
#include "src/dss.h"
#include "src/propertysystem.h"
#include "src/base.h"

namespace dss {
  __DEFINE_LOG_CHANNEL__(SystemInfo, lsInfo)

  void SystemInfo::collect() {
    enumerateInterfaces();
  } // collect

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

} // namespace dss

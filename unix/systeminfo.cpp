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

#include "systeminfo.h"

#ifdef linux
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

#include <arpa/inet.h>

#ifdef __APPLE__
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_dl.h>
#endif

#include <cstring>

#include "core/logger.h"
#include "core/dss.h"
#include "core/propertysystem.h"
#include "core/base.h"

namespace dss {

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
      Logger::getInstance()->log("Unknown address type");
    }
    return ipBuf;
  } // ipToString

#ifndef __APPLE__
  void SystemInfo::enumerateInterfaces() {
    struct ifreq* ifr;
    struct ifconf ifc;
    int sock;
    char buf[1024];

    PropertySystem& propSys = DSS::getInstance()->getPropertySystem();
    PropertyNodePtr hostNode = propSys.createProperty("/system/host");
    PropertyNodePtr interfacesNode = hostNode->createProperty("interfaces");

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
      perror("socket()");
      return;
    }

    memset(&ifc, '\0', sizeof(ifc));
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if(ioctl(sock,  SIOCGIFCONF, &ifc) < 0) {
      perror("ioctl(SIOCGIFCONF)");
      return;
    }

    ifr = ifc.ifc_req;
    int numIntfs = ifc.ifc_len / sizeof(struct ifreq);
    for(int iInterface = 0; iInterface < numIntfs; iInterface++) {
      struct ifreq* pRequest = &ifr[iInterface];
      struct sockaddr* sa = &(pRequest->ifr_addr);

      std::string ip = ipToString(sa);

      char mac[32];
      for(int j=0, k=0; j<6; j++) {
          k+=snprintf(mac+k, sizeof(mac)-k-1, j ? ":%02X" : "%02X",
              (int)(unsigned int)(unsigned char)pRequest->ifr_hwaddr.sa_data[j]);
      }
      mac[sizeof(mac)-1]='\0';

      if(ioctl(sock, SIOCGIFNETMASK, pRequest) < 0) {
        perror("ioctl(SIOCGIFNETMASK)");
        continue;
      }
      sa = &(pRequest->ifr_netmask);
      std::string netmask = ipToString(sa);

      PropertyNodePtr intfNode = interfacesNode->createProperty(pRequest->ifr_name);
      intfNode->createProperty("mac")->setStringValue(mac);
      intfNode->createProperty("ip")->setStringValue(ip);
      intfNode->createProperty("netmask")->setStringValue(netmask);
    }

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

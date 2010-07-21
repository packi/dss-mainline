/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

#ifndef NEUROBONJOUR_H
#define NEUROBONJOUR_H

#include "thread.h"

#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>


#ifdef HAVE_CONFIG_H
#include "config.h"

  #ifdef WITH_BONJOUR
    #ifdef HAVE_AVAHI
      #define USE_AVAHI
    #elif defined HAVE_DNS_SD
      #define USE_DNS_SD
    #else
      #error "Need either AVAHI or DNS_SD"
    #endif
  #endif
#endif // HAVE_CONFIG_H

#ifdef USE_AVAHI
  #include <avahi-client/client.h>
  #include <avahi-client/publish.h>

  #include <avahi-common/alternative.h>
  #include <avahi-common/simple-watch.h>
#endif

#ifdef USE_DNS_SD
  #include <dns_sd.h>
#endif

namespace dss {

  /**
	  @author Patrick Staehlin <me@packi.ch>
  */
  class BonjourHandler : public Thread {
    private:
#ifdef USE_AVAHI
#endif
#ifdef USE_DNS_SD
      DNSServiceRef m_RegisterReference;
#endif
    public:
      BonjourHandler();
      virtual ~BonjourHandler();

      virtual void execute();
      void quit();

  }; //  Bonjour

}

#endif


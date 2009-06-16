#ifndef NEUROBONJOUR_H
#define NEUROBONJOUR_H

#include "thread.h"

#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>


#ifdef HAVE_CONFIG_H
#include "config.h"

  #ifdef HAVE_AVAHI
    #define USE_AVAHI
  #elif defined HAVE_DNS_SD
    #define USE_DNS_SD
  #else
    #error "Need either AVAHI or DNS_SD"
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

  }; //  Bonjour

}

#endif


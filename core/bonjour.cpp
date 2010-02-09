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

#include "bonjour.h"
#include "core/propertysystem.h"
#include "core/dss.h"
#include "core/base.h"
#include "core/logger.h"
#include "core/model/apartment.h"

#include <arpa/inet.h>

#include <iostream>
#include <stdexcept>

#ifdef USE_AVAHI
  #include <avahi-common/malloc.h>
  #include <avahi-common/error.h>
  #include <avahi-common/timeval.h>
#endif // USE_AVAHI


namespace dss {

  BonjourHandler::BonjourHandler()
    : Thread("BonjourHandler")
  {
  } // ctor


  BonjourHandler::~BonjourHandler()
  { } // dtor

#ifdef USE_DNS_SD
  void RegisterCallback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    DNSServiceErrorType errorCode,
    const char *name,
    const char *regtype,
    const char *domain,
    void *context )
  {
    if(errorCode != kDNSServiceErr_NoError) {
      Logger::getInstance()->log("error received in browse callback", lsError);
    }
  } // registerCallback
#endif


  const char* TheMDNSRegisterType = "_dssweb._tcp";

#ifdef USE_AVAHI
static AvahiEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;
static char* name;

static void create_services(AvahiClient *c);

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
    assert(g == group || group == NULL);
    group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            Logger::getInstance()->log(std::string("Service '") + name + "' successfully established.", lsInfo);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(name);
            avahi_free(name);
            name = n;

            Logger::getInstance()->log(std::string("Service name collision, renaming service to '") + name + "'", lsError);

            /* And recreate the services */
            create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            Logger::getInstance()->log(std::string("Entry group failure: ") + avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))), lsError);

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

static void create_services(AvahiClient *c) {
    char *n;
    int ret;
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
		Logger::getInstance()->log(std::string("avahi_entry_group_new() failed: ") + avahi_strerror(avahi_client_errno(c)), lsError);
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        Logger::getInstance()->log(std::string("Adding service '") + name + "'", lsInfo);

        /* We will now add two services and one subtype to the entry
         * group. The two services have the same name, but differ in
         * the service type (IPP vs. BSD LPR). Only services with the
         * same name should be put in the same entry group. */

        int serverPort = DSS::getInstance()->getPropertySystem().getIntValue("/config/subsystems/WebServer/ports");

        /* Add the service for IPP */
        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, name, "_dssweb._tcp", NULL, NULL, serverPort, NULL, NULL, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            Logger::getInstance()->log(std::string("Failed to add _dssweb._tcp service: ") + avahi_strerror(ret), lsError);
            goto fail;
        }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
            Logger::getInstance()->log(std::string("Failed to commit entry group: ") + avahi_strerror(ret), lsError);
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name(name);
    avahi_free(name);
    name = n;

    Logger::getInstance()->log(std::string("Service name collision, renaming service to '") + name + "'", lsInfo);

    avahi_entry_group_reset(group);

    create_services(c);
    return;

fail:
    avahi_simple_poll_quit(simple_poll);
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(c);

    /* Called whenever the client or server state changes */

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:

            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
            create_services(c);
            break;

        case AVAHI_CLIENT_FAILURE:

            Logger::getInstance()->log(std::string("Client failure: ") + avahi_strerror(avahi_client_errno(c)), lsError);
            avahi_simple_poll_quit(simple_poll);

            break;

        case AVAHI_CLIENT_S_COLLISION:

            /* Let's drop our registered services. When the server is back
             * in AVAHI_SERVER_RUNNING state we will register them
             * again with the new host name. */

        case AVAHI_CLIENT_S_REGISTERING:

            /* The server records are now being established. This
             * might be caused by a host name change. We need to wait
             * for our own records to register until the host name is
             * properly esatblished. */

            if (group)
                avahi_entry_group_reset(group);

            break;

        case AVAHI_CLIENT_CONNECTING:
            ;
    }
}


#endif


  void BonjourHandler::execute() {

#ifdef USE_AVAHI
    AvahiClient *client = NULL;
    int error;

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        Logger::getInstance()->log("Failed to create simple poll object.", lsError);
        goto fail;
    }

    name = avahi_strdup(DSS::getInstance()->getApartment().getName().c_str());

    /* Allocate a new client */
    client = avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0 , client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client) {
        Logger::getInstance()->log(std::string("Failed to create client: ") + avahi_strerror(error), lsError);
        goto fail;
    }

    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);

fail:

    /* Cleanup things */

    if (client)
        avahi_client_free(client);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    avahi_free(name);

#endif
#ifdef USE_DNS_SD
    DNSServiceErrorType err;
    int serverPort = DSS::getInstance()->getPropertySystem().getIntValue("/config/subsystems/WebServer/ports");

    memset(&m_RegisterReference, '\0', sizeof(m_RegisterReference));

    err =
    DNSServiceRegister(
      &m_RegisterReference,
      kDNSServiceFlagsDefault,
      0,
      NULL,
      TheMDNSRegisterType,
      NULL,
      NULL,
      htons(serverPort),
      0,
      NULL,
      &RegisterCallback,
      NULL
    );

    if(err != kDNSServiceErr_NoError) {
      throw std::runtime_error("error registering service");
    }
#endif
  } // execute

}

#include "bonjour.h"
#include "core/propertysystem.h"
#include "core/dss.h"
#include "core/model.h"
#include "core/base.h"

#include <arpa/inet.h>

#include <iostream>
#include <stdexcept>

#ifdef USE_AVAHI
  #include <avahi-common/malloc.h>
  #include <avahi-common/error.h>
  #include <avahi-common/timeval.h>
#endif // USE_AVAHI


using namespace std;

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
      cerr << "error received in browse callback" << endl;
    }
  } // RegisterCallback
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
            fprintf(stderr, "Service '%s' successfully established.\n", name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(name);
            avahi_free(name);
            name = n;

            fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

            /* And recreate the services */
            create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

static void create_services(AvahiClient *c) {
    char *n, r[128];
    int ret;
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
            fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        fprintf(stderr, "Adding service '%s'\n", name);

        /* Create some random TXT data */
        snprintf(r, sizeof(r), "random=%i", rand());

        /* We will now add two services and one subtype to the entry
         * group. The two services have the same name, but differ in
         * the service type (IPP vs. BSD LPR). Only services with the
         * same name should be put in the same entry group. */

        int serverPort = StrToInt(DSS::GetInstance()->GetPropertySystem().GetStringValue("/config/subsystems/WebServer/ports"));

        /* Add the service for IPP */
        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, name, "_dssweb._tcp", NULL, NULL, serverPort, "test=blah", r, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            fprintf(stderr, "Failed to add _dssweb._tcp service: %s\n", avahi_strerror(ret));
            goto fail;
        }

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
            fprintf(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
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

    fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

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

            fprintf(stderr, "Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
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


  void BonjourHandler::Execute() {

    int serverPort = StrToInt(DSS::GetInstance()->GetPropertySystem().GetStringValue("/config/subsystems/WebServer/ports"));
#ifdef USE_AVAHI
    AvahiClient *client = NULL;
    int error;
    struct timeval tv;

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        fprintf(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    name = avahi_strdup(DSS::GetInstance()->GetApartment().GetName());

    /* Allocate a new client */
    client = avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0 , client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client) {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
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
      throw runtime_error("error registering service");
    }
#endif
  } // Execute

}


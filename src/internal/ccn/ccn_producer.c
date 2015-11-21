#include <config.h>
#include <LongBow/runtime.h>

#include <time.h>

#include <ccnx/api/ccnx_Portal/ccnx_Portal.h>
#include <ccnx/api/ccnx_Portal/ccnx_PortalRTA.h>

#include <parc/security/parc_Security.h>
#include <parc/security/parc_PublicKeySignerPkcs12Store.h>
#include <parc/security/parc_IdentityFile.h>

#include <ccnx/common/ccnx_Name.h>

#include "ccn_consumer.h"
#include "ccn_producer.h"

// TODO: API: listen (on separate thread) and pass messages to function pointer
// TODO: create this with a function pointer (callback)

struct producer_portal {
    CCNxName *prefix;
    CCNxPortal *portal;
}

// TOOD: we should really return an Optional here
ProducerPortal *
producerPortal_Create(char *prefix)
{
    parcSecurity_Init();

    CCNxPortalFactory *factory = setupPortalFactory();

    ProducerPortal *producer = (ProducerPortal *) malloc(sizeof(ProducerPortal));
    producer->portal = ccnxPortalFactory_CreatePortal(factory, ccnxPortalRTA_Message, &ccnxPortalAttributes_Blocking);
    producer->prefix = ccnxName_CreateFromURI(prefix);

    if (ccnxPortal_Listen(producer->portal, producer->prefix)) {
        ccnxPortalFactory_Release(&factory);
        return producer;
    } else {
        ccnxPortalFactory_Release(&factory);
        free(producer);
        return NULL;
    }
}

// TODO: this shouldn't be an interest... but a wrapper for Name+Payload (RPC plus body?)
// TODO: should this be an ActorMessage?
PARCBuffer *
producerPortal_Get(ProducerPortal *producer)
{
    CCNxMetaMessage *request = ccnxPortal_Receive(portal);

    if (request == NULL) {
        return NULL;
    }

    CCNxInterest *interest = ccnxMetaMessage_GetInterest(request);
    PARCBuffer *buffer = parcBuffer_Acquire(ccnxInterest_GetPayload(interest));
    ccnxMetaMessage_Release(&request);

    return buffer;
}


    // if (interest != NULL) {
    //     CCNxName *interestName = ccnxInterest_GetName(interest);
    //
    //     if (ccnxName_Equals(interestName, contentName)) {
    //
    //         PARCBuffer *payload = makePayload();
    //
    //         CCNxContentObject *contentObject = ccnxContentObject_CreateWithDataPayload(contentName, payload);
    //
    //         CCNxMetaMessage *message = ccnxMetaMessage_CreateFromContentObject(contentObject);
    //
    //         if (ccnxPortal_Send(portal, message) == false) {
    //             fprintf(stderr, "ccnxPortal_Send failed: %d\n", ccnxPortal_GetError(portal));
    //         }
    //
    //         ccnxMetaMessage_Release(&message);
    //
    //         parcBuffer_Release(&payload);
    //     } else if (ccnxName_Equals(interestName, goodbye)) {
    //         break;
    //     }
    // }
    // ccnxMetaMessage_Release(&request);

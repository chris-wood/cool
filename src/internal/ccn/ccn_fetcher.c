#include <config.h>
#include <stdio.h>

#include <LongBow/runtime.h>

#include <ccnx/api/ccnx_Portal/ccnx_Portal.h>

#include "ccn_common.h"
#include "ccn_fetcher.h"

// TODO; API: non-blocking get, including a payload if it's an actor message (that's it)

struct ccn_fetcher {
    CCNxName *prefix;
    CCNxPortal *portal;
}

CCNFetcher *
consumerPortal_Create(char *prefix)
{
    parcSecurity_Init();

    CCNxPortalFactory *factory = setupConsumerFactory();

    CCNFetcher *consumer = (CCNFetcher *) malloc(sizeof(CCNFetcher));
    consumer->portal = ccnxPortalFactory_CreatePortal(factory, ccnxPortalRTA_Message, &ccnxPortalAttributes_Blocking);

    assertNotNull(consumer->portal, "Expected a non-null CCNxPortal pointer.");

    // TODO: lci:/ was the prefix
    consumer->prefix = ccnxName_CreateFromURI(prefix);

    return consumer;
}

// TODO: get -- returns COOLMessage

//     CCNxInterest *interest = ccnxInterest_CreateSimple(name);
//     ccnxName_Release(&name);
//
//     CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);
//
//     if (ccnxPortal_Send(portal, message)) {
//         while (ccnxPortal_IsError(portal) == false) {
//             CCNxMetaMessage *response = ccnxPortal_Receive(portal);
//             if (response != NULL) {
//                 if (ccnxMetaMessage_IsContentObject(response)) {
//                     CCNxContentObject *contentObject = ccnxMetaMessage_GetContentObject(response);
//
//                     PARCBuffer *payload = ccnxContentObject_GetPayload(contentObject);
//
//                     char *string = parcBuffer_ToString(payload);
//                     printf("%s\n", string);
//                     parcMemory_Deallocate((void **)&string);
//
//                     break;
//                 }
//             }
//             ccnxMetaMessage_Release(&response);
//         }
//     }
//
//     ccnxPortal_Release(&portal);
//
//     ccnxPortalFactory_Release(&factory);
//
//     parcSecurity_Fini();
//     return 0;
// }

#include <stdio.h>

#include <LongBow/runtime.h>

#include <ccnx/api/ccnx_Portal/ccnx_Portal.h>
#include <ccnx/api/ccnx_Portal/ccnx_PortalRTA.h>

#include <parc/algol/parc_Buffer.h>

#include <parc/security/parc_Security.h>

#include "ccn_common.h"
#include "ccn_fetcher.h"

// TODO; API: non-blocking get, including a payload if it's an actor message (that's it)

struct ccn_fetcher {
    CCNxPortal *portal;
};

CCNFetcher *
ccnFetcher_Create()
{
    parcSecurity_Init();

    CCNxPortalFactory *factory = setupConsumerFactory();

    CCNFetcher *consumer = (CCNFetcher *) malloc(sizeof(CCNFetcher));
    consumer->portal = ccnxPortalFactory_CreatePortal(factory, ccnxPortalRTA_Message);

    assertNotNull(consumer->portal, "Expected a non-null CCNxPortal pointer.");

    return consumer;
}

cJSON *
ccnFetcher_Fetch(CCNFetcher *fetcher, char *nameString, cJSON *message)
{
    CCNxName *name = ccnxName_CreateFromCString(nameString);
    if (name != NULL) {
        PARCBuffer *buffer = NULL;
        if (message != NULL) {
            char *payload = cJSON_Print(message);
            size_t payloadSize = strlen(payload);
            buffer = parcBuffer_Allocate(payloadSize + 1);
            parcBuffer_Flip(parcBuffer_PutArray(buffer, payloadSize, payload));
        }

        CCNxInterest *interest = ccnxInterest_CreateSimple(name);
        if (buffer != NULL) {
            ccnxInterest_SetPayloadAndId(interest, buffer);
            PARCBuffer *realPayload = ccnxInterest_GetPayload(interest);
        }

        CCNxMetaMessage *message = ccnxMetaMessage_CreateFromInterest(interest);
        cJSON *response = cJSON_Parse("{}");

        if (ccnxPortal_Send(fetcher->portal, message, CCNxStackTimeout_Never)) {
            while (ccnxPortal_IsError(fetcher->portal) == false) {
                CCNxMetaMessage *responseObject = ccnxPortal_Receive(fetcher->portal, CCNxStackTimeout_Never);
                if (responseObject != NULL) {
                    if (ccnxMetaMessage_IsContentObject(responseObject)) {
                        CCNxContentObject *contentObject = ccnxMetaMessage_GetContentObject(responseObject);

                        PARCBuffer *payload = ccnxContentObject_GetPayload(contentObject);

                        char *bufferString = parcBuffer_ToString(payload);
                        response = cJSON_Parse(bufferString);
                        parcBuffer_Release(&payload);

                        break;
                    }
                }
                ccnxMetaMessage_Release(&responseObject);
            }
        }

        return response;
    }

    return NULL;
}

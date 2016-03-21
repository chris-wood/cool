#include <LongBow/runtime.h>

#include <time.h>

#include <stdio.h>

#include <ccnx/api/ccnx_Portal/ccnx_Portal.h>
#include <ccnx/api/ccnx_Portal/ccnx_PortalRTA.h>

#include <parc/security/parc_Security.h>

#include <ccnx/common/ccnx_Name.h>

#include "ccn_common.h"
#include "ccn_producer.h"

// TODO: API: listen (on separate thread) and pass messages to function pointer
// TODO: create this with a function pointer (callback)

struct ccn_producer {
    CCNxName *prefix;
    CCNxPortal *portal;

    void *callbackMetadata;
    cJSON *(*callback)();
};

// TOOD: we should really return an Optional here
CCNProducer *
ccnProducer_Create(char *prefix, void *callbackMetadata, cJSON *(*callback)(void *, cJSON *))
{
    parcSecurity_Init();

    CCNxPortalFactory *factory = setupConsumerFactory();

    CCNProducer *producer = (CCNProducer *) malloc(sizeof(CCNProducer));
    producer->portal = ccnxPortalFactory_CreatePortal(factory, ccnxPortalRTA_Message);
    producer->prefix = ccnxName_CreateFromURI(prefix);

    producer->callback = callback;
    producer->callbackMetadata = callbackMetadata;

    if (ccnxPortal_Listen(producer->portal, producer->prefix, 86400, CCNxStackTimeout_Never)) {
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
cJSON *
producerPortal_Get(CCNProducer *producer, CCNxName **name)
{
    CCNxMetaMessage *request = ccnxPortal_Receive(producer->portal, CCNxStackTimeout_Never);

    if (request == NULL) {
        return NULL;
    }

    CCNxInterest *interest = ccnxMetaMessage_GetInterest(request);

    printf("Got something! %s\n", ccnxInterest_ToString(interest));

    // TODO:  the interest paylod is empty... wtf.

    *name = ccnxName_CreateFromURI(ccnxName_ToString(ccnxInterest_GetName(interest)));
    PARCBuffer *buffer = parcBuffer_Acquire(ccnxInterest_GetPayload(interest));
    printf("Payload length = %zu\n", parcBuffer_Remaining(buffer));
    char *bufferString = parcBuffer_ToString(buffer);
    printf("buffer from payload = %s\n", bufferString);
    cJSON *message = cJSON_Parse(bufferString);

    parcBuffer_Release(&buffer);
    ccnxMetaMessage_Release(&request);

    printf("message: %s\n", bufferString);

    // CBuffer *cbuffer = cbuffer_Create();
    // cbuffer_AppendBytes(cbuffer, parcBuffer_Overlay(buffer), parcBuffer_Remaining(buffer));
    // parcBuffer_Release(&buffer);

    return message;
}

void
producerPortal_Put(CCNProducer *producer, CCNxName *name, cJSON *buffer) // interest is the response
{
    char *bufferString = cJSON_Print(buffer);
    PARCBuffer *responsePayload = parcBuffer_WrapCString(bufferString);

    CCNxContentObject *response = ccnxContentObject_CreateWithDataPayload(name, responsePayload);
    CCNxMetaMessage *message = ccnxMetaMessage_CreateFromContentObject(response);

    if (ccnxPortal_Send(producer->portal, message, CCNxStackTimeout_Never) == false) {
        fprintf(stderr, "ccnxPortal_Send failed: %d\n", ccnxPortal_GetError(producer->portal));
    }
}

void
ccnProducer_Run(CCNProducer *producer)
{
    for (;;) {
        CCNxName *name = NULL;
        cJSON *message = producerPortal_Get(producer, &name);
        if (message != NULL) {
            printf("Preparing the response... %d %d\n", producer->callback, producer->callbackMetadata);
            cJSON *response = producer->callback(producer->callbackMetadata, message);
            printf("sending it back\n");
            producerPortal_Put(producer, name, response);
            printf("Sent the response back!\n");
        } else {
            printf("Sending empty\n");
            cJSON *emptyResponse = cJSON_CreateString("Invalid message");
            producerPortal_Put(producer, name, emptyResponse);
            printf("Sent the response back!\n");
        }
    }
}


/// REFERENCE CODE
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

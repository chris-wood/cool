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

    // TODO: add handle to the repo here

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
    producer->prefix = ccnxName_CreateFromCString(prefix);

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

cJSON *
producerPortal_Get(CCNProducer *producer, CCNxName **name)
{
    CCNxMetaMessage *request = ccnxPortal_Receive(producer->portal, CCNxStackTimeout_Never);

    if (request == NULL) {
        return NULL;
    }

    CCNxInterest *interest = ccnxMetaMessage_GetInterest(request);

    // Save the name so that the entity can act on it
    *name = ccnxName_CreateFromCString(ccnxName_ToString(ccnxInterest_GetName(interest)));
    PARCBuffer *buffer = parcBuffer_Acquire(ccnxInterest_GetPayload(interest));
    char *bufferString = parcBuffer_ToString(buffer);
    cJSON *message = cJSON_Parse(bufferString);

    parcBuffer_Release(&buffer);
    ccnxMetaMessage_Release(&request);

    return message;
}

void
producerPortal_Put(CCNProducer *producer, CCNxName *name, cJSON *buffer) // interest is the response
{
    char *bufferString = cJSON_Print(buffer);
    PARCBuffer *responsePayload = parcBuffer_WrapCString(bufferString);

    CCNxContentObject *response = ccnxContentObject_CreateWithNameAndPayload(name, responsePayload);
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
            cJSON *response = producer->callback(producer->callbackMetadata, message);
            producerPortal_Put(producer, name, response);
        } else {
            cJSON *emptyResponse = cJSON_CreateString("Invalid message");
            producerPortal_Put(producer, name, emptyResponse);
        }
    }
}

bool
ccnProducer_Publish(CCNProducer *producer, CBuffer *data)
{
    // TODO: insert into the repo
}

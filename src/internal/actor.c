#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../cool_types.h"
#include "channel.h"
#include "actor.h"

#include "ccn/ccn_producer.h"

typedef struct actor_message_queue ActorMessageQueue;

static size_t _actorId;

struct actor_message_queue {
    Channel *channel;
};

struct local_actor {
    ActorMessageQueue *inputQueue;
    cJSON *(*callback)(void *metadata, cJSON *message);
    void *metadata;
};
typedef struct local_actor LocalActor;

struct global_actor {
    Actor *actor;
    ProducerPortal *portal;
};
typedef struct global_actor GlobalActor;

struct actor {
    void *instance;
    ActorInterface *interface;
    ActorID id;
};

static cJSON *localActor_SendMessageSync(LocalActor *actor, cJSON *message);
static cJSON *globalActor_SendMessageSync(GlobalActor *actor, cJSON *message);
static void localActor_SendMessageAsync(LocalActor *actor, cJSON *message);
static void globalActor_SendMessageAsync(GlobalActor *actor, cJSON *message);
static void globalActor_Start(GlobalActor *actor);
static void localActor_Start(LocalActor *actor);
static void globalActor_Run(GlobalActor *actor);
static void localActor_Run(LocalActor *actor);

ActorInterface *LocalActorInterface = &(ActorInterface) {
    .start = (void (*)(void *)) localActor_Start,
    .getID = (ActorID (*)(void *)) actor_GetID,
    .sendMessageAsync = (void (*)(void *, cJSON *)) localActor_SendMessageAsync,
    .sendMessageSync = (cJSON *(*)(void *, cJSON *)) localActor_SendMessageSync
};

ActorInterface *GlobalActorInterface = &(ActorInterface) {
    .start = (void (*)(void *)) globalActor_Start,
    .getID = (ActorID (*)(void *)) actor_GetID,
    .sendMessageAsync = (void (*)(void *, cJSON *)) globalActor_SendMessageAsync,
    .sendMessageSync = (cJSON *(*)(void *, cJSON *)) globalActor_SendMessageSync
};

// TODO: implement this generic actor interface
// typedef struct actor_implementation {
//     void *instance;
//
//     void (*start)(void);
//     void (*sendMessageAsync)(void);
//     void *(*sendMessageAsync)(void);
//     ActorID (*getID)(void);
// } ActorImplementation;

ActorMessageQueue *
ActorMessageQueue_Create()
{
    ActorMessageQueue *queue = (ActorMessageQueue *) malloc(sizeof(ActorMessageQueue));
    queue->channel = channel_Create();
    return queue;
}

ChannelMessage *
ActorMessageQueue_PushMessage(ActorMessageQueue *queue, ChannelMessage *message)
{
    ChannelMessage *insertedMessage = channel_Enqueue(queue->channel, message);
    return insertedMessage;
}

ChannelMessage *
ActorMessageQueue_PopMessage(ActorMessageQueue *queue)
{
    return channel_Dequeue(queue->channel);
}

Actor *
actor_CreateLocal(void *callbackMetadata, cJSON *(*callback)(void *, cJSON *))
{
    Actor *actor = (Actor *) malloc(sizeof(Actor));
    volatile size_t inc = 1;

    LocalActor *localActor = (LocalActor *) malloc(sizeof(LocalActor));

    localActor->inputQueue = ActorMessageQueue_Create();
    localActor->callback = callback;
    localActor->metadata = callbackMetadata;

    actor->id = __sync_fetch_and_add(&_actorId, inc);
    actor->instance = (void *) localActor;
    actor->interface = LocalActorInterface;

    return actor;
}

Actor *
actor_CreateGlobal(char *name, void *callbackMetadata, cJSON *(*callback)(void *, cJSON *))
{
    // Actor *actor = actor_CreateLocal(callbackMetadata, callback);
    // actor->producer = producerPortal_Create(name);

    // return actor;
    return NULL;
}

static void
localActor_Run(LocalActor *actor)
{
    for (;;) {
        ChannelMessage *message = ActorMessageQueue_PopMessage(actor->inputQueue);
        Signal *thesignal = channelMessage_GetSignal(message);

        signal_Lock(thesignal);
        cJSON *result = actor->callback(actor->metadata, channelMessage_GetPayload(message));

        channelMessage_SetOutput(message, result);
        signal_Notify(thesignal);
        signal_Unlock(thesignal);
    }
}

static void
globalActor_Run(GlobalActor *actor)
{
    for (;;) {
        // TODO
    }
}

void
localActor_Start(LocalActor *actor)
{
    pthread_t *t = (pthread_t *) malloc(sizeof(pthread_t));
    pthread_create(t, NULL, (void *) &localActor_Run, (void *) actor);
}

void
globalActor_Start(GlobalActor *actor)
{
    pthread_t *t = (pthread_t *) malloc(sizeof(pthread_t));
    pthread_create(t, NULL, (void *) &globalActor_Run, (void *) actor);
}

void
globalActor_SendMessageAsync(GlobalActor *actor, cJSON *message)
{
    // TODO
}

// TODO: this should take a callback as an argument, obviously.
void
localActor_SendMessageAsync(LocalActor *actor, cJSON *message)
{
    // We ignore the signal that's returned since we are not waiting
    // for it to complete.
    ChannelMessage *channelMessage = channelMessage_Create(message);
    Signal *thesignal = channelMessage_GetSignal(channelMessage);
    signal_Lock(thesignal);
    ActorMessageQueue_PushMessage(actor->inputQueue, channelMessage);
    signal_Unlock(thesignal);
}

cJSON *
globalActor_SendMessageSync(GlobalActor *actor, cJSON *message)
{
    // TODO
    return NULL;
}

cJSON *
localActor_SendMessageSync(LocalActor *actor, cJSON *message)
{
    ChannelMessage *channelMessage = channelMessage_Create(message);
    Signal *thesignal = channelMessage_GetSignal(channelMessage);
    signal_Lock(thesignal);

    channelMessage = ActorMessageQueue_PushMessage(actor->inputQueue, channelMessage);

    signal_Wait(thesignal, NULL);
    signal_Unlock(thesignal);

    cJSON *output = channelMessage_GetOutput(channelMessage);

    return output;
}

ActorID
actor_GetID(const Actor *actor)
{
    return actor->id;
}

void
actor_Start(Actor *actor)
{
    actor->interface->start(actor->instance);
}

void
actor_SendMessageAsync(Actor *actor, cJSON *message)
{
    actor->interface->sendMessageAsync(actor->instance, message);
}

cJSON *
actor_SendMessageSync(Actor *actor, cJSON *message)
{
    return actor->interface->sendMessageSync(actor->instance, message);
}

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../cool_types.h"
#include "channel.h"
#include "actor.h"

typedef struct actor_message_queue ActorMessageQueue;

static size_t _actorId;

struct actor_message_queue {
    Channel *channel;
};

struct actor {
    ActorID id;
    ActorMessageQueue *inputQueue;
    void *(*callback)(void *metadata, void *message);
    void *metadata;
};

ActorMessageQueue *
ActorMessageQueue_Create()
{
    ActorMessageQueue *queue = (ActorMessageQueue *) malloc(sizeof(ActorMessageQueue));
    queue->channel = channel_Create();
    return queue;
}

ChannelMessage *
ActorMessageQueue_PushMessage(ActorMessageQueue *queue, void *message)
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
actor_CreateLocal(void *callbackMetadata, void *(*callback)(void *, void *))
{
    Actor *actor = (Actor *) malloc(sizeof(Actor));
    volatile size_t inc = 1;

    actor->id = __sync_fetch_and_add(&_actorId, inc);
    actor->inputQueue = ActorMessageQueue_Create();
    actor->callback = callback;
    actor->metadata = callbackMetadata;

    return actor;
}

Actor *
actor_CreateGlobal(char *name, void *callbackMetadata, void *(*callback)(void *, void *))
{
    Actor *actor = actor_CreateLocal(callbackMetadata, callback);
    // actor->producer = TODO: producer portal here!
    return actor;
}

static void
_actor_Run(Actor *actor)
{
    for (;;) {
        ChannelMessage *message = ActorMessageQueue_PopMessage(actor->inputQueue);
        void *result = actor->callback(actor->metadata, message);
        channelMessage_SetOutput(message, result);
        signal_Notify(channelMessage_GetSignal(message));
    }
}

void
actor_Start(Actor *actor)
{
    pthread_t *t = (pthread_t *) malloc(sizeof(pthread_t));
    pthread_create(t, NULL, (void *) &_actor_Run, (void *) actor);
}

void
actor_SendMessageAsync(Actor *actor, void *message)
{
    // We ignore the signal that's returned since we are not waiting
    // for it to complete.
    ActorMessageQueue_PushMessage(actor->inputQueue, message);
}

void *
actor_SendMessageSync(Actor *actor, void *message)
{
    ChannelMessage *channelMessage = ActorMessageQueue_PushMessage(actor->inputQueue, message);

    Signal *signal = channelMessage_GetSignal(channelMessage);
    signal_Wait(signal, NULL);

    void *output = channelMessage_GetOutput(channelMessage);

    return output;
}

ActorID
actor_GetID(const Actor *actor)
{
    return actor->id;
}

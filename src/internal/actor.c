#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../cool_types.h"
#include "channel.h"
#include "actor.h"

typedef struct actor_message_queue ActorMessageQueue;

static size_t _actorId;

struct actor_message {
    ActorID sender;
    ActorID receiver;
};

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
actorMessageQueue_Create()
{
    ActorMessageQueue *queue = (ActorMessageQueue *) malloc(sizeof(ActorMessageQueue));
    queue->channel = channel_Create();
    return queue;
}

ActorMessageQueue *
actorMessageQueue_PushMessage(ActorMessageQueue *queue, void *message)
{
    channel_Enqueue(queue->channel, message);
    return queue;
}

void *
actorMessageQueue_PopMessage(ActorMessageQueue *queue)
{
    return channel_Dequeue(queue->channel);
}

Actor *
actor_Create(void *callbackMetadata, void *(*callback)(void *, void *))
{
    Actor *actor = (Actor *) malloc(sizeof(Actor));
    volatile size_t inc = 1;

    actor->id = __sync_fetch_and_add(&_actorId, inc);
    actor->inputQueue = actorMessageQueue_Create();
    actor->callback = callback;
    actor->metadata = callbackMetadata;

    return actor;
}

static void
_actor_Run(Actor *actor)
{
    for (;;) {
        void *message = actorMessageQueue_PopMessage(actor->inputQueue);
        actor->callback(actor->metadata, message);
    }
}

void
actor_Start(Actor *actor)
{
    pthread_t *t = (pthread_t *) malloc(sizeof(pthread_t));
    pthread_create(t, NULL, (void *) &_actor_Run, (void *) actor);
}

Actor *
actor_SendMessage(Actor *actor, void *message)
{
    actorMessageQueue_PushMessage(actor->inputQueue, message);
    return actor;
}

ActorID
actor_GetID(const Actor *actor)
{
    return actor->id;
}

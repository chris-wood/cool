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
};

ActorMessageQueue *
actorMessageQueue_Create()
{
    ActorMessageQueue *queue = (ActorMessageQueue *) malloc(sizeof(ActorMessageQueue));
    queue->channel = channel_Create();
    return queue;
}

ActorMessageQueue *
actorMessageQueue_PushMessage(ActorMessageQueue *queue, ActorMessage *message)
{
    channel_Enqueue(queue->channel, message);
    return queue;
}

Actor *
actor_Create()
{
    Actor *actor = (Actor *) malloc(sizeof(Actor));
    volatile size_t inc = 1;

    actor->id = __sync_fetch_and_add(&_actorId, inc);
    actor->inputQueue = actorMessageQueue_Create();

    return actor;
}

Actor *
actor_SendMessage(Actor *actor, ActorMessage *message)
{
    actorMessageQueue_PushMessage(actor->inputQueue, message);
    return actor;
}

ActorID
actor_GetID(const Actor *actor)
{
    return actor->id;
}

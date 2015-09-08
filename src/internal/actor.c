#include <pthread.h>

#include "../cool_types.h"
#include "list.h"
#include "actor.h"

struct actor_message {
    ActorID sender;
    ActorID receiver;
};

struct actor_message_queue {
    List *queue;
};

struct actor {
    ActorID id;
    ActorMessageQueue inputQueue;
};

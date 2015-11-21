#include "actor.h"
#include "actor_message.h"

struct actor_message {
    ActorID sender;
    ActorID receiver;

    // TODO: messages have (a) function/actor name (RPC?), body (ayload), sender name (name), and authenticator (signature)
};

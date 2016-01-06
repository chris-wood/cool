#ifndef libcool_internal_actor_
#define libcool_internal_actor_

#include "signal.h"

typedef size_t ActorID;
typedef struct actor Actor;

typedef struct actor_implementation {
    void (*start)(void *);
    void (*sendMessageAsync)(void *, void *);
    void *(*sendMessageSync)(void *, void *);
    ActorID (*getID)(void *);
} ActorInterface;

Actor *actor_CreateLocal(void *metadata, void *(*callback)(void *metadata, void *message));
Actor *actor_CreateGlobal(char *name, void *metadata, void *(*callback)(void *metadata, void *message));

void actor_Start(Actor *actor);
void actor_SendMessageAsync(Actor *actor, void *message);
void *actor_SendMessageSync(Actor *actor, void *message);
ActorID actor_GetID(const Actor *actor);

#endif // libcool_internal_actor_

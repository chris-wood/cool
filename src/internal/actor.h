#ifndef libcool_internal_actor_
#define libcool_internal_actor_

#include <stddef.h>

#include "signal.h"
#include "encoding/cJSON.h"

typedef size_t ActorID;
typedef struct actor Actor;

typedef struct actor_implementation {
    void (*start)(void *);
    void (*sendMessageAsync)(void *, cJSON *);
    cJSON *(*sendMessageSync)(void *, cJSON *);
    ActorID (*getID)(void *);
} ActorInterface;

Actor *actor_CreateLocal(void *metadata, cJSON *(*callback)(void *metadata, cJSON *message));
Actor *actor_CreateGlobal(char *name, void *metadata, cJSON *(*callback)(void *metadata, cJSON *message));

void actor_Start(Actor *actor);
void actor_SendMessageAsync(Actor *actor, cJSON *message);
cJSON *actor_SendMessageSync(Actor *actor, cJSON *message);
ActorID actor_GetID(const Actor *actor);

#endif // libcool_internal_actor_

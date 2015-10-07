#ifndef libcool_internal_actor_
#define libcool_internal_actor_

typedef size_t ActorID;
typedef struct actor_message ActorMessage;
typedef struct actor Actor;

Actor *actor_Create(void *metadata, void *(*callback)(void *metadata, void *message));
void actor_Start(Actor *actor);
void *actor_SendMessageAsync(Actor *actor, void *message);
void *actor_SendMessageSync(Actor *actor, void *message);
ActorID actor_GetID(const Actor *actor);

#endif // libcool_internal_actor_

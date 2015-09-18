#ifndef libcool_internal_actor_
#define libcool_internal_actor_

typedef size_t ActorID;
typedef struct actor_message ActorMessage;
typedef struct actor Actor;

Actor *actor_Create();
Actor *actor_SendMessage(Actor *actor, ActorMessage *message);
ActorID actor_GetID(const Actor *actor);

#endif // libcool_internal_actor_

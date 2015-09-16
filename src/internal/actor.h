#ifndef libcool_internal_actor_
#define libcool_internal_actor_

typedef size_t ActorID;
typedef struct actor_message ActorMessage;

Actor *actor_Create();
Actor *actor_SendMessage(Actor *actor, ActorMessage *message);

#endif // libcool_internal_actor_

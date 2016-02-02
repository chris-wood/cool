#ifndef libcool_internal_channel_
#define libcool_internal_channel_

#include "signal.h"
#include "encoding/cJSON.h"

typedef struct channel Channel;
typedef struct channel_message ChannelMessage;

/**
 * Create a `ChannelNode` from a pointer to an arbitrary thing. It is expected
 * that each element in a `Channel` will be of the same type.
 *
 * @param [in] element A pointer to an element from which to create a node.
 *
 * Example:
 * @code
 * {
 *     TODO
 * }
 * @endcode
 */
Channel *channel_Create();
void channel_Destroy(Channel **channelP);

ChannelMessage *channelMessage_Create(cJSON *element);
ChannelMessage *channel_Enqueue(Channel *channel, ChannelMessage *element);
ChannelMessage *channel_Dequeue(Channel *channel);

// void *channel_GetAtIndex(Channel *channel, size_t index);
// void *channel_RemoveAtIndex(Channel *channel, void *element, size_t index);

Signal *channelMessage_GetSignal(ChannelMessage *message);
void channelMessage_SetOutput(ChannelMessage *message, cJSON *data);
cJSON *channelMessage_GetOutput(ChannelMessage *message);
cJSON *channelMessage_GetPayload(ChannelMessage *message);

#endif // libcool_internal_channel_

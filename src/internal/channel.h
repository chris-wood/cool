#ifndef libcool_internal_channel_
#define libcool_internal_channel_

#include "signal.h"

typedef struct channel Channel;

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
Signal *channel_Enqueue(Channel *channel, void *element);
void *channel_Dequeue(Channel *channel);
void *channel_GetAtIndex(Channel *channel, size_t index);
void *channel_RemoveAtIndex(Channel *channel, void *element, size_t index);

#endif // libcool_internal_channel_

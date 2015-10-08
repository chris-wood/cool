#include <stdlib.h>
#include <pthread.h>

#include "channel.h"

struct channel_entry;

struct channel_entry {
    void *element;
    struct channel_entry *next;
    Signal *signal;

};

typedef struct channel_entry ChannelEntry;

struct channel {
    struct channel_entry *head;
    struct channel_entry *tail;
    size_t size;
    Signal *signal;

    void (*delete)(void **element);
};

void
channelEntry_Destroy(ChannelEntry **nodeP)
{
    ChannelEntry *result = (ChannelEntry *) *nodeP;
    free(result);
    *nodeP = NULL;
}

ChannelEntry *
channelEntry_Create(void *element)
{
    ChannelEntry *result = (ChannelEntry *) malloc(sizeof(ChannelEntry));
    result->element = element;
    result->next = NULL;
    result->signal = signal_Create(result);
    return result;
}

Channel *
channel_Create(void (*delete)(void **element))
{
    Channel *result = (Channel *) malloc(sizeof(Channel));
    result->size = 0;
    result->head = result->tail = NULL;
    result->delete = delete;
    result->signal = signal_Create(result);

    return result;
}

void
channel_Destroy(Channel **channelP)
{
    Channel *channel = (Channel *) *channelP;

    ChannelEntry *current = channel->head;
    for (size_t i = 0; i < channel->size; i++) {
        channel->delete(&(current->element));
        current = current->next;
    }

    signal_Destroy(&channel->signal);

    free(channel);
    *channelP = NULL;
}

Signal *
channel_Enqueue(Channel *channel, void *element)
{
    ChannelEntry *newNode = channelEntry_Create(element);

    signal_Lock(channel->signal);

    if (channel->head == NULL || channel->tail == NULL) {
        channel->head = channel->tail = newNode;
    } else {
        channel->tail->next = newNode;
        channel->tail = newNode;
    }

    channel->size++;

    signal_Notify(channel->signal);

    return newNode->signal;
}

int
channel_IsEmpty(Channel *channel)
{
    return channel->size == 0;
}

void *
channel_Dequeue(Channel *channel)
{
    signal_Wait(channel->signal, (int (*)(void *)) channel_IsEmpty);

    ChannelEntry *target = channel->head;
    void *result = target->element;
    channelEntry_Destroy(&target);

    channel->head = channel->head->next;
    channel->size--;

    signal_Notify(channel->signal);

    return result;
}

void *
channel_GetAtIndex(Channel *channel, size_t index)
{
    ChannelEntry *element = NULL;

    // pthread_mutex_lock(&channel->mutex);
    signal_Lock(channel->signal);

    index = (index % channel->size);

    ChannelEntry *current = channel->head;
    size_t i = 0;
    while (i < index) {
        current = current->next;
        i++;
    }

    element = current;

    // pthread_mutex_unlock(&channel->mutex);
    signal_Unlock(channel->signal);

    return element;
}

void *
channel_RemoveAtIndex(Channel *channel, void *element, size_t index)
{
    // pthread_mutex_lock(&channel->mutex);
    signal_Lock(channel->signal);

    if (channel->head == NULL) {
        // pthread_mutex_unlock(&channel->mutex);
        signal_Unlock(channel->signal);
        return NULL;
    } else {

        index = index % channel->size;
        ChannelEntry *target = channel->head;

        if (index == 0) {
            channel->head = channel->head->next;
        } else {
            size_t i = 0;
            ChannelEntry *prev = NULL;

            while (i < index) {
                prev = target;
                target = target->next;
                i++;
            }
            prev->next = target->next;

            if (index == (channel->size - 1)) {
                channel->tail = prev; // move back
            }
        }

        channel->size--;

        // pthread_mutex_unlock(&channel->mutex);
        signal_Unlock(channel->signal);

        return target;
    }
}

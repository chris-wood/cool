#include <stdlib.h>
#include <pthread.h>

#include "channel.h"

struct channel_message;

struct channel_message {

    void *input;
    void *output;

    struct channel_message *next;
    Signal *signal;
};

struct channel {
    struct channel_message *head;
    struct channel_message *tail;
    size_t size;
    Signal *signal;

    void (*delete)(void **element);
};

void
channelEntry_Destroy(ChannelMessage **nodeP)
{
    ChannelMessage *result = (ChannelMessage *) *nodeP;
    free(result);
    *nodeP = NULL;
}

ChannelMessage *
channelEntry_Create(void *element)
{
    ChannelMessage *result = (ChannelMessage *) malloc(sizeof(ChannelMessage));
    result->input = element;
    result->output = NULL;
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

    ChannelMessage *current = channel->head;
    for (size_t i = 0; i < channel->size; i++) {
        channel->delete(&(current->input));
        channel->delete(&(current->output));
        current = current->next;
    }

    signal_Destroy(&channel->signal);

    free(channel);
    *channelP = NULL;
}

Signal *
channel_Enqueue(Channel *channel, void *element)
{
    ChannelMessage *newNode = channelEntry_Create(element);

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

ChannelMessage *
channel_Dequeue(Channel *channel)
{
    signal_Wait(channel->signal, (int (*)(void *)) channel_IsEmpty);

    ChannelMessage *target = channel->head;

    channel->head = channel->head->next;
    channel->size--;

    signal_Notify(channel->signal);

    return target;
}

void *
channel_GetAtIndex(Channel *channel, size_t index)
{
    ChannelMessage *element = NULL;

    // pthread_mutex_lock(&channel->mutex);
    signal_Lock(channel->signal);

    index = (index % channel->size);

    ChannelMessage *current = channel->head;
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
        ChannelMessage *target = channel->head;

        if (index == 0) {
            channel->head = channel->head->next;
        } else {
            size_t i = 0;
            ChannelMessage *prev = NULL;

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

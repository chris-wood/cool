#include <stdlib.h>
#include <pthread.h>

#include "signal.h"

struct signal {
    void *context;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

Signal *
signal_Create(void *context)
{
    Signal *result = (Signal *) malloc(sizeof(Signal));

    result->context = context;
    pthread_mutex_init(&result->mutex, NULL);
    pthread_cond_init(&result->cond, NULL);

    return result;
}

void
signal_Destroy(Signal **signalP)
{
    Signal *signal = (Signal *) *signalP;
    pthread_mutex_destroy(&signal->mutex);
    pthread_cond_destroy(&signal->cond);

    free(signal);
    *signalP = NULL;
}

void
signal_Lock(Signal *signal)
{
    pthread_mutex_lock(&signal->mutex);
}

void
signal_Unlock(Signal *signal)
{
    pthread_mutex_unlock(&signal->mutex);
}

void
signal_Wait(Signal *signal, int (*condition)(void *state))
{
    pthread_mutex_lock(&signal->mutex);

    while (condition(signal->context) >= 1) {
        pthread_cond_wait(&(signal->cond), &(signal->mutex));
    }
}

void
signal_Notify(Signal *signal)
{
    pthread_cond_signal(&signal->cond);
    pthread_mutex_unlock(&signal->mutex);
}

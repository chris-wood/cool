#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "signal.h"

struct signal {
    void *context;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    long id;
};

static long _signalId = 0;

Signal *
signal_Create(void *context)
{
    Signal *result = (Signal *) malloc(sizeof(Signal));

    result->context = context;
    result->id = _signalId++;
    pthread_mutex_init(&result->mutex, NULL);
    pthread_cond_init(&result->cond, NULL);

    return result;
}

void
signal_Destroy(Signal **signalP)
{
    Signal *thesignal = (Signal *) *signalP;
    pthread_mutex_destroy(&thesignal->mutex);
    pthread_cond_destroy(&thesignal->cond);

    free(thesignal);
    *signalP = NULL;
}

void
signal_Lock(Signal *thesignal)
{
    // printf("locking condition %u\n", thesignal->id);
    pthread_mutex_lock(&thesignal->mutex);
}

void
signal_Unlock(Signal *thesignal)
{
    // printf("unlocking condition %u\n", thesignal->id);
    pthread_mutex_unlock(&thesignal->mutex);
}

void
signal_Notify(Signal *thesignal)
{
    // printf("setting condition %u\n", thesignal->id);
    pthread_cond_signal(&thesignal->cond);
}

void
signal_Wait(Signal *thesignal, int (*condition)(void *state))
{
    // printf("I've been unlocked %u\n", thesignal->id);

    if (condition != NULL) {
        while (condition(thesignal->context) >= 1) {
            pthread_cond_wait(&(thesignal->cond), &(thesignal->mutex));
        }
    } else {
        // printf("waiting on condition %u\n", thesignal->id);
        pthread_cond_wait(&(thesignal->cond), &(thesignal->mutex));
        // printf("cond released %u\n", thesignal->id);
    }
}

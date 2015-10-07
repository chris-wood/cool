#ifndef libcool_internal_signal_
#define libcool_internal_signal_

typedef struct signal Signal;

Signal *signal_Create(void *context);
void signal_Destroy(Signal **signalP);

void signal_Lock(Signal *signal);
void signal_Unlock(Signal *signal);
void signal_Wait(Signal *signal, int (*condition)(void *state));
void signal_Notify(Signal *signal);

#endif // libcool_internal_signal_

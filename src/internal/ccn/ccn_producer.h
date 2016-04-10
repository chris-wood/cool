#ifndef libcool_internal_ccn_producer_
#define libcool_internal_ccn_producer_

#include <stdbool.h>
#include <internal/encoding/cJSON.h>
#include <internal/buffer.h>

struct ccn_producer;
typedef struct ccn_producer CCNProducer;

CCNProducer *ccnProducer_Create(char *prefix, void *callbackMetadata, cJSON *(*callback)(void *, cJSON *));
void ccnProducer_Run(CCNProducer *producer);
bool ccnProducer_Publish(CCNProducer *producer, CBuffer *data);

#endif // libcool_internal_ccn_producer_

#ifndef libcool_internal_ccn_producer_
#define libcool_internal_ccn_producer_

#include <internal/encoding/cJSON.h>

struct ccn_producer;
typedef struct ccn_producer CCNProducer;

CCNProducer *ccnProducer_Create(char *prefix, void *callbackMetadata, cJSON *(*callback)(void *, cJSON *));
void ccnProducer_Run(CCNProducer *producer);

#endif // libcool_internal_ccn_producer_

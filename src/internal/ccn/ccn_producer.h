#ifndef libcool_internal_ccn_producer_
#define libcool_internal_ccn_producer_

#include <internal/encoding/cJSON.h>

struct producer_portal;
typedef struct producer_portal ProducerPortal;

ProducerPortal *producerPortal_Create(char *prefix, void *callbackMetadata, cJSON *(*callback)(void *, cJSON *));
void producePortal_Run(ProducerPortal *producer);

#endif // libcool_internal_ccn_producer_

#ifndef libcool_internal_ccn_fetcher_
#define libcool_internal_ccn_fetcher_

#include <internal/encoding/cJSON.h>

struct ccn_fetcher;
typedef struct ccn_fetcher CCNFetcher;

CCNFetcher *ccnFetcher_Create();

cJSON *ccnFetcher_Fetch(CCNFetcher *fetcher, char *nameString, cJSON *message);

#endif // libcool_internal_ccn_fetcher_

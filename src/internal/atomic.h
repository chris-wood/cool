#include libcool_internal_atomic_
#define libcool_internal_atomic_

#include <asm/atomic.h>

typedef atomic_t AtomicInteger;

AtomicInteger atomicInteger_New();
AtomicInteger atomicInteger_Inc(AtomicInteger *ai);
AtomicInteger atomicInteger_Set(AtomicInteger *ai, size_t value);
AtomicInteger atomicInteger_Add(AtomicInteger *ai, size_t value);

#endif // libcool_internal_atomic_

#include "atomic.h"

AtomicInteger
atomicInteger_New()
{
    return ATOMIC_INIT(0);
}

AtomicInteger
atomicInteger_Inc(AtomicInteger ai)
{
    atomic_inc(&ai);
    return ai;
}

AtomicInteger
atomicInteger_Set(AtomicInteger ai, size_t value)
{
    atomic_set(&ai, value);
    return ai;
}

AtomicInteger
atomicInteger_Add(AtomicInteger ai, size_t value)
{
    atomic_add(value, &ai);
    return ai;
}

// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef _OE_ATOMIC32_H
#define _OE_ATOMIC32_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

#if defined(__linux__)
#define OE_ATOMIC_MEMORY_BARRIER_ACQUIRE() \
    __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define OE_ATOMIC_MEMORY_BARRIER_RELEASE() \
    __atomic_thread_fence(__ATOMIC_RELEASE)
#elif defined(_MSC_VER)
#define OE_ATOMIC_MEMORY_BARRIER_ACQUIRE() _ReadBarrier()
#define OE_ATOMIC_MEMORY_BARRIER_RELEASE() _WriteBarrier()
#else
#error "Unsupported platform"
#endif

#if defined(_MSC_VER)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_mm_pause)
long _InterlockedOr64(long volatile* value, long mask);
__int64 _InterlockedIncrement(long* lpAddend);
long _InterlockedCompareExchange(long volatile* a, long b, long c);
void* _InterlockedCompareExchangePointer(
    void* volatile* Dest,
    void* newptr,
    void* old);
void _mm_pause(void);
#endif

OE_INLINE
bool oe_atomic_compare_and_swap_32(
    uint32_t volatile* dest,
    uint32_t old,
    uint32_t newval)
{
#if defined(__GNUC__)
    bool weak = false;
    return __atomic_compare_exchange_n(
        dest, &old, newval, weak, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#elif defined(_MSC_VER)
    return _InterlockedCompareExchange(
               (volatile long*)dest, (long)newval, (long)old) == old;
#else
#error "unsupported"
#endif
}

/* Atomically fetch the value of given variable */
OE_INLINE uint32_t oe_atomic_load32(volatile uint32_t* x)
{
#if defined(__GNUC__)
    return __atomic_load_n(x, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
    return (uint32_t)_InterlockedOr((volatile long*)x, 0);
#else
#error "unsupported"
#endif
}

/* Atomically increment **x** and return its new value */
OE_INLINE uint32_t oe_atomic_increment32(volatile uint32_t* x)
{
#if defined(__GNUC__)
    return __sync_add_and_fetch(x, 1);
#elif defined(_MSC_VER)
    return (uint32_t)_InterlockedIncrement((long*)x);
#else
#error "unsupported"
#endif
}

OE_INLINE
bool oe_atomic_compare_and_swap_ptr(
    void* volatile* dest,
    void* old,
    void* newptr)
{
#if defined(__GNUC__)
    bool weak = false;
    return __atomic_compare_exchange_n(
        dest, &old, newptr, weak, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#elif defined(_MSC_VER)
    return _InterlockedCompareExchangePointer(dest, newptr, old) == old;
#else
#error "unsupported"
#endif
}

OE_INLINE
void oe_yield_cpu(void)
{
#if defined(__GNUC__)
    asm volatile("pause");
#elif defined(_MSC_VER)
    _mm_pause();
#else
#error "unsupported"
#endif
}

OE_EXTERNC_END

#endif /* _OE_ATOMIC32_H */

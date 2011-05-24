//===---------------------------- cxa_guard.cpp ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "cxxabi.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace __cxxabiv1
{

namespace
{

void abort_message(const char* s)
{
    fputs(s, stderr);
    ::abort();
}

pthread_mutex_t guard_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  guard_cv  = PTHREAD_COND_INITIALIZER;

#if __APPLE__

typedef uint32_t lock_type;

#if __LITTLE_ENDIAN__

inline
lock_type
get_lock(uint64_t x)
{
    return static_cast<lock_type>(x >> 32);
}

inline
void
set_lock(uint64_t& x, lock_type y)
{
    x = static_cast<uint64_t>(y) << 32;
}

#else  // __LITTLE_ENDIAN__

inline
lock_type
get_lock(uint64_t x)
{
    return static_cast<lock_type>(x);
}

inline
void
set_lock(uint64_t& x, lock_type y)
{
    x = y;
}

#endif  // __LITTLE_ENDIAN__

#else  // __APPLE__

typedef bool lock_type;

inline
lock_type
get_lock(uint64_t x)
{
    union
    {
        uint64_t guard;
        uint8_t lock[2];
    } f = {x};
    return f.lock[1] != 0;
}

inline
void
set_lock(uint64_t& x, lock_type y)
{
    union
    {
        uint64_t guard;
        uint8_t lock[2];
    } f = {0};
    f.lock[1] = y;
    x = f.guard;
}

#endif  // __APPLE__

}  // unnamed namespace

extern "C"
{

int __cxa_guard_acquire(uint64_t* guard_object)
{
    char* initialized = (char*)guard_object;
    if (pthread_mutex_lock(&guard_mut))
        abort_message("__cxa_guard_acquire failed to acquire mutex");
    int result = *initialized == 0;
    if (result)
    {
#if __APPLE__
        const lock_type id = pthread_mach_thread_np(pthread_self());
        lock_type lock = get_lock(*guard_object);
        if (lock)
        {
            // if this thread set lock for this same guard_object, abort
            if (lock == id)
                abort_message("__cxa_guard_acquire detected deadlock");
            do
            {
                if (pthread_cond_wait(&guard_cv, &guard_mut))
                    abort_message("__cxa_guard_acquire condition variable wait failed");
                lock = get_lock(*guard_object);
            } while (lock);
            result = *initialized == 0;
            if (result)
                set_lock(*guard_object, id);
        }
        else
            set_lock(*guard_object, id);
#else  // __APPLE__
        while (get_lock(*guard_object))
            if (pthread_cond_wait(&guard_cv, &guard_mut))
                abort_message("__cxa_guard_acquire condition variable wait failed");
        result = *initialized == 0;
        if (result)
            set_lock(*guard_object, true);
#endif  // __APPLE__
    }
    if (pthread_mutex_unlock(&guard_mut))
        abort_message("__cxa_guard_acquire failed to release mutex");
    return result;
}

void __cxa_guard_release(uint64_t* guard_object)
{
    char* initialized = (char*)guard_object;
    if (pthread_mutex_lock(&guard_mut))
        abort_message("__cxa_guard_release failed to acquire mutex");
    *guard_object = 0;
    *initialized = 1;
    if (pthread_mutex_unlock(&guard_mut))
        abort_message("__cxa_guard_release failed to release mutex");
    if (pthread_cond_broadcast(&guard_cv))
        abort_message("__cxa_guard_release failed to broadcast condition variable");
}

void __cxa_guard_abort(uint64_t* guard_object)
{
    if (pthread_mutex_lock(&guard_mut))
        abort_message("__cxa_guard_abort failed to acquire mutex");
    *guard_object = 0;
    if (pthread_mutex_unlock(&guard_mut))
        abort_message("__cxa_guard_abort failed to release mutex");
    if (pthread_cond_broadcast(&guard_cv))
        abort_message("__cxa_guard_abort failed to broadcast condition variable");
}

}  // extern "C"

}  // __cxxabiv1

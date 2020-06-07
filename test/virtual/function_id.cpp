// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/edger8r/host.h>
#include "atomic32.h"
#include "enclave_impl.h"

extern "C"
{
    oe_result_t oe_register_ecall_function_names(
        oe_enclave_t* enclave,
        const char** function_names,
        uint32_t function_names_count)
    {
        enclave->ecall_function_names = function_names;
        enclave->num_ecall_function_names = function_names_count;
        return OE_OK;
    }

    oe_result_t oe_get_ecall_function_id(
        oe_enclave_t* enclave,     /* in */
        const char* function_name, /* in */
        uint32_t* uid,             /* in/out */
        uint32_t* function_id)     /* in/out */
    {
        static uint32_t uid_count = 0;
        oe_result_t result = OE_FAILURE;
        uint32_t uid_value = 0;
        uint32_t cache_size = 0;
        uint32_t* cache = 0;
        bool locked = false;

        // First check whether uid has been initialized.
        // Use atomic operations to fetch the value since reading uint32_t
        // memory is not guaranteed to be atomic on all platforms. Additionally
        // use acquire-release pattern to set uid.
        uid_value = oe_atomic_load32(uid);
        OE_ATOMIC_MEMORY_BARRIER_ACQUIRE();
        if (uid_value == (uint32_t)-1)
        {
            uid_value = oe_atomic_increment32(&uid_count);
            OE_ATOMIC_MEMORY_BARRIER_RELEASE();
            // Atomic store instead of compare and swap would suffice too.
            if (!oe_atomic_compare_and_swap_32(uid, (uint32_t)-1, uid_value))
                goto done;
        }

        // Acquire lock for enclave.
        while (!oe_atomic_compare_and_swap_32(&enclave->_lock, 0, 1))
            oe_yield_cpu();
        locked = true;

        cache_size = enclave->_cache_size;
        cache = enclave->_cache;

        if (cache_size <= uid_value)
        {
            uint32_t new_cache_size = (uid_value + 1) * 2;
            if (new_cache_size < 32)
                new_cache_size = 32;

            cache =
                (uint32_t*)realloc(cache, sizeof(uint32_t) * new_cache_size);
            if (cache == NULL)
                goto done;
            memset(
                cache + cache_size,
                (uint32_t)-1,
                sizeof(uint32_t) * (new_cache_size - cache_size));
            cache_size = new_cache_size;
            enclave->_cache = cache;
            enclave->_cache_size = cache_size;
        }

        if (cache[uid_value] == (uint32_t)-1)
        {
            for (uint32_t i = 0; i < enclave->num_ecall_function_names; ++i)
            {
                if (strcmp(enclave->ecall_function_names[i], function_name) ==
                    0)
                {
                    cache[uid_value] = i;
                    break;
                }
            }
        }

        *function_id = cache[uid_value];

        result = OE_OK;
    done:
        if (locked)
            oe_atomic_compare_and_swap_32(&enclave->_lock, 1, 0);

        return result;
    }
}

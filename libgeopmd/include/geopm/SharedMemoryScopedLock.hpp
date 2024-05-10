/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SHAREDMEMORYSCOPEDLOCK_HPP_INCLUDE
#define SHAREDMEMORYSCOPEDLOCK_HPP_INCLUDE

#include <pthread.h>

#include "geopm_public.h"

namespace geopm
{
    /// @brief An object used to automatically hold a SharedMemory mutex while
    ///        in scope, and release it when out of scope.
    class GEOPM_PUBLIC SharedMemoryScopedLock
    {
        public:
            SharedMemoryScopedLock() = delete;
            SharedMemoryScopedLock(pthread_mutex_t *mutex);
            SharedMemoryScopedLock(const SharedMemoryScopedLock &other) = delete;
            SharedMemoryScopedLock &operator=(const SharedMemoryScopedLock &other) = delete;
            virtual ~SharedMemoryScopedLock();
        private:
            pthread_mutex_t *m_mutex;
    };
}

#endif

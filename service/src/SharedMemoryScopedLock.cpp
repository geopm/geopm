/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"
#include "geopm/SharedMemoryScopedLock.hpp"
#include <iostream>
#include "geopm/Exception.hpp"

namespace geopm
{
    SharedMemoryScopedLock::SharedMemoryScopedLock(pthread_mutex_t *mutex)
        : m_mutex(mutex)
    {
        if (m_mutex == nullptr) {
            throw Exception("SharedMemoryScopedLock(): mutex cannot be NULL",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int err = pthread_mutex_lock(m_mutex); // Default mutex will block until this completes.
        if (err) {
            throw Exception("SharedMemoryScopedLock(): pthread_mutex_lock() failed:", err, __FILE__, __LINE__);
        }
    }

    SharedMemoryScopedLock::~SharedMemoryScopedLock()
    {
        int err = pthread_mutex_unlock(m_mutex);
        if (err != 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> pthread_mutex_unlock() failed with error: "
                      << geopm::error_message(err) << std::endl;
#endif
        }
    }
}

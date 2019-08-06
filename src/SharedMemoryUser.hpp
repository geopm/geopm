/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SHAREDMEMORYUSER_HPP_INCLUDE
#define SHAREDMEMORYUSER_HPP_INCLUDE

#include <stdlib.h>
#include <string>
#include <memory>

#include "SharedMemoryScopedLock.hpp"

namespace geopm
{
    /// @brief This class encapsulates attaching to inter-process shared memory.
    class SharedMemoryUser
    {
        public:
            SharedMemoryUser() = default;
            SharedMemoryUser(const SharedMemoryUser &other) = default;
            virtual ~SharedMemoryUser() = default;
            /// @brief Retrieve a pointer to the shared memory region.
            /// @return Void pointer to the shared memory region.
            virtual void *pointer(void) const = 0;
            /// @brief Retrieve the key to the shared memory region.
            /// @return Key to the shared memory region.
            virtual std::string key(void) const = 0;
            /// @brief Retrieve the size of the shared memory region.
            /// @return Size of the shared memory region.
            virtual size_t size(void) const = 0;
            /// @brief Unlink the shared memory region.
            virtual void unlink(void) = 0;
            /// @brief Attempt to lock the mutex for the shared memory region
            ///        and return a scoped mutex object that will unlock the
            ///        mutex when it goes out of scope.
            virtual std::unique_ptr<SharedMemoryScopedLock> get_scoped_lock(void) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<SharedMemoryUser> make_unique(const std::string &shm_key, unsigned int timeout);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<SharedMemoryUser> make_shared(const std::string &shm_key, unsigned int timeout);
    };
}

#endif

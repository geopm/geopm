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

#ifndef SHAREDMEMORY_HPP_INCLUDE
#define SHAREDMEMORY_HPP_INCLUDE

#include <stdlib.h>

#include <string>
#include <memory>

namespace geopm
{
    class SharedMemoryScopedLock;

    /// @brief This class encapsulates the creation of inter-process shared memory.
    class SharedMemory
    {
        public:
            SharedMemory() = default;
            SharedMemory(const SharedMemory &other) = default;
            virtual ~SharedMemory() = default;
            /// @brief Retrieve a pointer to the shared memory region.
            /// @return Void pointer to the shared memory region.
            virtual void *pointer(void) const = 0;
            /// @brief Retrieve the key to the shared memory region.
            /// @return Key to the shared memory region.
            virtual std::string key(void) const = 0;
            /// @brief Retrieve the size of the shared memory region.
            /// @return Size of the shared memory region.
            virtual size_t size(void) const = 0;
            /// @brief Attempt to lock the mutex for the shared memory region
            ///        and return a scoped mutex object that will unlock the
            ///        mutex when it goes out of scope.
            virtual std::shared_ptr<SharedMemoryScopedLock> get_scoped_lock(void) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<SharedMemory> make_unique(const std::string &shm_key, size_t size);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<SharedMemory> make_shared(const std::string &shm_key, size_t size);
    };
}

#endif

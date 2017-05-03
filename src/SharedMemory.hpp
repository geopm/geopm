/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <string>
#include <stdlib.h>

namespace geopm
{
    class SharedMemoryBase
    {
        public:
            SharedMemoryBase() {}
            SharedMemoryBase(const SharedMemoryBase &other) {}
            virtual ~SharedMemoryBase() {}
            virtual void *pointer(void) = 0;
            virtual std::string key(void) = 0;
            virtual size_t size(void) = 0;
    };

    class SharedMemoryUserBase
    {
        public:
            SharedMemoryUserBase() {}
            SharedMemoryUserBase(const SharedMemoryUserBase &other) {}
            virtual ~SharedMemoryUserBase() {}
            virtual void *pointer(void) = 0;
            virtual std::string key(void) = 0;
            virtual size_t size(void) = 0;
            virtual void unlink(void) = 0;
    };

    /// This class encapsulates the creation of inter-process shared memory.
    class SharedMemory : public SharedMemoryBase
    {
        public:
            /// Constructor takes a key and a size and creates a inter-process
            /// shared memory region.
            /// @param [in] shm_key Shared memory key to create the region.
            /// @param [in] size Size of the region to create.
            SharedMemory(const std::string &shm_key, size_t size);
            /// Destructor destroys and unlinks the shared memory region.
            virtual ~SharedMemory();
            /// Retrieve a pointer to the shared memory region.
            /// @return Void pointer to the shared memory region.
            void *pointer(void);
            /// Retrieve the key to the shared memory region.
            /// @return Key to the shared memory region.
            std::string key(void);
            size_t size(void);
        protected:
            /// Shared memory key for the region.
            std::string m_shm_key;
            /// Size of the region.
            size_t m_size;
            /// Pointer to the region.
            void *m_ptr;
    };

    /// This class encapsulates attaching to inter-process shared memory.
    class SharedMemoryUser : public SharedMemoryUserBase
    {
        public:
            /// Constructor takes a key and attempts to attach to a
            /// inter-process shared memory region. This version of the
            /// constructor tries to attach multiple times until a timeout
            /// is reached.
            /// @param [in] shm_key Shared memory key to attach to the region.
            /// @param [in] timeout Length in seconds to keep retrying the
            ///             attachment process to a shared memory region.
            SharedMemoryUser(const std::string &shm_key, unsigned int timeout);
            /// Constructor takes a key and attempts to attach to a
            /// inter-process shared memory region. This version of the
            /// constructor attempts to attach a single time.
            /// @param [in] shm_key Shared memory key to attach to the region.
            SharedMemoryUser(const std::string &shm_key);
            /// Destructor detaches from shared memory region.
            virtual ~SharedMemoryUser();
            /// Retrieve a pointer to the shared memory region.
            /// @return Void pointer to the shared memory region.
            void *pointer(void);
            /// Retrieve the key to the shared memory region.
            /// @return Key to the shared memory region.
            std::string key(void);
            size_t size(void);
            void unlink(void);
        protected:
            /// Shared memory key for the region.
            std::string m_shm_key;
            /// Size of the region.
            size_t m_size;
            /// Pointer to the region.
            void *m_ptr;
            bool m_is_linked;
    };
}

#endif

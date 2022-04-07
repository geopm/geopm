/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef SHAREDMEMORYIMP_HPP_INCLUDE
#define SHAREDMEMORYIMP_HPP_INCLUDE

#include "geopm/SharedMemory.hpp"

#include <pthread.h>

#include <functional>

namespace geopm
{
    class SharedMemoryImp : public SharedMemory
    {
        public:
            SharedMemoryImp();
            /// @brief Destructor destroys and unlinks the shared memory region.
            virtual ~SharedMemoryImp();
            /// @brief Retrieve a pointer to the shared memory region.
            /// @return Void pointer to the shared memory region.
            void *pointer(void) const override;
            /// @brief Retrieve the key to the shared memory region.
            /// @return Key to the shared memory region.
            std::string key(void) const override;
            size_t size(void) const override;
            void unlink(void) override;
            std::unique_ptr<SharedMemoryScopedLock> get_scoped_lock(void) override;
            /// @brief Takes a key and a size and creates
            ///        an inter-process shared memory region.
            /// @param [in] shm_key Shared memory key to create the region.
            /// @param [in] size Size of the region to create.
            /// @param [in] is_secure Disallow group and world r/w if true.
            void create_memory_region(const std::string &shm_key, size_t size, bool is_secure);
            /// @brief Takes a key and attempts to attach to a
            ///        inter-process shared memory region. This version of the
            ///        constructor tries to attach multiple times until a timeout
            ///        is reached.
            /// @param [in] shm_key Shared memory key to attach to the region.
            /// @param [in] timeout Length in seconds to keep retrying the
            ///             attachment process to a shared memory region.
            void attach_memory_region(const std::string &shm_key, unsigned int timeout);
            /// @brief Modifies the shared memory to be owned by the specified gid
            //         and uid if current permissions allow for the change.
            /// @param [in] uid User ID to become owner.
            /// @param [in] gid Group ID to become owner.
            void chown(const unsigned int uid, const unsigned int gid) override;
        private:
            /// @brief Helper struct to track the key functions for the
            ///        particular shared memory object implementation being
            ///        used: shmem or file.
            struct SharedMemFunc
            {
                SharedMemFunc() = default;
                SharedMemFunc(
                    std::function<int(const char *, int, mode_t)> open_fn,
                    std::function<int(const char *)> unlink_fn);

                std::function<int(const char *, int, mode_t)> open;
                std::function<int(const char *)> unlink;
            };

            /// @brief Construct a SharedMemFunc object based on the type of
            ///        shared memory key: shmem key or file path.
            static SharedMemFunc make_shared_mem_func(
                const std::string &key);
            
            /// @brief Shared memory key for the region.
            std::string m_shm_key;
            /// @brief Size of the region.
            size_t m_size;
            /// @brief Pointer to the region.
            void *m_ptr;
            /// @brief Indicates whether the shared memory is ready for use,
            ///        either from calling create() or attach().
            bool m_is_linked;
            /// @brief Whether to throw if unlink fails.  An object created
            ///        through make_unique_owner() may be unlinked in other
            ///        objects' destructors, and should not throw.
            bool m_do_unlink_check;

            SharedMemFunc shm_func;
    };
}

#endif

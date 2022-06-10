/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SHAREDMEMORYIMP_HPP_INCLUDE
#define SHAREDMEMORYIMP_HPP_INCLUDE

#include "geopm/SharedMemory.hpp"

#include <pthread.h>


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
            void chown(const unsigned int uid, const unsigned int gid) const override;

        protected:
            /// @brief Construct the file path to use for the provided key.
            static std::string construct_shm_path(const std::string &key);

        private:
            /// @brief Shared memory key for the region.
            std::string m_shm_key;
            /// @brief file path for shared memory object.
            std::string m_shm_path;
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
    };
}

#endif

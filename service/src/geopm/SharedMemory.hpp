/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SHAREDMEMORY_HPP_INCLUDE
#define SHAREDMEMORY_HPP_INCLUDE

#include <stdlib.h>

#include <string>
#include <memory>

#include "SharedMemoryScopedLock.hpp"

namespace geopm
{
    /// @brief This class encapsulates an inter-process shared memory region.
    class SharedMemory
    {
        public:
            SharedMemory() = default;
            SharedMemory(const SharedMemory &other) = default;
            SharedMemory &operator=(const SharedMemory &other) = default;
            virtual ~SharedMemory() = default;
            /// @brief Retrieve a pointer to the shared memory region.
            ///
            /// @return Void pointer to the shared memory region.
            virtual void *pointer(void) const = 0;
            /// @brief Retrieve the key to the shared memory region.
            ///
            /// @return Key to the shared memory region.
            virtual std::string key(void) const = 0;
            /// @brief Retrieve the size of the shared memory region.
            ///
            /// @return Size of the shared memory region.
            virtual size_t size(void) const = 0;
            /// @brief Unlink the shared memory region.
            virtual void unlink(void) = 0;
            /// @brief Attempt to lock the mutex for the shared memory region
            ///        and return a scoped mutex object that will unlock the
            ///        mutex when it goes out of scope.
            ///
            /// @details This method should be called before accessing the
            ///          memory with pointer()
            virtual std::unique_ptr<SharedMemoryScopedLock> get_scoped_lock(void) = 0;
            /// @brief Creates a shared memory region with the given key and size.
            ///
            /// @return a pointer to a SharedMemory object managing the region.
            static std::unique_ptr<SharedMemory> make_unique_owner(const std::string &shm_key, size_t size);
            /// @brief Creates a shared memory region with the given key and size without group or world permissions.
            ///
            /// @return a pointer to a SharedMemory object managing the region.
            static std::unique_ptr<SharedMemory> make_unique_owner_secure(const std::string &shm_key, size_t size);
            /// @brief Attaches to the shared memory region with the given key.
            ///
            /// @return a pointer to a SharedMemory object managing the region.
            ///
            /// @throws If it cannot attach within the timeout, throws an exception.
            static std::unique_ptr<SharedMemory> make_unique_user(const std::string &shm_key, unsigned int timeout);
            /// @brief Modifies the shared memory to be owned by the specified gid
            //         and uid if current permissions allow for the change.
            virtual void chown(const unsigned int uid, const unsigned int gid) = 0;
    };
}

#endif

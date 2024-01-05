/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IOURINGFALLBACK_HPP_INCLUDE
#define IOURINGFALLBACK_HPP_INCLUDE

#include "IOUring.hpp"

#include <functional>
#include <vector>

namespace geopm
{
    /// @brief Fallback implementation of the IOUring batch interface. This
    /// implementation uses queues of individual read/write operations instead
    /// of a single batched operation.
    class IOUringFallback final : public IOUring
    {
        public:
            IOUringFallback(unsigned entries);
            virtual ~IOUringFallback();

            void submit() override;

            void prep_read(std::shared_ptr<int> ret, int fd,
                           void *buf, unsigned nbytes, off_t offset) override;

            void prep_write(std::shared_ptr<int> ret, int fd,
                            const void *buf, unsigned nbytes, off_t offset) override;

            /// @brief Create a fallback implementation of IOUring that uses non-batched
            ///        IO operations, in case we cannot use IO uring or liburing.
            /// @param entries The expected maximum number of batched operations.
            static std::unique_ptr<IOUring> make_unique(unsigned entries);
        private:
            // Pairs of pointers where operation results are desired, and functions
            // that perform the operation and forward its return value.
            using FutureOperation = std::pair<std::shared_ptr<int>, std::function<int()> >;
            std::vector<FutureOperation> m_operations;
    };
}
#endif // IOURINGFALLBACK_HPP_INCLUDE

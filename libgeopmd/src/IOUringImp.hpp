/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IOURINGIMP_HPP_INCLUDE
#define IOURINGIMP_HPP_INCLUDE

#ifndef _DEFAULT_SOURCE
// Liburing uses some typedefs gated by _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <liburing.h>

#include "IOUring.hpp"

#include <vector>

namespace geopm
{
    /// @brief Implementation of the IOUring batch interface. This
    /// implementation batches operations inside io_uring submission queues.
    class IOUringImp final : public IOUring
    {
        public:
            IOUringImp(unsigned entries);
            virtual ~IOUringImp();
            IOUringImp(const IOUringImp &other) = delete;
            IOUringImp &operator=(const IOUringImp &other) = delete;

            void submit() override;

            void prep_read(std::shared_ptr<int> ret, int fd,
                           void *buf, unsigned nbytes, off_t offset) override;

            void prep_write(std::shared_ptr<int> ret, int fd,
                            const void *buf, unsigned nbytes, off_t offset) override;

            /// @brief Return whether this implementation of IOUring is supported.
            static bool is_supported();

            /// @brief Create an IO uring with queues of a given size.
            /// @param entries  Maximum number of queue operations to contain
            ///                 within a single batch submission.
            static std::unique_ptr<IOUring> make_unique(unsigned entries);
        protected:
            struct io_uring_sqe *get_sqe_or_throw();
            void set_sqe_return_destination(
                struct io_uring_sqe *sqe,
                std::shared_ptr<int> destination);

        private:
            struct io_uring m_ring;
            std::vector<std::shared_ptr<int> > m_result_destinations;
    };
}
#endif // IOURINGIMP_HPP_INCLUDE

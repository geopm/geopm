/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "IOUringImp.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "liburing.h"

#include <memory>

namespace geopm
{
    IOUringImp::IOUringImp(unsigned entries)
        : m_ring()
        , m_result_destinations()
    {
        int ret = io_uring_queue_init(entries, &m_ring, 0);
        if (ret < 0) {
            throw Exception("Failed to initialize a batch queue with IO uring",
                            -ret, __FILE__, __LINE__);
        }
        m_result_destinations.reserve(entries);
    }

    IOUringImp::~IOUringImp()
    {
        io_uring_queue_exit(&m_ring);
    }

    struct io_uring_sqe *IOUringImp::get_sqe_or_throw()
    {
        auto sqe = io_uring_get_sqe(&m_ring);
        if (!sqe) {
            throw Exception("Attempted to add an operation to a full batch queue.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return sqe;
    }

    void IOUringImp::set_sqe_return_destination(
        struct io_uring_sqe *sqe,
        std::shared_ptr<int> destination)
    {
        // IO uring accepts a contextual argument that we can recall when we
        // view the associated completion queue event. Give it the pointer
        // to the location the user requested we write operation return values.
        // Ensure that point remains valid by keeping the shared_ptr in our
        // result destinations vector until we finish cleaning up the batch.
        io_uring_sqe_set_data(sqe, destination.get());
        m_result_destinations.push_back(std::move(destination));
    }

    void IOUringImp::submit()
    {
        int submitted_operations = 0;
        while (io_uring_sq_ready(&m_ring) > 0) {
            submitted_operations += io_uring_submit(&m_ring);
        }
        struct io_uring_cqe *cqe;

        for (int operation = 0; operation < submitted_operations; ++operation) {
            int ret = io_uring_wait_cqe(&m_ring, &cqe);
            if (ret < 0) {
                throw Exception("Failed to get a completion event from IO uring",
                                -ret, __FILE__, __LINE__);
            }

            int *result_destination = static_cast<int *>(io_uring_cqe_get_data(cqe));
            if (result_destination) {
                // The caller of prep_...() for this operation wants to
                // know the return value of the operation, so write it back.
                *result_destination = cqe->res;
            }
            io_uring_cqe_seen(&m_ring, cqe);
        }

        // We're done writing batch operation results, so we don't need to
        // track the destination pointers any more.
        m_result_destinations.clear();
    }

    void IOUringImp::prep_read(std::shared_ptr<int> ret, int fd, void *buf,
                               unsigned nbytes, off_t offset)
    {
        auto sqe = get_sqe_or_throw();
        // Available since Linux 5.10
        io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        set_sqe_return_destination(sqe, std::move(ret));
    }

    void IOUringImp::prep_write(std::shared_ptr<int> ret, int fd, const void *buf,
                                unsigned nbytes, off_t offset)
    {
        auto sqe = get_sqe_or_throw();
        // Available since Linux 5.10
        io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        set_sqe_return_destination(sqe, std::move(ret));
    }

    bool IOUringImp::is_supported()
    {
        std::unique_ptr<io_uring_probe, decltype(&free)> probe(io_uring_get_probe(), free);
        if (!probe) {
            return false;
        }
        if (!io_uring_opcode_supported(probe.get(), IORING_OP_READ)) {
            return false;
        }
        if (!io_uring_opcode_supported(probe.get(), IORING_OP_WRITE)) {
            return false;
        }
        return true;
    }

    std::unique_ptr<IOUring> IOUringImp::make_unique(unsigned entries)
    {
        return geopm::make_unique<IOUringImp>(entries);
    }
}

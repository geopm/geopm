/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPICOMM_HPP_INCLUDE
#define MPICOMM_HPP_INCLUDE

#include <memory>
#include <set>
#include <vector>
#include <string>
#include "Comm.hpp"
#ifndef GEOPM_TEST
#include <mpi.h>
#endif

namespace geopm
{
    /// @brief Implementation of the Comm interface using MPI as the
    ///        underlying communication mechanism.
    class MPIComm : public Comm
    {
        public:
            MPIComm();
            MPIComm(MPI_Comm in_comm);
            MPIComm(const MPIComm *in_comm);
            MPIComm(const MPIComm *in_comm, std::vector<int> dimension, std::vector<int> periods, bool is_reorder);
            MPIComm(const MPIComm *in_comm, int color, int key);
            MPIComm(const MPIComm *in_comm, const std::string &tag,  bool &is_ctl);
            MPIComm(const MPIComm *in_comm, const std::string &tag);
            MPIComm(const MPIComm *in_comm, const std::string &tag, int split_type);
            virtual ~MPIComm();

            static std::string plugin_name(void);
            static std::unique_ptr<Comm> make_plugin(void);
            static MPIComm &comm_world(void);

            virtual std::shared_ptr<Comm> split() const override;
            virtual std::shared_ptr<Comm> split(int color, int key) const override;
            virtual std::shared_ptr<Comm> split(const std::string &tag, int split_type) const override;
            virtual std::shared_ptr<Comm> split(std::vector<int> dimensions, std::vector<int> periods, bool is_reorder) const override;
            virtual std::shared_ptr<Comm> split_cart(std::vector<int> dimensions) const override;

            virtual bool comm_supported(const std::string &description) const override;

            virtual int cart_rank(const std::vector<int> &coords) const override;
            virtual int rank(void) const override;
            virtual int num_rank(void) const override;
            virtual void dimension_create(int num_ranks, std::vector<int> &dimension) const override;
            virtual void alloc_mem(size_t size, void **base) override;
            virtual void free_mem(void *base) override;
            virtual size_t window_create(size_t size, void *base) override;
            virtual void window_destroy(size_t window_id) override;
            virtual void coordinate(int rank, std::vector<int> &coord) const override;
            virtual std::vector<int> coordinate(int rank) const override;
            virtual void window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const override;
            virtual void window_unlock(size_t window_id, int rank) const override;
            virtual void barrier(void) const override;
            virtual void broadcast(void *buffer, size_t size, int root) const override;
            virtual bool test(bool is_true) const override;
            virtual void reduce_max(double *send_buf, double *recv_buf, size_t count, int root) const override;
            virtual void gather(const void *send_buf, size_t send_size, void *recv_buf,
                                size_t recv_size, int root) const override;
            virtual void gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                                 const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const override;
            virtual void window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const override;

            void tear_down(void) override;
        protected:
            void check_window(size_t window_id) const;
            bool is_valid() const;
            MPI_Comm m_comm;
            size_t m_maxdims;
            std::set<size_t> m_windows;
            const std::string m_name;
            bool m_is_torn_down = false;
    };
}

#endif

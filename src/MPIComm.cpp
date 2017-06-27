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

#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <map>

#include "Comm.hpp"
#include "MPIComm.hpp"
#include "Exception.hpp"
#include "SharedMemory.hpp"
#include "geopm_env.h"
#include "geopm_comm.h"
#include "config.h"

extern "C"
{
static int geopm_comm_split_imp(MPI_Comm comm, const char *tag, int *num_node, MPI_Comm *split_comm, int *is_ctl_comm);

int geopm_comm_split_ppn1(MPI_Comm comm, const char *tag, MPI_Comm *ppn1_comm)
{
    int num_node = 0;
    int is_shm_root = 0;
    int err = geopm_comm_split_imp(comm, tag, &num_node, ppn1_comm, &is_shm_root);
    if (!err && !is_shm_root) {
        err = MPI_Comm_free(ppn1_comm);
        *ppn1_comm = MPI_COMM_NULL;
    }
    return err;
}

int geopm_comm_split_shared(MPI_Comm comm, const char *tag, MPI_Comm *split_comm)
{
    int err = 0;
    struct stat stat_struct;
    try {
        std::ostringstream shmem_key;
        shmem_key << geopm_env_shmkey() << "-comm-split-" << tag;
        std::ostringstream shmem_path;
        shmem_path << "/dev/shm" << shmem_key.str();
        geopm::SharedMemory *shmem = NULL;
        geopm::SharedMemoryUser *shmem_user = NULL;
        int rank, color = -1;

        MPI_Comm_rank(comm, &rank);
        // remove shared memory file if one already exists
        (void)unlink(shmem_path.str().c_str());
        MPI_Barrier(comm);
        err = stat(shmem_path.str().c_str(), &stat_struct);
        if (!err || (err && errno != ENOENT)) {
            std::stringstream ex_str;
            ex_str << "geopm_comm_split_shared(): " << shmem_key.str()
                << " already exists and cannot be deleted.";
            throw geopm::Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        MPI_Barrier(comm);
        try {
            shmem = new geopm::SharedMemory(shmem_key.str(), sizeof(int));
        }
        catch (geopm::Exception ex) {
            if (ex.err_value() != EEXIST) {
                throw ex;
            }
        }
        if (!shmem) {
            shmem_user = new geopm::SharedMemoryUser(shmem_key.str(), 1);
        }
        else {
            color = rank;
            *((int*)(shmem->pointer())) = color;
        }
        MPI_Barrier(comm);
        if (shmem_user) {
            color = *((int*)(shmem_user->pointer()));
        }
        err = MPI_Comm_split(comm, color, rank, split_comm);
        delete shmem;
        delete shmem_user;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}

int geopm_comm_split(MPI_Comm comm, const char *tag, MPI_Comm *split_comm, int *is_ctl_comm)
{
    int num_node = 0;
    return geopm_comm_split_imp(comm, tag, &num_node, split_comm, is_ctl_comm);
}

static int geopm_comm_split_imp(MPI_Comm comm, const char *tag, int *num_node, MPI_Comm *split_comm, int *is_shm_root)
{
    int err, comm_size, comm_rank, shm_rank;
    MPI_Comm shm_comm = MPI_COMM_NULL, tmp_comm = MPI_COMM_NULL;
    MPI_Comm *split_comm_ptr;

    *is_shm_root = 0;

    if (split_comm) {
        split_comm_ptr = split_comm;
    }
    else {
        split_comm_ptr = &tmp_comm;
    }

    err = MPI_Comm_size(comm, &comm_size);
    if (!err) {
        err = MPI_Comm_rank(comm, &comm_rank);
    }
    if (!err) {
        err = geopm_comm_split_shared(comm, tag, &shm_comm);
    }
    if (!err) {
        err = MPI_Comm_rank(shm_comm, &shm_rank);
    }
    if (!err) {
        if (!shm_rank) {
            *is_shm_root = 1;
        }
        else {
            *is_shm_root = 0;
        }
        err = MPI_Comm_split(comm, *is_shm_root, comm_rank, split_comm_ptr);
    }
    if (!err) {
        if (*is_shm_root == 1) {
            err = MPI_Comm_size(*split_comm_ptr, num_node);
        }
    }
    if (!err) {
        err = MPI_Bcast(num_node, 1, MPI_INT, 0, shm_comm);
    }
    if (shm_comm != MPI_COMM_NULL) {
        MPI_Comm_free(&shm_comm);
    }
    if (!split_comm) {
        MPI_Comm_free(split_comm_ptr);
    }
    return err;
}
}
namespace geopm
{
    const char *MPICOMM_DESCRIPTION = "MPIComm";
    class CommWindow
    {
        public:
            CommWindow(MPI_Comm comm, void *base, size_t size);
            virtual ~CommWindow();
            void lock(bool is_exclusive, int rank, int assert);
            void unlock(int rank);
            void put(const void *send_buf, size_t send_size, int rank, off_t disp);
#ifndef GEOPM_TEST
        protected:
#endif
            MPI_Win m_window;
    };
    ///////////////////////////////
    // Helper for MPI exceptions //
    ///////////////////////////////
    static void check_mpi(int err)
    {
        if (err) {
            char error_str[MPI_MAX_ERROR_STRING];
            int name_max = MPI_MAX_ERROR_STRING;
            MPI_Error_string(err, error_str, &name_max);
            std::ostringstream ex_str;
            ex_str << "MPI Error: " << error_str;
            throw Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    MPIComm::MPIComm()
        : m_comm(MPI_COMM_WORLD)
        , m_maxdims(1)
        , m_description(MPICOMM_DESCRIPTION)
    {
    }

    MPIComm::MPIComm(const MPIComm *in_comm)
        : m_comm(MPI_COMM_NULL)
        , m_maxdims(1)
        , m_description(in_comm->m_description)
    {
        if (in_comm->is_valid()) {
            check_mpi(PMPI_Comm_dup(in_comm->m_comm, &m_comm));
        }
    }

    MPIComm::MPIComm(MPI_Comm comm)
        : m_comm(MPI_COMM_NULL)
        , m_maxdims(1)
        , m_description(MPICOMM_DESCRIPTION)
    {
        if (comm != MPI_COMM_NULL) {
            check_mpi(PMPI_Comm_dup(comm, &m_comm));
        }
    }

    MPIComm::MPIComm(const MPIComm *in_comm, std::vector<int> dimension, std::vector<int> periods, bool is_reorder)
        : m_comm(MPI_COMM_NULL)
        , m_maxdims(dimension.size())
        , m_description(in_comm->m_description)
    {
        if (in_comm->is_valid()) {
            check_mpi(PMPI_Cart_create(in_comm->m_comm, m_maxdims, dimension.data(), periods.data(), (int) is_reorder, &m_comm));
        }
    }

    MPIComm::MPIComm(const MPIComm *in_comm, int color, int key)
        : m_comm(MPI_COMM_NULL)
        , m_maxdims(1)
        , m_description(in_comm->m_description)
    {
        static std::map<int, int> color_map = {{M_SPLIT_COLOR_UNDEFINED, MPI_UNDEFINED}};
        auto it = color_map.find(color);
        if (it != color_map.end()) {
            color = it->second;
        }
        if (in_comm->is_valid()) {
            check_mpi(PMPI_Comm_split(in_comm->m_comm, color, key, &m_comm));
        }
    }

    MPIComm::MPIComm(const MPIComm *in_comm, std::string tag, int split_type)
        : m_comm(MPI_COMM_NULL)
        , m_maxdims(1)
        , m_description(in_comm->m_description)
    {
        int err = 0;
        if (!in_comm->is_valid()) {
            throw Exception("in_comm is invalid", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        switch (split_type) {
            case M_COMM_SPLIT_TYPE_CTL:
                throw Exception("got split type ctl, remove this if exception not seen", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                break;
            case M_COMM_SPLIT_TYPE_PPN1:
                err = geopm_comm_split_ppn1(in_comm->m_comm, tag.c_str(), &m_comm);
                break;
            case M_COMM_SPLIT_TYPE_SHARED:
                err = geopm_comm_split_shared(in_comm->m_comm, tag.c_str(), &m_comm);
                break;
            default:
                std::ostringstream ex_str;
                ex_str << "Invalid split_type.";
                throw Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (err) {
            throw geopm::Exception("geopm_comm_split_ppn1()", err, __FILE__, __LINE__);
        }

    }

    MPIComm::MPIComm(const MPIComm *in_comm, std::string tag,  bool &is_ctl)
        : m_comm(MPI_COMM_NULL)
        , m_maxdims(1)
        , m_description(in_comm->m_description)
    {
        geopm_comm_split_ppn1(in_comm->m_comm, tag.c_str(), &m_comm);
        if (!is_valid()) {
            is_ctl = false;
        } else {
            is_ctl = true;
        }

    }

    MPIComm::~MPIComm()
    {
        for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
            delete (CommWindow *) *it;
        }
        if (m_comm != MPI_COMM_WORLD) {
            MPI_Comm_free(&m_comm);
        }
    }

    bool MPIComm::comm_supported(const std::string &description) const
    {
        return description == m_description;
    }

    int MPIComm::cart_rank(const std::vector<int> &coords) const
    {
        int rank = 0;
        check_mpi(PMPI_Cart_rank(m_comm, coords.data(), &rank));
        return rank;
    }

    int MPIComm::rank(void) const
    {
        int tmp_rank = 0;
        check_mpi(PMPI_Comm_rank(m_comm, &tmp_rank));
        return tmp_rank;
    }

    int MPIComm::num_rank(void) const
    {
        int tmp_size = 0;
        if (m_comm != MPI_COMM_NULL) {
            check_mpi(PMPI_Comm_size(m_comm, &tmp_size));
        }
        return tmp_size;
    }

    void MPIComm::dimension_create(int num_nodes, std::vector<int> &dimension) const
    {
        check_mpi(PMPI_Dims_create(num_nodes, dimension.size(), dimension.data()));
    }

    void MPIComm::check_window(size_t win_handle) const
    {
        if (m_windows.find(win_handle) == m_windows.end()) {
            std::ostringstream ex_str;
            ex_str << "requested window handle " << win_handle << " invalid";
            throw Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    bool MPIComm::is_valid() const
    {
        return m_comm != MPI_COMM_NULL;
    }

    void MPIComm::alloc_mem(size_t size, void **base)
    {
        check_mpi(PMPI_Alloc_mem(size, MPI_INFO_NULL, base));
    }

    void MPIComm::free_mem(void *base)
    {
        check_mpi(PMPI_Free_mem(base));
    }

    size_t MPIComm::window_create(size_t size, void *base)
    {
        CommWindow *win_handle = new CommWindow(m_comm, base, size);
        m_windows.insert((size_t) win_handle);
        return (size_t ) win_handle;
    }

    void MPIComm::window_destroy(size_t win_handle)
    {
        check_window(win_handle);
        m_windows.erase(win_handle);
        delete (CommWindow *) win_handle;
    }

    void MPIComm::window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const
    {
        check_window(window_id);
        ((CommWindow *) window_id)->lock(is_exclusive, rank, assert);
    }

    void MPIComm::window_unlock(size_t window_id, int rank) const
    {
        check_window(window_id);
        ((CommWindow *) window_id)->unlock(rank);
    }

    void MPIComm::coordinate(int rank, std::vector<int> &coord) const
    {
        size_t in_size = coord.size();
        if (m_maxdims != in_size) {
            std::stringstream ex_str;
            ex_str << "input coord size (" << in_size << ") != m_maxdims (" << m_maxdims << ")";
            throw geopm::Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (is_valid()) {
            check_mpi(PMPI_Cart_coords(m_comm, rank, m_maxdims, coord.data()));
        }
    }

    void MPIComm::barrier(void) const
    {
        if (is_valid()) {
            check_mpi(PMPI_Barrier(m_comm));
        }
    }

    void MPIComm::broadcast(void *buffer, size_t size, int root) const
    {
        if (is_valid()) {
            check_mpi(PMPI_Bcast(buffer, size, MPI_BYTE, root, m_comm));
        }
    }


    void MPIComm::reduce_max(double *sendbuf, double *recvbuf, size_t count, int root) const
    {
        if (is_valid()) {
            check_mpi(PMPI_Reduce(sendbuf, recvbuf, count, MPI_DOUBLE, MPI_MAX, root, m_comm));
        }
    }

    bool MPIComm::test(bool is_true) const
    {
        int is_all_true = 0;
        int tmp_is_true = (int) is_true;
        if (is_valid()) {
            check_mpi(PMPI_Allreduce(&tmp_is_true, &is_all_true, 1, MPI_INT, MPI_LAND, m_comm));
        }
        return (bool) is_all_true;
    }

    void MPIComm::gather(const void *send_buf, size_t send_size, void *recv_buf,
            size_t recv_size, int root) const
    {
        if (is_valid()) {
            check_mpi(PMPI_Gather(GEOPM_MPI_CONST_CAST(void *)(send_buf), send_size, MPI_BYTE, recv_buf, recv_size, MPI_BYTE, root, m_comm));
        }
    }

    void MPIComm::gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                    const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const
    {
        std::vector<int> sizes(recv_sizes.size(), 0);
        std::vector<int> offsets(rank_offset.size(), 0);
        auto in_size_it = recv_sizes.begin();
        auto out_size_it = sizes.begin();
        auto in_off_it = rank_offset.begin();
        auto out_off_it = offsets.begin();

        for (;in_size_it != recv_sizes.end();
             ++in_size_it, ++out_size_it,
             ++in_off_it, ++out_off_it) {
            // TODO overflow check
            *out_size_it = *in_size_it;
            *out_off_it = *in_off_it;
        }
        if (is_valid()) {
            check_mpi(PMPI_Gatherv(GEOPM_MPI_CONST_CAST(void *)(send_buf), send_size, MPI_BYTE, recv_buf, sizes.data(),
                                   offsets.data(), MPI_BYTE, root, m_comm));
        }
    }

    void MPIComm::window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const
    {
        check_window(window_id);
        ((CommWindow *) window_id)->put(send_buf, send_size, rank, disp);
    }

    CommWindow::CommWindow(MPI_Comm comm, void *base, size_t size)
    {
        check_mpi(PMPI_Win_create(base, (MPI_Aint) size, 1, MPI_INFO_NULL, comm, &m_window));
    }

    CommWindow::~CommWindow()
    {
        check_mpi(PMPI_Win_free(&m_window));
    }

    void CommWindow::lock(bool is_exclusive, int rank, int assert)
    {
        check_mpi(PMPI_Win_lock(is_exclusive ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED, rank, assert, m_window));
    }

    void CommWindow::unlock(int rank)
    {
        check_mpi(PMPI_Win_unlock(rank, m_window));
    }

    void CommWindow::put(const void *send_buf, size_t send_size, int rank, off_t disp)
    {
        check_mpi(PMPI_Put(GEOPM_MPI_CONST_CAST(void *)(send_buf), send_size, MPI_BYTE, rank, disp,
                    send_size, MPI_BYTE, m_window));
    }
}

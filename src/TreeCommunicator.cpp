/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <algorithm>
#include <fstream>
#include <system_error>

#include "Exception.hpp"
#include "TreeCommunicator.hpp"
#include "GlobalPolicy.hpp"
#include "SharedMemory.hpp"
#include "geopm_message.h"
#include "geopm_env.h"
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
            std::string shmem_key(geopm_env_shmkey());
            shmem_key += "-comm-split-" + std::string(tag);
            std::string shmem_path("/dev/shm" + shmem_key);
            geopm::SharedMemory *shmem = NULL;
            geopm::SharedMemoryUser *shmem_user = NULL;
            int rank, color = -1;

            MPI_Comm_rank(comm, &rank);
            // remove shared memory file if one already exists
            (void)unlink(shmem_path.c_str());
            MPI_Barrier(comm);
            err = stat(shmem_path.c_str(), &stat_struct);
            if (!err || (err && errno != ENOENT)) {
                throw geopm::Exception("geopm_comm_split_shared(): " + std::string(shmem_key) + " already exists and cannot be deleted.", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            MPI_Barrier(comm);
            try {
                shmem = new geopm::SharedMemory(shmem_key, sizeof(int));
            }
            catch (geopm::Exception ex) {
                if (ex.err_value() != EEXIST) {
                    throw ex;
                }
            }
            if (!shmem) {
                shmem_user = new geopm::SharedMemoryUser(shmem_key, 1);
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
    //////////////////////
    // Static constants //
    //////////////////////

    enum mpi_tag_e {
        GEOPM_SAMPLE_TAG,
        GEOPM_POLICY_TAG,
    };

    ///////////////////////////////
    // Helper for MPI exceptions //
    ///////////////////////////////
    void check_mpi(int err)
    {
        if (err) {
            char error_str[MPI_MAX_ERROR_STRING];
            int name_max = MPI_MAX_ERROR_STRING;
            MPI_Error_string(err, error_str, &name_max);
            throw Exception("MPI Error: " + std::string(error_str), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    //////////////////////////////////
    // Static function declarations //
    //////////////////////////////////

    static MPI_Datatype create_sample_mpi_type(void);
    static MPI_Datatype create_policy_mpi_type(void);

    /////////////////////////////////
    // Internal class declarations //
    /////////////////////////////////

    /// @brief TreeCommunicatorLevel class encapsulates communication functionality on
    /// a per-level basis.
    class TreeCommunicatorLevel
    {
        public:
            TreeCommunicatorLevel(MPI_Comm comm, MPI_Datatype sample_mpi_type, MPI_Datatype policy_mpi_type);
            ~TreeCommunicatorLevel();
            /// Check sample mailbox for each child and if all are full copy
            /// them into sample and post a new MPI_Irecv(), otherwise throw
            /// geopm::Exception with err_value() of
            /// GEOPM_ERROR_SAMPLE_INCOMPLETE
            void get_sample(std::vector<struct geopm_sample_message_s> &sample);
            /// Check policy mailbox and if full copy to m_policy and post a
            /// new MPI_Irecv(). If mailbox is empty set policy to last known
            /// policy.  If mailbox is empty and no policy has been set throw
            /// a geopm::Exception with err_value() of
            /// GEOPM_ERROR_POLICY_UNKNOWN.
            void get_policy(struct geopm_policy_message_s &policy);
            /// Send sample via MPI_Isend() to root of level, skip if no
            /// recieve has been posted.
            void send_sample(const struct geopm_sample_message_s &sample);
            /// Send policy via MPI_Isend() to all children, skip if no
            /// recieve has been posted.
            void send_policy(const std::vector<struct geopm_policy_message_s> &policy, size_t length);
            /// Returns the level rank of the calling process.
            int level_rank(void);
            size_t overhead_send(void);
        protected:
            void open_recv(void);
            void close_recv(void);
            MPI_Comm m_comm;
            MPI_Datatype m_sample_mpi_type; // MPI data type for sample message
            MPI_Datatype m_policy_mpi_type; // MPI data type for policy message
            int m_size;
            int m_rank;
            std::vector <struct geopm_sample_message_s> m_sample_mailbox;
            MPI_Win m_sample_window;
            struct geopm_policy_message_s m_policy_mailbox;
            MPI_Request m_policy_request;
            struct geopm_policy_message_s m_policy;
            size_t m_overhead_send;
    };

    /////////////////////////////////
    // Static function definitions //
    /////////////////////////////////

    static MPI_Datatype create_sample_mpi_type(void)
    {
        int blocklength[4] = {1, 1, 1, 1};
        MPI_Datatype mpi_type[5] = {MPI_UNSIGNED_LONG_LONG,
                                    MPI_DOUBLE,
                                    MPI_DOUBLE,
                                    MPI_DOUBLE
                                   };
        MPI_Aint offset[4];
        MPI_Datatype result;
        offset[0] = offsetof(struct geopm_sample_message_s, region_id);
        offset[1] = offsetof(struct geopm_sample_message_s, signal);
        offset[2] = offset[1] + sizeof(double);
        offset[3] = offset[2] + sizeof(double);
        check_mpi(MPI_Type_create_struct(4, blocklength, offset, mpi_type, &result));
        check_mpi(MPI_Type_commit(&result));
        return result;
    }


    static MPI_Datatype create_policy_mpi_type(void)
    {
        int blocklength[4] = {1, 1, 1, 1};
        MPI_Datatype mpi_type[4] = {MPI_INT,
                                    MPI_UNSIGNED_LONG,
                                    MPI_INT,
                                    MPI_DOUBLE
                                   };
        MPI_Aint offset[4];
        MPI_Datatype result;

        offset[0] = offsetof(struct geopm_policy_message_s, mode);
        offset[1] = offsetof(struct geopm_policy_message_s, flags);
        offset[2] = offsetof(struct geopm_policy_message_s, num_sample);
        offset[3] = offsetof(struct geopm_policy_message_s, power_budget);
        check_mpi(MPI_Type_create_struct(4, blocklength, offset, mpi_type, &result));
        check_mpi(MPI_Type_commit(&result));
        return result;
    }

    ///////////////////////////////////
    // TreeCommunicator public API's //
    ///////////////////////////////////

    TreeCommunicator::TreeCommunicator(const std::vector<int> &fan_out, GlobalPolicy *global_policy, const MPI_Comm &comm)
        : m_num_node(0)
        , m_fan_out(fan_out)
        , m_comm(fan_out.size())
        , m_global_policy(global_policy)
        , m_level(fan_out.size())
    {
        mpi_type_create();
        comm_create(comm);
        level_create();
        check_mpi(MPI_Comm_size(comm, &m_num_node));
    }

    TreeCommunicator::~TreeCommunicator()
    {
        level_destroy();
        comm_destroy();
        mpi_type_destroy();
    }

    int TreeCommunicator::num_level(void) const
    {
        return m_num_level;
    }

    int TreeCommunicator::root_level(void) const
    {
        return m_fan_out.size();
    }

    int TreeCommunicator::level_rank(int level) const
    {
        return m_level[level]->level_rank();
    }

    int TreeCommunicator::level_size(int level) const
    {
        int result = 1;
        if (level < (int)m_fan_out.size()) {
            result = *(m_fan_out.end() - level - 1);
        }
        return result;
    }

    void TreeCommunicator::send_sample(int level, const struct geopm_sample_message_s &sample)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw Exception("TreeCommunicator::send_sample()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level[level]->send_sample(sample);
    }

    void TreeCommunicator::send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw Exception("TreeCommunicator::send_policy()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        else {
            m_level[level]->send_policy(policy, level_size(level));
        }
    }

    void TreeCommunicator::get_sample(int level, std::vector<struct geopm_sample_message_s> &sample)
    {
        if (level <= 0 || level >= num_level()) {
            throw Exception("TreeCommunicator::get_sample()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level[level - 1]->get_sample(sample);
    }

    void TreeCommunicator::get_policy(int level, struct geopm_policy_message_s &policy)
    {
        if (level < 0 || level >= num_level()) {
            throw Exception("TreeCommunicator::get_policy()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        if (level == root_level()) {
            m_global_policy->policy_message(policy);
            if (policy.power_budget > 0) {
                policy.power_budget *= m_num_node;
            }
        }
        else {
            m_level[level]->get_policy(policy);
        }
    }

    size_t TreeCommunicator::overhead_send(void)
    {
        size_t result = 0;
        for (auto it = m_level.begin(); it != m_level.end(); ++it) {
            result += (*it)->overhead_send();
        }
        return result;
    }


    //////////////////////////////////////
    // TreeCommunicator protected API's //
    //////////////////////////////////////

    void TreeCommunicator::mpi_type_create(void)
    {
        m_sample_mpi_type = create_sample_mpi_type();
        m_policy_mpi_type = create_policy_mpi_type();
    }

    void TreeCommunicator::comm_create(const MPI_Comm &comm)
    {
        int num_dim = m_fan_out.size();
        int color, key;
        MPI_Comm comm_cart;
        std::vector<int> flags(num_dim);
        std::vector<int> coords(num_dim);
        int rank_cart;

        memset(flags.data(), 0, sizeof(int)*num_dim);
        flags[0] = 1;
        check_mpi(MPI_Cart_create(comm, num_dim, m_fan_out.data(), flags.data(), 1, &comm_cart));
        check_mpi(MPI_Comm_rank(comm_cart, &rank_cart));
        check_mpi(MPI_Cart_coords(comm_cart, rank_cart, num_dim, coords.data()));
        check_mpi(MPI_Cart_sub(comm_cart, flags.data(), &(m_comm[0])));
        for (int i = 1; i < num_dim; ++i) {
            if (coords[i-1] == 0) {
                color = 1;
                key = coords[i];
            }
            else {
                color = MPI_UNDEFINED;
                key = 0;
            }
            check_mpi(MPI_Comm_split(comm_cart, color, key, &(m_comm[i])));
        }
        check_mpi(MPI_Comm_free(&comm_cart));

        m_num_level = 0;
        for (auto comm_it = m_comm.begin();
             comm_it != m_comm.end() && *comm_it != MPI_COMM_NULL;
             ++comm_it) {
            m_num_level++;
        }

        m_comm.resize(m_num_level);

        if (m_global_policy) {
            m_num_level++;
        }

        if (rank_cart == 0 && m_global_policy == NULL) {
            throw Exception("process at root of tree communicator has not mapped the control file", GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }
        if (rank_cart != 0 && m_global_policy != NULL) {
            throw Exception("process not at root of tree communicator has mapped the control file", GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }
    }

    void TreeCommunicator::level_create(void)
    {
        if (num_level() == root_level() + 1) {
            m_level.resize(root_level());
        }
        else {
            m_level.resize(num_level());
        }

        auto comm_it = m_comm.begin();
        for (auto level_it = m_level.begin();
             level_it != m_level.end();
             ++level_it, ++comm_it) {
            *level_it = new TreeCommunicatorLevel(*comm_it, m_sample_mpi_type, m_policy_mpi_type);
        }
    }

    void TreeCommunicator::level_destroy(void)
    {
        for (auto level_it = m_level.rbegin();
             level_it != m_level.rend();
             ++level_it) {
            delete *level_it;
        }
    }

    void TreeCommunicator::comm_destroy(void)
    {
        for (auto comm_it = m_comm.begin(); comm_it != m_comm.end(); ++comm_it) {
            MPI_Comm_free(&(*comm_it));
        }
    }

    void TreeCommunicator::mpi_type_destroy(void)
    {
        MPI_Type_free(&m_policy_mpi_type);
        MPI_Type_free(&m_sample_mpi_type);
    }

    /////////////////////////////////
    // TreeCommunicatorLevel API's //
    /////////////////////////////////

    TreeCommunicatorLevel::TreeCommunicatorLevel(MPI_Comm comm, MPI_Datatype sample_mpi_type, MPI_Datatype policy_mpi_type)
        : m_comm(comm)
        , m_sample_mpi_type(sample_mpi_type)
        , m_policy_mpi_type(policy_mpi_type)
        , m_policy_request(MPI_REQUEST_NULL)
        , m_overhead_send(0)
    {
        check_mpi(MPI_Comm_size(comm, &m_size));
        check_mpi(MPI_Comm_rank(comm, &m_rank));
        m_sample_mailbox.resize(m_size);
        std::fill(m_sample_mailbox.begin(), m_sample_mailbox.end(), GEOPM_SAMPLE_INVALID);
        m_policy = GEOPM_POLICY_UNKNOWN;
        open_recv();
    }

    TreeCommunicatorLevel::~TreeCommunicatorLevel()
    {
        close_recv();
    }

    void TreeCommunicatorLevel::get_sample(std::vector<struct geopm_sample_message_s> &sample)
    {
        if (sample.size() < m_sample_mailbox.size()) {
            throw Exception(std::string(__func__) + ": Input sample vector too small", GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }

        bool is_complete = true;
        for (auto mailbox_it = m_sample_mailbox.begin(); mailbox_it != m_sample_mailbox.end(); ++mailbox_it) {
            if ((*mailbox_it).region_id == 0) {
                is_complete = false;
            }
        }

        if (!is_complete) {
            throw Exception(__func__, GEOPM_ERROR_SAMPLE_INCOMPLETE, __FILE__, __LINE__);
        }

        check_mpi(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, m_sample_window));
        copy(m_sample_mailbox.begin(), m_sample_mailbox.end(), sample.begin());
        std::fill(m_sample_mailbox.begin(), m_sample_mailbox.end(), GEOPM_SAMPLE_INVALID);
        check_mpi(MPI_Win_unlock(0, m_sample_window));
    }

    void TreeCommunicatorLevel::get_policy(struct geopm_policy_message_s &policy)
    {
        int is_complete;
        MPI_Status status;

        check_mpi(MPI_Test(&m_policy_request, &is_complete, &status));
        if (is_complete) {
            m_policy = m_policy_mailbox;
            check_mpi(MPI_Irecv(&m_policy_mailbox, 1, m_policy_mpi_type, 0, GEOPM_POLICY_TAG, m_comm, &m_policy_request));
        }
        policy = m_policy;
        if (geopm_is_policy_equal(&policy, &GEOPM_POLICY_UNKNOWN)) {
            throw Exception("TreeCommunicatorLevel::get_policy", GEOPM_ERROR_POLICY_UNKNOWN, __FILE__, __LINE__);
        }
    }

    void TreeCommunicatorLevel::send_sample(const struct geopm_sample_message_s &sample)
    {
        check_mpi(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, m_sample_window));
#ifdef GEOPM_ENABLE_MPI3
        check_mpi(MPI_Put(&sample, 1, m_sample_mpi_type, 0, m_rank, 1, m_sample_mpi_type, m_sample_window));
#else
        check_mpi(MPI_Put((void *)&sample, 1, m_sample_mpi_type, 0, m_rank, 1, m_sample_mpi_type, m_sample_window));
#endif
        check_mpi(MPI_Win_unlock(0, m_sample_window));
        m_overhead_send += sizeof(struct geopm_sample_message_s);
    }

    void TreeCommunicatorLevel::send_policy(const std::vector<struct geopm_policy_message_s> &policy, size_t length)
    {
        size_t dest;
        MPI_Request request;

        if (m_rank != 0) {
            throw Exception("called send_policy() from rank not at root of level", GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }
        dest = 0;
        for (auto policy_it = policy.begin(); dest != length; ++policy_it, ++dest) {
            // Don't check return code or hold onto request, drop message if receiver not ready
            (void) MPI_Isend(const_cast<struct geopm_policy_message_s*>(&(*policy_it)), 1, m_policy_mpi_type, dest, GEOPM_POLICY_TAG, m_comm, &request);
            (void) MPI_Request_free(&request);
        }
        m_overhead_send += policy.size() * sizeof(struct geopm_policy_message_s);
    }

    size_t TreeCommunicatorLevel::overhead_send(void)
    {
        return m_overhead_send;
    }

    int TreeCommunicatorLevel::level_rank(void)
    {
        return m_rank;
    }

    void TreeCommunicatorLevel::open_recv(void)
    {
        check_mpi(MPI_Irecv(&m_policy_mailbox, 1, m_policy_mpi_type, 0, GEOPM_POLICY_TAG, m_comm, &m_policy_request));
        if (!m_rank) {
            check_mpi(MPI_Win_create(m_sample_mailbox.data(), m_sample_mailbox.size(), sizeof(struct geopm_sample_message_s), MPI_INFO_NULL, m_comm, &m_sample_window));
        }
        else {
            check_mpi(MPI_Win_create(NULL, 0, sizeof(struct geopm_sample_message_s), MPI_INFO_NULL, m_comm, &m_sample_window));
        }
    }

    void TreeCommunicatorLevel::close_recv(void)
    {
        if (m_policy_request != MPI_REQUEST_NULL) {
            check_mpi(MPI_Cancel(&m_policy_request));
            check_mpi(MPI_Request_free(&m_policy_request));
        }
        check_mpi(MPI_Win_free(&m_sample_window));
    }

    SingleTreeCommunicator::SingleTreeCommunicator(GlobalPolicy *global_policy)
        : m_policy(global_policy)
        , m_sample(GEOPM_SAMPLE_INVALID)
    {

    }

    SingleTreeCommunicator::~SingleTreeCommunicator()
    {

    }

    int SingleTreeCommunicator::num_level(void) const
    {
        return 1;
    }

    int SingleTreeCommunicator::root_level(void) const
    {
        return 0;
    }

    int SingleTreeCommunicator::level_rank(int level) const
    {
        return 0;
    }

    int SingleTreeCommunicator::level_size(int level) const
    {
        return 1;
    }

    void SingleTreeCommunicator::send_sample(int level, const struct geopm_sample_message_s &sample)
    {
        m_sample = sample;
    }

    void SingleTreeCommunicator::send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy)
    {

    }

    void SingleTreeCommunicator::get_sample(int level, std::vector<struct geopm_sample_message_s> &sample)
    {
        sample[0] = m_sample;
    }

    void SingleTreeCommunicator::get_policy(int level, struct geopm_policy_message_s &policy)
    {
        m_policy->policy_message(policy);
    }

    size_t SingleTreeCommunicator::overhead_send(void)
    {
        return 0;
    }

}

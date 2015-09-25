/*
 * Copyright (c) 2015, Intel Corporation
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

#include "TreeCommunicator.hpp"
#include "GlobalPolicy.hpp"

namespace geopm
{
    //////////////////////
    // Static constants //
    //////////////////////

    enum mpi_tag_e {
        GEOPM_SAMPLE_TAG,
        GEOPM_POLICY_TAG,
    };

    //////////////////////////////////
    // Static function declarations //
    //////////////////////////////////

    static inline void check_mpi(int err);
    static MPI_Datatype create_sample_mpi_type(void);
    static MPI_Datatype create_policy_mpi_type(void);

    /////////////////////////////////
    // Internal class declarations //
    /////////////////////////////////

    class TreeCommunicatorLevel
    {
        public:
            TreeCommunicatorLevel(MPI_Comm comm, MPI_Datatype sample_mpi_type, MPI_Datatype policy_mpi_type);
            ~TreeCommunicatorLevel();
            /// Check sample mailbox for each child and if all are
            /// full copy them into sample and post a new MPI_Irecv(),
            /// otherwise throw incomplete_sample_error
            void get_sample(std::vector<struct sample_message_s> &sample);
            /// Check policy mailbox and if full copy to m_policy and
            /// post a new MPI_Irecv(). If mailbox is empty set policy
            /// to last known policy.  If mailbox is empty and no
            /// policy has been set throw unknown_policy_error.
            void get_policy(struct geopm_policy_message_s &policy);
            /// Send sample via MPI_Irsend() to root of level, skip if
            /// no recieve has been posted.
            void send_sample(const struct sample_message_s &sample);
            /// Send policy via MPI_Irsend() to all children, skip if
            /// no recieve has been posted.
            void send_policy(const std::vector<struct geopm_policy_message_s> &policy);
            /// Returns the level rank of the calling process.
            int level_rank(void);
        protected:
            void open_recv(void);
            void close_recv(void);
            MPI_Comm m_comm;
            MPI_Datatype m_sample_mpi_type; // MPI data type for sample message
            MPI_Datatype m_policy_mpi_type; // MPI data type for policy message
            int m_size;
            int m_rank;
            std::vector <struct sample_message_s> m_sample_mailbox;
            std::vector<MPI_Request> m_sample_request;
            struct geopm_policy_message_s m_policy_mailbox;
            MPI_Request m_policy_request;
            struct geopm_policy_message_s m_policy;
    };

    /////////////////////////////////
    // Static function definitions //
    /////////////////////////////////

    static inline void check_mpi(int err)
    {
        if (err) {
            throw MPI::Exception(err);
        }
    }

    static MPI_Datatype create_sample_mpi_type(void)
    {
        int blocklength[5] = {1, 1, 1, 1, 1};
        MPI_Datatype mpi_type[5] = {MPI_INT,
                                    MPI_DOUBLE,
                                    MPI_DOUBLE,
                                    MPI_DOUBLE,
                                    MPI_DOUBLE
                                   };
        MPI_Aint offset[5];
        MPI_Datatype result;
        offset[0] = offsetof(struct sample_message_s, phase_id);
        offset[1] = offsetof(struct sample_message_s, runtime);
        offset[2] = offsetof(struct sample_message_s, progress);
        offset[3] = offsetof(struct sample_message_s, energy);
        offset[4] = offsetof(struct sample_message_s, frequency);
        check_mpi(MPI_Type_create_struct(5, blocklength, offset, mpi_type, &result));
        check_mpi(MPI_Type_commit(&result));
        return result;
    }


    static MPI_Datatype create_policy_mpi_type(void)
    {
        int blocklength[5] = {1, 1, 1, 1, 1};
        MPI_Datatype mpi_type[5] = {MPI_INT,
                                    MPI_INT,
                                    MPI_UNSIGNED_LONG,
                                    MPI_INT,
                                    MPI_DOUBLE
                                   };
        MPI_Aint offset[5];
        MPI_Datatype result;

        offset[0] = offsetof(struct geopm_policy_message_s, phase_id);
        offset[1] = offsetof(struct geopm_policy_message_s, mode);
        offset[2] = offsetof(struct geopm_policy_message_s, flags);
        offset[3] = offsetof(struct geopm_policy_message_s, num_sample);
        offset[4] = offsetof(struct geopm_policy_message_s, power_budget);
        check_mpi(MPI_Type_create_struct(5, blocklength, offset, mpi_type, &result));
        check_mpi(MPI_Type_commit(&result));
        return result;
    }

    ///////////////////////////////////
    // TreeCommunicator public API's //
    ///////////////////////////////////

    TreeCommunicator::TreeCommunicator(const std::vector<int> &fan_out, const GlobalPolicy *global_policy, const MPI_Comm &comm)
        : m_fan_out(fan_out)
        , m_comm(fan_out.size())
        , m_global_policy(global_policy)
        , m_level(fan_out.size())
    {
        mpi_type_create();
        comm_create(comm);
        level_create();
    }

    TreeCommunicator::~TreeCommunicator()
    {
        level_destroy();
        comm_destroy();
        mpi_type_destroy();
    }

    int TreeCommunicator::num_level(void)
    {
        return m_num_level;
    }

    int TreeCommunicator::root_level(void)
    {
        return m_fan_out.size();
    }

    int TreeCommunicator::level_rank(int level)
    {
        return m_level[level]->level_rank();
    }

    int TreeCommunicator::level_size(int level)
    {
        int result = 1;
        if (level < (int)m_fan_out.size()) {
            result = *(m_fan_out.end() - level - 1);
        }
        return result;
    }

    void TreeCommunicator::send_sample(int level, const struct sample_message_s &sample)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw std::invalid_argument("Called with level out of range\n");
        }
        m_level[level]->send_sample(sample);
    }

    void TreeCommunicator::send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw std::invalid_argument("Called with level out of range\n");
        }
        else {
            m_level[level]->send_policy(policy);
        }
    }

    void TreeCommunicator::get_sample(int level, std::vector<struct sample_message_s> &sample)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw std::invalid_argument("Called with level out of range\n");
        }
        m_level[level]->get_sample(sample);
    }
    void TreeCommunicator::get_policy(int level, struct geopm_policy_message_s &policy)
    {
        if (level < 0 || level >= num_level()) {
            throw std::invalid_argument("Called with level out of range\n");
        }
        if (level == root_level()) {
            m_global_policy->policy_message(policy);
        }
        else {
            m_level[level]->get_policy(policy);
        }
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
            throw std::runtime_error("Process at root of tree communicator has not mapped the control file.");
        }
        if (rank_cart != 0 && m_global_policy != NULL) {
            throw std::runtime_error("Process not at root of tree communicator has mapped the control file.");
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
        auto level_it = m_level.begin();
        for (; level_it < m_level.end(); ++level_it, ++comm_it) {
            *level_it = new TreeCommunicatorLevel(*comm_it, m_sample_mpi_type, m_policy_mpi_type);
        }
    }

    void TreeCommunicator::level_destroy(void)
    {
        for (auto level_it = m_level.begin(); level_it < m_level.end(); ++level_it) {
            delete *level_it;
        }
    }

    void TreeCommunicator::comm_destroy(void)
    {
        for (auto comm_it = m_comm.begin(); comm_it < m_comm.end(); ++comm_it) {
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
    {
        check_mpi(MPI_Comm_size(comm, &m_size));
        check_mpi(MPI_Comm_rank(comm, &m_rank));
        m_sample_mailbox.resize(m_size);
        m_sample_request.resize(m_size);
        m_policy = GEOPM_UNKNOWN_POLICY;
        open_recv();
    }

    TreeCommunicatorLevel::~TreeCommunicatorLevel()
    {
        close_recv();
    }

    void TreeCommunicatorLevel::get_sample(std::vector<struct sample_message_s> &sample)
    {
        int is_complete;
        int source;
        MPI_Status status;

        for (auto request_it = m_sample_request.begin(); request_it < m_sample_request.end(); ++request_it) {
            check_mpi(MPI_Test(&(*request_it), &is_complete, &status));
            if (!is_complete) {
                throw incomplete_sample_error();
            }
        }
        if (sample.size() < m_sample_mailbox.size()) {
            throw std::range_error("input sample vector too small\n");
        }
        copy(m_sample_mailbox.begin(), m_sample_mailbox.end(), sample.begin());
        source = 0;
        auto request_it = m_sample_request.begin();
        auto sample_it = m_sample_mailbox.begin();
        for (; sample_it < m_sample_mailbox.end();
             ++sample_it, ++request_it, ++source) {
            check_mpi(MPI_Irecv(&(*sample_it), 1, m_sample_mpi_type, source, GEOPM_SAMPLE_TAG, m_comm, &(*request_it)));
        }
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
        if (geopm_is_policy_equal(&policy, &GEOPM_UNKNOWN_POLICY)) {
            throw unknown_policy_error();
        }
    }

    void TreeCommunicatorLevel::send_sample(const struct sample_message_s &sample)
    {
        MPI_Request request;

        // Don't check return code or hold onto request, drop message if receiver not ready
        (void) MPI_Irsend(const_cast<struct sample_message_s*>(&sample), 1, m_sample_mpi_type, 0, GEOPM_SAMPLE_TAG, m_comm, &request);
    }

    void TreeCommunicatorLevel::send_policy(const std::vector<struct geopm_policy_message_s> &policy)
    {
        int dest;
        MPI_Request request;

        if (m_rank != 0) {
            throw std::domain_error("Called send_policy() from rank not at root of level\n");
        }
        dest = 0;
        for (auto policy_it = policy.begin(); policy_it < policy.end(); ++policy_it, ++dest) {
            // Don't check return code or hold onto request, drop message if receiver not ready
            (void) MPI_Irsend(const_cast<struct geopm_policy_message_s*>(&(*policy_it)), 1, m_policy_mpi_type, dest, GEOPM_POLICY_TAG, m_comm, &request);
        }
    }

    int TreeCommunicatorLevel::level_rank(void)
    {
        return m_rank;
    }

    void TreeCommunicatorLevel::open_recv(void)
    {
        check_mpi(MPI_Irecv(&m_policy_mailbox, 1, m_policy_mpi_type, 0, GEOPM_POLICY_TAG, m_comm, &m_policy_request));
        if (m_rank == 0) {
            int source = 0;
            auto request_it = m_sample_request.begin();
            auto sample_it = m_sample_mailbox.begin();
            for (; sample_it < m_sample_mailbox.end();
                 ++sample_it, ++request_it, ++source) {
                check_mpi(MPI_Irecv(&(*sample_it), 1, m_sample_mpi_type, source, GEOPM_SAMPLE_TAG, m_comm, &(*request_it)));
            }
        }
    }

    void TreeCommunicatorLevel::close_recv(void)
    {
        check_mpi(MPI_Cancel(&m_policy_request));
        if (m_rank == 0) {
            for (auto request_it = m_sample_request.begin(); request_it < m_sample_request.end(); ++request_it) {
                check_mpi(MPI_Cancel(&(*request_it)));
            }
        }
    }
}

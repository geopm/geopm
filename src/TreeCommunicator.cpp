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
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <errno.h>
#include <algorithm>
#include <fstream>
#include <system_error>

#include "Exception.hpp"
#include "TreeCommunicator.hpp"
#include "GlobalPolicy.hpp"
#include "SharedMemory.hpp"
#include "geopm_ctl.h"
#include "geopm_message.h"
#include "geopm_env.h"
#include "config.h"
#include "Comm.hpp"

namespace geopm
{
    /////////////////////////////////
    // Internal class declarations //
    /////////////////////////////////

    /// @brief TreeCommunicatorLevel class encapsulates communication functionality on
    /// a per-level basis.
    class TreeCommunicatorLevel
    {
        public:
            TreeCommunicatorLevel(IComm *comm);
            TreeCommunicatorLevel(const TreeCommunicatorLevel &other);
            ~TreeCommunicatorLevel();
            /// Check sample mailbox for each child and if all are full copy
            /// them into sample and reset values in mailbox, otherwise throw
            /// geopm::Exception with err_value() of
            /// GEOPM_ERROR_SAMPLE_INCOMPLETE
            void get_sample(std::vector<struct geopm_sample_message_s> &sample);
            /// Check policy mailbox and set policy to new value
            /// stored there.  If the mailbox has not been modified or
            /// contains GEOPM_POLICY_UNKNOWN for any other reason
            /// throw a geopm::Exception with err_value() of
            /// GEOPM_ERROR_POLICY_UNKNOWN.
            void get_policy(struct geopm_policy_message_s &policy);
            /// Send sample via MPI_Put() to root of level.
            void send_sample(const struct geopm_sample_message_s &sample);
            /// Send any changed policies via MPI_Put() to children.
            void send_policy(const std::vector<struct geopm_policy_message_s> &policy);
            /// Returns the level rank of the calling process.
            int level_rank(void);
            /// Returns number of bytes transfered over the network so far.
            size_t overhead_send(void);
        protected:
            void create_window(void);
            IComm *m_comm;
            int m_size;
            int m_rank;
            struct geopm_sample_message_s *m_sample_mailbox;
            volatile struct geopm_policy_message_s m_policy_mailbox;
            size_t m_sample_window;
            size_t m_policy_window;
            size_t m_overhead_send;
            std::vector<struct geopm_policy_message_s> m_last_policy;
    };

    ///////////////////////////////////
    // TreeCommunicator public API's //
    ///////////////////////////////////

    TreeCommunicator::TreeCommunicator(const std::vector<int> &fan_out, IGlobalPolicy *global_policy, const IComm *comm)
        : m_num_node(0)
        , m_fan_out(fan_out)
        , m_global_policy(global_policy)
        , m_level(fan_out.size(), NULL)
    {
        int num_level = m_fan_out.size();
        int color, key;
        IComm *comm_cart;
        std::vector<int> flags(num_level, 0);
        std::vector<int> coords(num_level, 0);
        std::vector<int> parent_coords(num_level, 0);
        int rank_cart;

        m_num_node = comm->num_rank();

        comm_cart = comm->split(m_fan_out, flags, 1);
        rank_cart = comm_cart->rank();
        comm_cart->coordinate(rank_cart, coords);
        parent_coords = coords;

        /* Tracks if the rank's coordinate is zero for higher order
           dimensions than the depth */
        bool is_all_zero = true;
        IComm *level_comm = NULL;
        m_num_level = 0;
        int level = 0;
        int depth = num_level - 1;
        for (auto level_it = m_level.begin();
             level_it != m_level.end();
             ++level_it, ++level, --depth) {
            if (is_all_zero) {
                parent_coords[depth] = 0;
                color = comm_cart->cart_rank(parent_coords);
                key = rank_cart;
            }
            else {
                color = IComm::M_SPLIT_COLOR_UNDEFINED;
                key = 0;
            }

            level_comm = comm_cart->split(color, key);
            if (level_comm->num_rank()) {
                ++m_num_level;
                // TreeCommunicatorLevel will call MPI_Comm_Free on
                // level_comm in destructor.
                *level_it = new TreeCommunicatorLevel(level_comm);
            } else {
                delete level_comm;
            }

            if (coords[depth] != 0) {
                is_all_zero = false;
            }
        }
        delete comm_cart;

        m_level.resize(m_num_level);
        if (m_global_policy) {
            m_num_level++;
        }

        if (rank_cart == 0 && m_global_policy == NULL) {
            throw Exception("process at root of tree communicator has not mapped the control file",
                            GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }
        if (rank_cart != 0 && m_global_policy != NULL) {
            throw Exception("process not at root of tree communicator has mapped the control file",
                            GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }

        comm->barrier();
    }

    TreeCommunicator::TreeCommunicator(const TreeCommunicator &other)
        : ITreeCommunicator(other)
        , m_num_level(other.m_num_level)
        , m_num_node(other.m_num_node)
        , m_fan_out(other.m_fan_out)
        , m_global_policy(other.m_global_policy)
        , m_level(other.m_level.size())
    {
        auto o_level_it = other.m_level.begin();
        for (auto level_it = m_level.begin();
             level_it != m_level.end();
             ++level_it, ++o_level_it) {
            *level_it = new TreeCommunicatorLevel(**o_level_it);
        }
    }


    TreeCommunicator::~TreeCommunicator()
    {
        for (auto level_it = m_level.rbegin();
             level_it != m_level.rend();
             ++level_it) {
            delete *level_it;
        }
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
            m_level[level]->send_policy(policy);
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

    /////////////////////////////////
    // TreeCommunicatorLevel API's //
    /////////////////////////////////

    TreeCommunicatorLevel::TreeCommunicatorLevel(IComm *comm)
        : m_comm(comm)
        , m_size(0)
        , m_rank(0)
        , m_sample_mailbox(NULL)
        , m_policy_mailbox(GEOPM_POLICY_UNKNOWN)
        , m_sample_window(0)
        , m_policy_window(0)
        , m_overhead_send(0)
    {
        m_size = m_comm->num_rank();
        m_rank = m_comm->rank();
        if (!m_rank) {
            m_last_policy.resize(m_size, GEOPM_POLICY_UNKNOWN);
        }
        create_window();
    }

    TreeCommunicatorLevel::TreeCommunicatorLevel(const TreeCommunicatorLevel &other)
        : m_comm(NULL)
        , m_size(other.m_size)
        , m_rank(other.m_rank)
        , m_sample_mailbox(NULL)
        , m_sample_window(0)
        , m_policy_window(0)
        , m_overhead_send(other.m_overhead_send)
        , m_last_policy(other.m_last_policy)
    {
        m_policy_mailbox.mode = other.m_policy_mailbox.mode;
        m_policy_mailbox.flags = other.m_policy_mailbox.flags;
        m_policy_mailbox.num_sample = other.m_policy_mailbox.num_sample;
        m_policy_mailbox.power_budget = other.m_policy_mailbox.power_budget;
        m_comm = other.m_comm->split();
        create_window();
        std::copy(other.m_sample_mailbox, other.m_sample_mailbox + m_size, m_sample_mailbox);
    }

    TreeCommunicatorLevel::~TreeCommunicatorLevel()
    {
        m_comm->barrier();
        // Destroy sample window
        m_comm->window_destroy(m_sample_window);
        if (m_sample_mailbox) {
            m_comm->free_mem(m_sample_mailbox);
        }
        // Destroy policy window
        m_comm->window_destroy(m_policy_window);
        // Destroy the comm
        delete m_comm;
    }

    void TreeCommunicatorLevel::get_sample(std::vector<struct geopm_sample_message_s> &sample)
    {
        if (m_rank != 0) {
            throw Exception("TreeCommunicatorLevel::get_sample(): Only zero rank of the level can call sample",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (sample.size() < (size_t)m_size) {
            throw Exception("TreeCommunicatorLevel::get_sample(): Input sample vector too small",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        bool is_complete = true;
        m_comm->window_lock(m_sample_window, false, 0, 0);
        for (int i = 0; is_complete && i < m_size; ++i) {
            if (m_sample_mailbox[i].region_id == 0) {
                is_complete = false;
            }
        }
        m_comm->window_unlock(m_sample_window, 0);

        if (!is_complete) {
            throw Exception(__func__, GEOPM_ERROR_SAMPLE_INCOMPLETE, __FILE__, __LINE__);
        }

        m_comm->window_lock(m_sample_window, true, 0, 0);
        std::copy(m_sample_mailbox, m_sample_mailbox + m_size, sample.begin());
        std::fill(m_sample_mailbox, m_sample_mailbox + m_size, GEOPM_SAMPLE_INVALID);
        m_comm->window_unlock(m_sample_window, 0);
    }

    void TreeCommunicatorLevel::get_policy(struct geopm_policy_message_s &policy)
    {
        if (m_rank) {
            m_comm->window_lock(m_policy_window, false, m_rank, 0);
            policy = *((struct geopm_policy_message_s*)(&m_policy_mailbox));
            m_comm->window_unlock(m_policy_window, m_rank);
        }
        else {
            policy = *((struct geopm_policy_message_s *)(&m_policy_mailbox));
        }

        if (geopm_is_policy_equal(&policy, &GEOPM_POLICY_UNKNOWN)) {
            throw Exception("TreeCommunicatorLevel::get_policy",
                            GEOPM_ERROR_POLICY_UNKNOWN, __FILE__, __LINE__);
        }
    }

    void TreeCommunicatorLevel::send_sample(const struct geopm_sample_message_s &sample)
    {
        size_t msg_size = sizeof(struct geopm_sample_message_s);
        if (m_rank) {
            m_comm->window_lock(m_sample_window, true, 0, 0);
            m_comm->window_put((void *) &sample, msg_size, 0, m_rank * msg_size, m_sample_window);
            m_comm->window_unlock(m_sample_window, 0);
            m_overhead_send += msg_size;
        }
        else {
            *m_sample_mailbox = sample;
        }
    }

    void TreeCommunicatorLevel::send_policy(const std::vector<struct geopm_policy_message_s> &policy)
    {
        if (m_rank != 0) {
            throw Exception("Called send_policy() from rank not at root of level",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }

        *((struct geopm_policy_message_s *)(&m_policy_mailbox)) = policy[0];
        m_last_policy[0] = policy[0];
        size_t msg_size = sizeof(struct geopm_policy_message_s);
        int child_rank = 1;
        auto this_it = policy.begin() + 1;
        for (auto last_it = m_last_policy.begin() + 1;
             last_it != m_last_policy.end();
             ++this_it, ++last_it, ++child_rank) {
            if (!geopm_is_policy_equal(&(*this_it), &(*last_it))) {
                m_comm->window_lock(m_policy_window, true, child_rank, 0);
                m_comm->window_put((void *)&(*this_it), msg_size, child_rank, 0, m_policy_window);
                m_comm->window_unlock(m_policy_window, child_rank);
                m_overhead_send += msg_size;
                *last_it = *this_it;
            }
        }
    }

    size_t TreeCommunicatorLevel::overhead_send(void)
    {
        return m_overhead_send;
    }

    int TreeCommunicatorLevel::level_rank(void)
    {
        return m_rank;
    }

    void TreeCommunicatorLevel::create_window(void)
    {
        // Create policy window
        size_t msg_size = sizeof(struct geopm_policy_message_s);
        if (m_rank) {
            m_policy_window = m_comm->window_create(msg_size, (void *)(&m_policy_mailbox));
        }
        else {
            m_policy_window = m_comm->window_create(0, NULL);
        }
        // Create sample window
        if (!m_rank) {
            msg_size = sizeof(struct geopm_sample_message_s);
            m_comm->alloc_mem(m_size * msg_size, (void **)(&m_sample_mailbox));
            m_sample_window = m_comm->window_create(m_size * msg_size, (void *)(m_sample_mailbox));
            std::fill(m_sample_mailbox, m_sample_mailbox + m_size, GEOPM_SAMPLE_INVALID);
        }
        else {
            m_sample_window = m_comm->window_create(0, NULL);
        }
    }

    SingleTreeCommunicator::SingleTreeCommunicator(IGlobalPolicy *global_policy)
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

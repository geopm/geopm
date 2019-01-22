/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <string.h>
#include <cmath>
#include <algorithm>

#include "Comm.hpp"
#include "Exception.hpp"
#include "TreeCommLevel.hpp"
#include "config.h"

namespace geopm
{
    TreeCommLevel::TreeCommLevel(std::shared_ptr<Comm> comm, int num_send_up, int num_send_down)
        : m_comm(comm)
        , m_size(comm->num_rank())
        , m_rank(comm->rank())
        , m_sample_mailbox(nullptr)
        , m_policy_mailbox(nullptr)
        , m_sample_window(0)
        , m_policy_window(0)
        , m_overhead_send(0)
        , m_num_send_up(num_send_up)
        , m_num_send_down(num_send_down)
    {
        if (!m_rank) {
            m_policy_last.resize(m_size, std::vector<double>(num_send_down, 0.0));
        }
        create_window();
    }

    TreeCommLevel::~TreeCommLevel()
    {
        m_comm->barrier();
        // Destroy sample window
        m_comm->window_destroy(m_sample_window);
        if (m_sample_mailbox) {
            m_comm->free_mem(m_sample_mailbox);
        }
        // Destroy policy window
        m_comm->window_destroy(m_policy_window);
        if (m_policy_mailbox) {
            m_comm->free_mem(m_policy_mailbox);
        }
    }

    int TreeCommLevel::level_rank(void) const
    {
        return m_rank;
    }

    void TreeCommLevel::send_up(const std::vector<double> &sample)
    {
        if (sample.size() != m_num_send_up) {
            throw Exception("TreeCommLevel::send_up(): sample vector is not sized correctly.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        size_t msg_size = m_num_send_up * sizeof(double);
        double is_ready = 1.0;
        if (m_rank) {
            size_t base_off = m_rank * (msg_size + sizeof(double));
            m_comm->window_lock(m_sample_window, true, 0, 0);
            m_comm->window_put(&is_ready, sizeof(double), 0, base_off, m_sample_window);
            m_comm->window_put(sample.data(), msg_size, 0, base_off + sizeof(double), m_sample_window);
            m_comm->window_unlock(m_sample_window, 0);
            m_overhead_send += sizeof(double) + msg_size;
        }
        else {
            m_sample_mailbox[0] = 1.0;
            memcpy(m_sample_mailbox + 1, sample.data(), msg_size);
        }
    }

    void TreeCommLevel::send_down(const std::vector<std::vector<double> > &policy)
    {
#ifdef GEOPM_DEBUG
        if (m_rank != 0) {
            throw Exception("TreeCommLevel::send_down() called from rank not at root of level",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        size_t num_down = m_num_send_down;
        if (m_size != (int)policy.size() ||
            std::any_of(policy.begin(), policy.end(),
                        [num_down](std::vector<double> it)
                        {return it.size() != num_down;})) {
            throw Exception("TreeCommLevel::send_down(): policy vector is not sized correctly.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        size_t msg_size = sizeof(double) * m_num_send_down;
        double is_ready = 1.0;
        m_policy_mailbox[0] = is_ready;
        // Copy message to self for rank zero
        memcpy(m_policy_mailbox + 1, policy[0].data(), msg_size);

        for (int child_rank = 1; child_rank != m_size; ++child_rank) {
            if (policy[child_rank] != m_policy_last[child_rank]) {
                m_comm->window_lock(m_policy_window, true, child_rank, 0);
                m_comm->window_put(&is_ready, sizeof(double), child_rank, 0, m_policy_window);
                m_comm->window_put(policy[child_rank].data(), msg_size, child_rank, sizeof(double), m_policy_window);
                m_comm->window_unlock(m_policy_window, child_rank);
                m_overhead_send += sizeof(double) + msg_size;
                m_policy_last[child_rank] = policy[child_rank];
            }
        }
    }

    bool TreeCommLevel::receive_up(std::vector<std::vector<double> > &sample)
    {
#ifdef GEOPM_DEBUG
        if (m_rank != 0) {
            throw Exception("TreeCommunicatorLevel::receive_up(): Only zero rank of the level can call receive_up()",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        size_t num_up = m_num_send_up;
        if (m_size != (int)sample.size() ||
            std::any_of(sample.begin(), sample.end(),
                        [num_up](std::vector<double> it)
                        {return it.size() != num_up;})) {
            throw Exception("TreeCommLevel::send_down(): policy vector is not sized correctly.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        bool is_complete = true;
        m_comm->window_lock(m_sample_window, false, 0, 0);
        for (int child_rank = 0; is_complete && child_rank < m_size; ++child_rank) {
            if (m_sample_mailbox[child_rank * (m_num_send_up + 1)] == 0.0) {
                is_complete = false;
            }
        }
        m_comm->window_unlock(m_sample_window, 0);
        if (is_complete) {
            m_comm->window_lock(m_sample_window, true, 0, 0);
            for (int child_rank = 0; child_rank != m_size; ++child_rank) {
                memcpy(sample[child_rank].data(),
                       m_sample_mailbox + child_rank * (m_num_send_up + 1) + 1,
                       sizeof(double) * m_num_send_up);
                m_sample_mailbox[child_rank * (m_num_send_up + 1)] = 0.0;
            }
            m_comm->window_unlock(m_sample_window, 0);
        }

        is_complete = is_complete &&
                      std::none_of(sample.begin(), sample.end(),
                                   [](const std::vector<double> &vec)
                                   {
                                       return std::any_of(vec.begin(), vec.end(),
                                                          [](double val){return std::isnan(val);});
                                   });
        return is_complete;
    }

    bool TreeCommLevel::receive_down(std::vector<double> &policy)
    {
        bool is_complete = false;
        if (m_rank) {
            m_comm->window_lock(m_policy_window, false, m_rank, 0);
        }
        if (m_policy_mailbox[0] == 1.0) {
            is_complete = true;
            policy.resize(m_num_send_down);
            memcpy(policy.data(), m_policy_mailbox + 1, sizeof(double) * m_num_send_down);
        }
        if (m_rank) {
            m_comm->window_unlock(m_policy_window, m_rank);
        }
        is_complete = is_complete &&
                      std::none_of(policy.begin(), policy.end(),
                                   [](double val){return std::isnan(val);});
        return is_complete;
    }

    size_t TreeCommLevel::overhead_send(void) const
    {
        return m_overhead_send;
    }

    void TreeCommLevel::create_window()
    {
        // Create policy window
        // Note mem_size includes extra is_complete element
        size_t mem_size = sizeof(double) * (m_num_send_down + 1);
        m_comm->alloc_mem(mem_size, (void **)(&m_policy_mailbox));
        memset(m_policy_mailbox, 0, mem_size);
        if (m_rank) {
            m_policy_window = m_comm->window_create(mem_size, (void *)(m_policy_mailbox));
        }
        else {
            m_policy_window = m_comm->window_create(0, NULL);
        }
        // Create sample window
        mem_size = sizeof(double) * m_size * (m_num_send_up + 1);
        m_comm->alloc_mem(mem_size, (void **)(&m_sample_mailbox));
        memset(m_sample_mailbox, 0, mem_size);
        if (!m_rank) {
            m_sample_window = m_comm->window_create(mem_size, (void *)(m_sample_mailbox));
        }
        else {
            m_sample_window = m_comm->window_create(0, NULL);
        }
    }
}

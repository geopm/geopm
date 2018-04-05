/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <limits.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include <cmath>

#include "TreeComm.hpp"
#include "TreeCommLevel.hpp"
#include "Comm.hpp"
#include "config.h"

namespace geopm
{

     TreeComm::TreeComm(std::shared_ptr<IComm> comm,
                        int num_send_down,
                        int num_send_up)
         : TreeComm(comm, num_send_down, num_send_up, fan_out(comm), {})
     {

     }

     TreeComm::TreeComm(std::shared_ptr<IComm> comm,
                        int num_send_down,
                        int num_send_up,
                        const std::vector<int> &fan_out,
                        std::vector<std::unique_ptr<ITreeCommLevel> > mock_level)
        : m_comm(comm)
        , m_root_level(fan_out.size())
        , m_num_node(comm->num_rank()) // Assume that comm has one rank per node
        , m_fan_out(fan_out)
        , m_num_send_down(num_send_down)
        , m_num_send_up(num_send_up)
        , m_level_ctl(std::move(mock_level))
    {
        if (m_level_ctl.size() == 0) {
            std::shared_ptr<IComm> comm_cart(comm->split_cart(m_fan_out));
            m_level_ctl = init_level(comm_cart, m_root_level);
        }
#ifdef GEOPM_DEBUG
        if (m_num_level_ctl > m_root_level) {
            throw Exception("Number of controlled levels greater than tree depth.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        std::reverse(m_fan_out.begin(), m_fan_out.end());
        comm->barrier();
    }

    int TreeComm::num_level_controlled(std::vector<int> coords)
    {
        int result = 0;
        for (auto it = coords.rbegin(); it != coords.rend() && *it == 0; ++it) {
             ++result;
        }
        return result;
    }

    std::vector<std::unique_ptr<ITreeCommLevel> > TreeComm::init_level(std::shared_ptr<IComm> comm_cart, int root_level)
    {
        std::vector<std::unique_ptr<ITreeCommLevel> > result;
        int rank_cart = comm_cart->rank();
        std::vector<int> coords(comm_cart->coordinate(rank_cart));
        m_num_level_ctl = num_level_controlled(coords);
        std::vector<int> parent_coords(coords);
        int level = 0;
        int max_level = m_num_level_ctl;
        if (m_num_level_ctl != root_level) {
            ++max_level;
        }
        for (; level < max_level; ++level) {
            parent_coords[root_level - 1 - level] = 0;
            result.emplace_back(
                new TreeCommLevel(comm_cart->split(
                    comm_cart->cart_rank(parent_coords), rank_cart),
                    m_num_send_up, m_num_send_down));
        }
        for (; level < root_level; ++level) {
            comm_cart->split(IComm::M_SPLIT_COLOR_UNDEFINED, 0);
        }
        return result;
    }

    TreeComm::~TreeComm()
    {

    }

    int TreeComm::num_level_controlled(void) const
    {
        return m_num_level_ctl;
    }

    int TreeComm::root_level(void) const
    {
        return m_root_level;
    }

    int TreeComm::level_rank(int level) const
    {
        if (level < 0 || level >= m_num_level_ctl) {
            throw Exception("TreeComm::level_rank()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_level_ctl[level]->level_rank();
    }

    int TreeComm::level_size(int level) const
    {
        if (level < 0 || level >= (int)m_fan_out.size()) {
            throw Exception("TreeComm::level_size()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_fan_out[m_fan_out.size() - level - 1];
    }

    void TreeComm::send_up(int level, const std::vector<double> &sample)
    {
        if (level < 0 || (level != 0 && level >= m_num_level_ctl)) {
            throw Exception("TreeComm::send_up()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level_ctl[level]->send_up(sample);
    }

    void TreeComm::send_down(int level, const std::vector<std::vector<double> > &policy)
    {
        if (level < 0 || level >= m_num_level_ctl) {
            throw Exception("TreeComm::send_down()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level_ctl[level]->send_down(policy);
    }

    bool TreeComm::receive_up(int level, std::vector<std::vector<double> > &sample)
    {
        if (level < 0 || level >= m_num_level_ctl) {
            throw Exception("TreeComm::receive_up()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_level_ctl[level]->receive_up(sample);
    }

    bool TreeComm::receive_down(int level, std::vector<double> &policy)
    {
        if (level < 0 || (level != 0 && level >= m_num_level_ctl)) {
            throw Exception("TreeComm::receive_down()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_level_ctl[level]->receive_down(policy);
    }

    size_t TreeComm::overhead_send(void) const
    {
        size_t result = 0;
        for (auto it = m_level_ctl.begin(); it != m_level_ctl.end(); ++it) {
            result += (*it)->overhead_send();
        }
        return result;
    }

    void TreeComm::broadcast_string(const std::string &str)
    {
        char buffer[NAME_MAX];
        if (str.size() >= NAME_MAX) {
            throw Exception("TreeComm::broadcast_string(): string too long.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        strncpy(buffer, str.c_str(), sizeof(buffer));
        m_comm->broadcast(buffer, sizeof(buffer), 0);
    }

    std::string TreeComm::broadcast_string(void)
    {
        char buffer[NAME_MAX];
        m_comm->broadcast(buffer, sizeof(buffer), 0);
        std::string result(buffer, NAME_MAX - 1);
        return result;
    }

    std::vector<int> ITreeComm::fan_out(const std::shared_ptr<IComm> &comm)
    {
        std::vector<int> fan_out;
        int num_nodes = comm->num_rank();
        if (num_nodes > 1) {
            int num_fan_out = 1;
            fan_out.resize(num_fan_out);
            fan_out[0] = num_nodes;

            while (fan_out[0] > M_MAX_FAN_OUT && fan_out[num_fan_out - 1] != 1) {
                ++num_fan_out;
                fan_out.resize(num_fan_out);
                std::fill(fan_out.begin(), fan_out.end(), 0);
                comm->dimension_create(num_nodes, fan_out);
            }

            if (num_fan_out > 1 && fan_out[num_fan_out - 1] == 1) {
                --num_fan_out;
                fan_out.resize(num_fan_out);
            }
            std::reverse(fan_out.begin(), fan_out.end());
        }
        return fan_out;
    }
}

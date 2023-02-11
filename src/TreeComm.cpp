/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "TreeComm.hpp"

#include <limits.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include <cmath>

#include "Environment.hpp"

#include "geopm/Exception.hpp"
#include "TreeCommLevel.hpp"
#include "Comm.hpp"
#include "config.h"

namespace geopm
{

    TreeCommImp::TreeCommImp(std::shared_ptr<Comm> comm,
                             int num_send_down,
                             int num_send_up)
        : TreeCommImp(comm, fan_out(comm), 0, num_send_down, num_send_up, {})
    {

    }

    TreeCommImp::TreeCommImp(std::shared_ptr<Comm> comm,
                             const std::vector<int> &fan_out,
                             int num_level_ctl,
                             int num_send_down,
                             int num_send_up,
                             std::vector<std::shared_ptr<TreeCommLevel> > mock_level)
        : m_comm(comm)
        , m_fan_out(fan_out)
        , m_root_level(fan_out.size())
        , m_num_level_ctl(num_level_ctl)
        , m_max_level(m_root_level == m_num_level_ctl ? m_num_level_ctl : m_num_level_ctl + 1)
        , m_num_node(comm->num_rank()) // Assume that comm has one rank per node
        , m_num_send_down(num_send_down)
        , m_num_send_up(num_send_up)
        , m_level_ctl(std::move(mock_level))
    {
        if (m_level_ctl.size() == 0) {
            std::shared_ptr<Comm> comm_cart = comm->split_cart(m_fan_out);
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

    int TreeCommImp::num_level_controlled(const std::vector<int> &coords)
    {
        int result = 0;
        for (auto it = coords.rbegin(); it != coords.rend() && *it == 0; ++it) {
            ++result;
        }
        return result;
    }

    int TreeCommImp::max_level() const
    {
        return m_max_level;
    }

    std::vector<std::shared_ptr<TreeCommLevel> > TreeCommImp::init_level(std::shared_ptr<Comm> comm_cart, int root_level)
    {
        std::vector<std::shared_ptr<TreeCommLevel> > result;
        int rank_cart = comm_cart->rank();
        std::vector<int> coords = comm_cart->coordinate(rank_cart);
        m_num_level_ctl = num_level_controlled(coords);
        std::vector<int> parent_coords = coords;
        int level = 0;
        m_max_level = m_num_level_ctl;
        if (m_num_level_ctl != root_level) {
            ++m_max_level;
        }
        for (; level < m_max_level; ++level) {
            parent_coords[root_level - 1 - level] = 0;
            result.emplace_back(
                std::make_shared<TreeCommLevelImp>(comm_cart->split(
                                                   comm_cart->cart_rank(parent_coords), rank_cart),
                                                   m_num_send_up, m_num_send_down));
        }
        for (; level < root_level; ++level) {
            comm_cart->split(Comm::M_SPLIT_COLOR_UNDEFINED, 0);
        }
        return result;
    }

    TreeCommImp::~TreeCommImp()
    {

    }

    int TreeCommImp::num_level_controlled(void) const
    {
        return m_num_level_ctl;
    }

    int TreeCommImp::root_level(void) const
    {
        return m_root_level;
    }

    int TreeCommImp::level_rank(int level) const
    {
        if (level < 0 || level >= m_max_level) {
            throw Exception("TreeCommImp::level_rank()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_level_ctl[level]->level_rank();
    }

    int TreeCommImp::level_size(int level) const
    {
        if (level < 0 || level >= (int)m_fan_out.size()) {
            throw Exception("TreeCommImp::level_size()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_fan_out[level];
    }

    void TreeCommImp::send_up(int level, const std::vector<double> &sample)
    {
        if (level < 0 || (level != 0 && level >= m_max_level)) {
            throw Exception("TreeCommImp::send_up()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level_ctl[level]->send_up(sample);
    }

    void TreeCommImp::send_down(int level, const std::vector<std::vector<double> > &policy)
    {
        if (level < 0 || level >= m_num_level_ctl) {
            throw Exception("TreeCommImp::send_down()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level_ctl[level]->send_down(policy);
    }

    bool TreeCommImp::receive_up(int level, std::vector<std::vector<double> > &sample)
    {
        if (level < 0 || level >= m_num_level_ctl) {
            throw Exception("TreeCommImp::receive_up()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_level_ctl[level]->receive_up(sample);
    }

    bool TreeCommImp::receive_down(int level, std::vector<double> &policy)
    {
        if (level < 0 || (level != 0 && level >= m_max_level)) {
            throw Exception("TreeCommImp::receive_down()",
                            GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        return m_level_ctl[level]->receive_down(policy);
    }

    size_t TreeCommImp::overhead_send(void) const
    {
        size_t result = 0;
        for (auto it = m_level_ctl.begin(); it != m_level_ctl.end(); ++it) {
            result += (*it)->overhead_send();
        }
        return result;
    }

    std::vector<int> TreeComm::fan_out(const std::shared_ptr<Comm> &comm)
    {
        std::vector<int> fan_out;
        int num_nodes = comm->num_rank();
        if (num_nodes > 1) {
            int num_fan_out = 1;
            fan_out.resize(num_fan_out);
            fan_out[0] = num_nodes;

            const int max_fan_out = environment().max_fan_out();
            while (fan_out[0] > max_fan_out && fan_out[num_fan_out - 1] != 1) {
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

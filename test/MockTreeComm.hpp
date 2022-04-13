/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKTREECOMM_HPP_INCLUDE
#define MOCKTREECOMM_HPP_INCLUDE

#include <map>
#include <set>
#include <vector>

#include "gmock/gmock.h"

#include "TreeComm.hpp"

class MockTreeComm : public geopm::TreeComm
{
    public:
        MOCK_METHOD(int, num_level_controlled, (), (const, override));
        MOCK_METHOD(int, max_level, (), (const, override));
        MOCK_METHOD(int, root_level, (), (const, override));
        MOCK_METHOD(int, level_rank, (int level), (const, override));
        MOCK_METHOD(int, level_size, (int level), (const, override));

        void send_up(int level, const std::vector<double> &sample) override
        {
            ++m_num_send;
            m_levels_sent_up.insert(level);
            m_data_sent_up[level] = sample;
        }
        void send_up_mock_child(int level, int child_idx,
                                const std::vector<double> &sample)
        {
            m_data_sent_up_child[{ level, child_idx }] = sample;
        }
        void send_down(int level, const std::vector<std::vector<double> > &policy) override
        {
            ++m_num_send;
            m_levels_sent_down.insert(level);
            if (policy.size() == 0) {
                throw std::runtime_error(
                    "MockTreeComm::send_down(): policy vector was wrong size");
            }
            m_data_sent_down[level] = policy[0]; /// @todo slightly wrong
        }
        bool receive_up(int level, std::vector<std::vector<double> > &sample) override
        {
            if (m_data_sent_up.find(level) == m_data_sent_up.end()) {
                return false;
            }
            ++m_num_recv;
            m_levels_rcvd_up.insert(level);
            int child_idx = 0;
            for (auto &vec : sample) {
                if (m_data_sent_up_child.find({ level, child_idx }) !=
                    m_data_sent_up_child.end()) {
                    vec = m_data_sent_up_child.at({ level, child_idx });
                }
                else {
                    vec = m_data_sent_up.at(level);
                }
                ++child_idx;
            }
            return true;
        }
        bool receive_down(int level, std::vector<double> &policy) override
        {
            if (m_data_sent_down.find(level) == m_data_sent_down.end()) {
                return false;
            }
            ++m_num_recv;
            m_levels_rcvd_down.insert(level);
            policy = m_data_sent_down.at(level);
            return true;
        }
        MOCK_METHOD(size_t, overhead_send, (), (const, override));
        int num_send(void)
        {
            return m_num_send;
        }
        int num_recv(void)
        {
            return m_num_recv;
        }
        std::set<int> levels_sent_down(void)
        {
            return m_levels_sent_down;
        }
        std::set<int> levels_rcvd_down(void)
        {
            return m_levels_rcvd_down;
        }
        std::set<int> levels_sent_up(void)
        {
            return m_levels_sent_up;
        }
        std::set<int> levels_rcvd_up(void)
        {
            return m_levels_rcvd_up;
        }
        void reset_spy(void)
        {
            m_num_send = 0;
            m_num_recv = 0;
            m_levels_sent_down.clear();
            m_levels_sent_up.clear();
            m_levels_rcvd_down.clear();
            m_levels_rcvd_up.clear();
        }

    private:
        // map from level -> last sent data
        std::map<int, std::vector<double> > m_data_sent_up;
        std::map<int, std::vector<double> > m_data_sent_down;
        // map from level, child -> last sent data
        std::map<std::pair<int, int>, std::vector<double> > m_data_sent_up_child;
        int m_num_send = 0;
        int m_num_recv = 0;
        std::set<int> m_levels_sent_down;
        std::set<int> m_levels_rcvd_down;
        std::set<int> m_levels_sent_up;
        std::set<int> m_levels_rcvd_up;
};

#endif

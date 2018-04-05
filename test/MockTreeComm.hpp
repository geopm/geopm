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

#ifndef MOCKTREECOMM_HPP_INCLUDE
#define MOCKTREECOMM_HPP_INCLUDE

#include "TreeComm.hpp"

class MockTreeComm : public geopm::ITreeComm
{
    public:
        MOCK_CONST_METHOD0(num_level_controlled,
                           int(void));
        MOCK_CONST_METHOD0(root_level,
                           int(void));
        MOCK_CONST_METHOD1(level_rank,
                           int(int level));
        MOCK_CONST_METHOD1(level_size,
                           int(int level));

        void send_up(int level, const std::vector<double> &sample) override
        {
            std::cout << "tree send up to level " << level+1 << std::endl;
            m_data_sent_up[level+1] = sample;
        }
        void send_down(int level, const std::vector<std::vector<double> > &policy) override
        {
            std::cout << "tree send down to level " << level-1 << std::endl;
            m_data_sent_down[level-1] = policy[0]; /// @todo slightly wrong
        }
        bool receive_up(int level, std::vector<std::vector<double> > &sample)
        {
            std::cout << "tree recv up level " << level << std::endl;
            for (auto &vec : sample) {
                vec = m_data_sent_up.at(level);
            }
            return true;
        }
        bool receive_down(int level, std::vector<double> &policy)
        {
            std::cout << "tree recv down level " << level << std::endl;
            policy = m_data_sent_down.at(level);
            return true;
        }
        MOCK_CONST_METHOD0(overhead_send,
                     size_t(void));
        MOCK_METHOD1(broadcast_string,
                     void(const std::string &str));
        MOCK_METHOD0(broadcast_string,
                     std::string(void));

    private:
        // map from level -> last sent data
        std::map<int, std::vector<double> > m_data_sent_up;
        std::map<int, std::vector<double> > m_data_sent_down;

};

#endif

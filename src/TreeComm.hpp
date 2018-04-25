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

#ifndef TREECOMM_HPP_INCLUDE
#define TREECOMM_HPP_INCLUDE

#include <vector>

namespace geopm
{
    class IComm;
    class ITreeCommLevel;

    class ITreeComm
    {
        public:
            ITreeComm() = default;
            virtual ~ITreeComm() = default;
            virtual int num_level_controlled(void) const = 0;
            virtual int root_level(void) const = 0;
            virtual int level_rank(int level) const = 0;
            virtual int level_size(int level) const = 0;
            virtual int level_num_leaf(int level) const = 0;
            virtual void send_up(int level, const std::vector<double> &sample) = 0;
            virtual void send_down(int level, const std::vector<std::vector<double> > &policy) = 0;
            virtual bool receive_up(int level, std::vector<std::vector<double> > &sample) = 0;
            virtual bool receive_down(int level, std::vector<double> &policy) = 0;
            virtual size_t overhead_send(void) const = 0;
            virtual void broadcast_string(const std::string &str) = 0;
            virtual std::string broadcast_string(void) = 0;
            static std::vector<int> fan_out(const std::shared_ptr<IComm> &comm);
        private:
            enum m_tree_comm_const_e {
                M_MAX_FAN_OUT = 16,
            };
    };

    class TreeComm : public ITreeComm
    {
        public:
            TreeComm(std::shared_ptr<IComm> comm,
                     int num_send_down,
                     int num_send_up);
            TreeComm(std::shared_ptr<IComm> comm,
                     const std::vector<int> &fan_out,
                     int num_level_ctl,
                     int num_send_down,
                     int num_send_up,
                     std::vector<std::unique_ptr<ITreeCommLevel> > mock_level);
            virtual ~TreeComm();
            int num_level_controlled(void) const override;
            int root_level(void) const override;
            int level_rank(int level) const override;
            int level_size(int level) const override;
            int level_num_leaf(int level) const override;
            void send_down(int level, const std::vector<std::vector<double> > &policy) override;
            void send_up(int level, const std::vector<double> &sample) override;
            bool receive_down(int level, std::vector<double> &policy) override;
            bool receive_up(int level, std::vector<std::vector<double> > &sample) override;
            size_t overhead_send(void) const override;
            void broadcast_string(const std::string &str) override;
            std::string broadcast_string(void) override;
        private:
            int num_level_controlled(std::vector<int> coords);
            std::vector<std::unique_ptr<ITreeCommLevel> > init_level(
                std::shared_ptr<IComm> comm_cart, int root_level);
            std::shared_ptr<IComm> m_comm;
            /// Tree fan out from root to leaf. Note levels go from
            /// leaf to root
            std::vector<int> m_fan_out;
            int m_root_level;
            /// Number of levels this rank participates in
            int m_num_level_ctl;
            /// @brief Number of nodes in the job.
            int m_num_node;
            int m_num_send_down;
            int m_num_send_up;
            std::vector<std::unique_ptr<ITreeCommLevel> > m_level_ctl;
    };
}

#endif

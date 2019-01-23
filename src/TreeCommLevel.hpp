/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef TREECOMMLEVEL_HPP_INCLUDE
#define TREECOMMLEVEL_HPP_INCLUDE

#include <vector>
#include <memory>

namespace geopm
{
    class Comm;

    class ITreeCommLevel
    {
        public:
            ITreeCommLevel() = default;
            virtual ~ITreeCommLevel() = default;
            /// @brief Returns the rank for this level.
            virtual int level_rank(void) const = 0;
            /// @brief Send samples up to the parent.
            virtual void send_up(const std::vector<double> &sample) = 0;
            /// @brief Send policies down to children.
            virtual void send_down(const std::vector<std::vector<double> > &policy) = 0;
            /// @brief Receive samples up from children.
            virtual bool receive_up(std::vector<std::vector<double> > &sample) = 0;
            /// @brief Receive policies down from the parent.
            virtual bool receive_down(std::vector<double> &policy) = 0;
            /// @brief Returns the total number of bytes sent at this
            ///        level.
            virtual size_t overhead_send(void) const = 0;
    };

    class TreeCommLevel : public ITreeCommLevel
    {
        public:
            TreeCommLevel(std::shared_ptr<Comm> comm, int num_send_up, int num_send_down);
            virtual ~TreeCommLevel();
            int level_rank(void) const override;
            void send_up(const std::vector<double> &sample) override;
            void send_down(const std::vector<std::vector<double> > &policy) override;
            bool receive_up(std::vector<std::vector<double> > &sample) override;
            bool receive_down(std::vector<double> &policy) override;
            size_t overhead_send(void) const override;
        private:
            void create_window();
            std::shared_ptr<Comm> m_comm;
            int m_size;
            int m_rank;
            double *m_sample_mailbox;
            double *m_policy_mailbox;
            size_t m_sample_window;
            size_t m_policy_window;
            size_t m_overhead_send;
            std::vector<std::vector<double> > m_policy_last;
            size_t m_num_send_up;
            size_t m_num_send_down;
    };
}

#endif

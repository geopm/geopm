/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TREECOMMLEVEL_HPP_INCLUDE
#define TREECOMMLEVEL_HPP_INCLUDE

#include <vector>
#include <memory>

namespace geopm
{
    class TreeCommLevel
    {
        public:
            TreeCommLevel() = default;
            virtual ~TreeCommLevel() = default;
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

    class Comm;

    class TreeCommLevelImp : public TreeCommLevel
    {
        public:
            TreeCommLevelImp(std::shared_ptr<Comm> comm, int num_send_up, int num_send_down);
            TreeCommLevelImp(const TreeCommLevelImp &other) = delete;
            TreeCommLevelImp &operator=(const TreeCommLevelImp &other) = delete;
            virtual ~TreeCommLevelImp();
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

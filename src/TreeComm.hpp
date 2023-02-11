/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TREECOMM_HPP_INCLUDE
#define TREECOMM_HPP_INCLUDE

#include <stddef.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace geopm
{
    class Comm;

    class TreeComm
    {
        public:
            TreeComm() = default;
            virtual ~TreeComm() = default;
            /// @brief Returns the number of tree levels controlled by
            ///        the Controller on this node.  This determines
            ///        which levels can be used for send_down() and
            ///        receive_up().
            virtual int num_level_controlled(void) const = 0;
            /// @brief Returns the number of tree levels participated
            ///        in by the Controller on this node.  This
            ///        determines which levels can be used for sending
            ///        or receiving, including with the parent.
            virtual int max_level(void) const = 0;
            /// @brief Returns the level of the root of the tree,
            ///        which is equal to the number of levels in the
            ///        tree.
            virtual int root_level(void) const = 0;
            /// @brief Returns the rank of the given level.
            virtual int level_rank(int level) const = 0;
            /// @brief Returns the number of children for each parent
            ///        in the given level.
            virtual int level_size(int level) const = 0;
            /// @brief Send samples up to the parent within a level.
            virtual void send_up(int level, const std::vector<double> &sample) = 0;
            /// @brief Send policies down to children within a level.
            virtual void send_down(int level, const std::vector<std::vector<double> > &policy) = 0;
            /// @brief Receive samples from children within a level.
            virtual bool receive_up(int level, std::vector<std::vector<double> > &sample) = 0;
            /// @brief Receive policies from the parent within a level.
            virtual bool receive_down(int level, std::vector<double> &policy) = 0;
            /// @brief Returns the total number of bytes sent from the
            ///        entire tree.
            virtual size_t overhead_send(void) const = 0;
            /// @brief Returns the number of children at each level.
            static std::vector<int> fan_out(const std::shared_ptr<Comm> &comm);
    };

    class TreeCommLevel;

    class TreeCommImp : public TreeComm
    {
        public:
            TreeCommImp(std::shared_ptr<Comm> comm,
                        int num_send_down,
                        int num_send_up);
            TreeCommImp(std::shared_ptr<Comm> comm,
                        const std::vector<int> &fan_out,
                        int num_level_ctl,
                        int num_send_down,
                        int num_send_up,
                        std::vector<std::shared_ptr<TreeCommLevel> > mock_level);
            virtual ~TreeCommImp();
            int num_level_controlled(void) const override;
            int max_level(void) const override;
            int root_level(void) const override;
            int level_rank(int level) const override;
            int level_size(int level) const override;
            void send_down(int level, const std::vector<std::vector<double> > &policy) override;
            void send_up(int level, const std::vector<double> &sample) override;
            bool receive_down(int level, std::vector<double> &policy) override;
            bool receive_up(int level, std::vector<std::vector<double> > &sample) override;
            size_t overhead_send(void) const override;
        private:
            int num_level_controlled(const std::vector<int> &coords);
            std::vector<std::shared_ptr<TreeCommLevel> > init_level(
                std::shared_ptr<Comm> comm_cart, int root_level);
            std::shared_ptr<Comm> m_comm;
            /// Tree fan out from root to leaf. Note levels go from
            /// leaf to root
            std::vector<int> m_fan_out;
            int m_root_level;
            /// Number of levels the rank controls
            int m_num_level_ctl;
            /// Number of levels this rank participates in, including its parent
            int m_max_level;
            /// @brief Number of nodes in the job.
            int m_num_node;
            int m_num_send_down;
            int m_num_send_up;
            std::vector<std::shared_ptr<TreeCommLevel> > m_level_ctl;
    };
}

#endif

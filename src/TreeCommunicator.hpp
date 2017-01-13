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

#ifndef TREECOMMUNICATOR_HPP_INCLUDE
#define TREECOMMUNICATOR_HPP_INCLUDE

#include <vector>
#include <mpi.h>
#include <pthread.h>

#include "geopm_message.h"
#include "GlobalPolicy.hpp"

namespace geopm
{
    void check_mpi(int err);

    class TreeCommunicatorRoot;
    class TreeCommunicatorLevel;
    class IGlobalPolicy;

    /// @brief Class which enables inter-process communication for
    ///        geopm.
    ///
    /// The TreeCommunicator is used by the Controller to facilitate
    /// inter-node communication for passing samples up and policies
    /// down the control hierarchy.  It leverages MPI to obtain
    /// topology information to optimize communication pattern, and
    /// for non-blocking communication calls.
    class ITreeCommunicator
    {
        public:
            ITreeCommunicator() {}
            ITreeCommunicator(const ITreeCommunicator &other) {}
            virtual ~ITreeCommunicator() {}
            virtual int num_level(void) const = 0;
            virtual int root_level(void) const = 0;
            virtual int level_rank(int level) const = 0;
            virtual int level_size(int level) const = 0;
            virtual void send_sample(int level, const std::vector<double> &sample) = 0;
            virtual void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy) = 0;
            virtual void get_sample(int level, std::vector<double> &sample) = 0;
            virtual void get_policy(int level, struct geopm_policy_message_s &policy) = 0;
            virtual size_t overhead_send(void) = 0;
            virtual void fan_out(std::vector<int> &fanout) = 0;
    };

    class TreeCommunicator : public ITreeCommunicator
    {
        public:
            /// @brief TreeCommunicator constructor.
            ///
            /// User provides the geometry of the balanced tree, a
            /// GlobalPolicy object and an MPI communicator.  The
            /// geometry is specified by giving the fan out at each
            /// level of the tree (the fan out is the same for all
            /// nodes at each level).  This tree defines the
            /// communication pattern used to send samples up and
            /// policies down.  Note that the product of the fan out
            /// values must equal the size of the MPI communicator
            /// passed.  The GlobalPolicy provides the over all policy
            /// constraints used to dictate the policy at the root of
            /// the tree.  The MPI communicator encompasses all
            /// compute nodes under geopm control and the
            /// communicators used are derived from the given
            /// communicator.
            ///
            /// @param [in] fan_out Vector of fan out values for each
            ///        level ordered from root to leaves.
            ///
            /// @param [in] global_policy Policy enforced at the root
            ///        of the tree.
            ///
            /// @param [in] comm All ranks in MPI communicator
            ///        participate in the tree.
            TreeCommunicator(const std::vector<int> &fan_out, IGlobalPolicy *global_policy, const MPI_Comm &comm, int sample_size);
            TreeCommunicator(const TreeCommunicator &other);
            /// @brief TreeCommunicator destructor, virtual.
            virtual ~TreeCommunicator();
            int num_level(void) const;
            int root_level(void) const;
            int level_rank(int level) const;
            int level_size(int level) const;
            void send_sample(int level, const std::vector<double> &sample);
            void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy);
            void get_sample(int level, std::vector<double> &sample);
            void get_policy(int level, struct geopm_policy_message_s &policy);
            size_t overhead_send(void);
            void fan_out(std::vector<int> &fanout);
        protected:
            /// @brief Constructor helper to instantiate
            ///        sub-communicators.
            void comm_create(const MPI_Comm &comm);
            /// @brief Constructor helper to instantiate the level
            ///        specific objects.
            void level_create(void);
            /// @brief Destructor helper to free resources of level
            ///        specific objects.
            void level_destroy(void);
            /// @brief Destructor helper to free resources of
            ///        sub-communicators.
            void comm_destroy(void);
            /// Number of levels this rank participates in
            int m_num_level;
            /// @brief Number of nodes in the job.
            int m_num_node;
            /// Tree fan out from root to leaf. Note levels go from
            /// leaf to root
            std::vector<int> m_fan_out;
            /// GlobalPolicy object defining the policy
            IGlobalPolicy *m_global_policy;
            /// Intermediate levels
            std::vector<TreeCommunicatorLevel *> m_level;
    };

    class SingleTreeCommunicator : public ITreeCommunicator
    {
        public:
            /// @brief SingleTreeCommunicator constructor.
            ///
            /// Supports the TreeCommunicator interface when
            /// allocation is running on one node only.
            ///
            /// @param [in] global_policy Determines the policy for
            ///        the run.
            SingleTreeCommunicator(const std::vector<int> &fan_out, IGlobalPolicy *global_policy, int sample_size);
            SingleTreeCommunicator(const SingleTreeCommunicator &other);
            virtual ~SingleTreeCommunicator();
            int num_level(void) const;
            int root_level(void) const;
            int level_rank(int level) const;
            int level_size(int level) const;
            void send_sample(int level, const std::vector<double> &sample);
            void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy);
            void get_sample(int level, std::vector<double> &sample);
            void get_policy(int level, struct geopm_policy_message_s &policy);
            size_t overhead_send(void);
            void fan_out(std::vector<int> &fanout);
        protected:
            IGlobalPolicy *m_policy;
            std::vector<double> m_sample;
            /// Tree fan out from root to leaf. Note levels go from
            /// leaf to root
            std::vector<int> m_fan_out;
    };

}

#endif

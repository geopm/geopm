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
    class GlobalPolicy;

    /// @brief Class which enables inter-process communication for
    ///        geopm.
    ///
    /// The TreeCommunicator is used by the Controller to facilitate
    /// inter-node communication for passing samples up and policies
    /// down the control hierarchy.  It leverages MPI to obtain
    /// topology information to optimize communication pattern, and
    /// for non-blocking communication calls.
    class TreeCommunicatorBase
    {
        public:
            TreeCommunicatorBase() {}
            TreeCommunicatorBase(const TreeCommunicatorBase &other) {}
            virtual ~TreeCommunicatorBase() {}
            /// @brief The number of levels for calling process.
            ///
            /// Each of the processes in the communicator passed at
            /// construction participate in operations at the leaf
            /// level.  Some processes have responsibilities at higher
            /// levels of the control hierarchy.  This method returns
            /// the number of levels (from leaf upward in the tree)
            /// that the calling process participates in.
            ///
            /// @return The number of levels of which the calling
            ///         process is a member.
            virtual int num_level(void) const = 0;
            /// @brief The level of root (maximum level for any rank).
            ///
            /// At construction time the user provides a vector of fan
            /// out values which define the geometry to the balanced
            /// tree.  This method returns the length of that vector
            /// plus one, which is number of levels of the tree
            /// including the root.
            ///
            /// @return Number of levels in the balanced tree.
            virtual int root_level(void) const = 0;
            /// @brief The rank of the calling process among children
            ///        with the same parent node.
            ///
            /// Siblings in the tree have a local rank which is
            /// returned by this method.  The process with local level
            /// rank zero participates in the next level up and acts
            /// as the parent node.  All other siblings report to the
            /// zero local level rank process and do not participate
            /// in higher levels of the tree.
            ///
            /// @param [in] level The level of the tree to query.
            ///
            /// @return The local level rank for the calling process.
            virtual int level_rank(int level) const = 0;
            /// @brief Number of siblings at a level.
            ///
            /// Returns the number of siblings that a process
            /// participating in the responsibilities of the given
            /// level has associated with it.  Note that if level is
            /// zero than this is the number of leaf level processes
            /// that report to a single aggregator at level one, and
            /// if level is root_level() the result is one.  This is
            /// essentially the reverse of the fan out vector provided
            /// at construction with one appended to it.
            ///
            /// @param [in] level The level of the tree to query.
            ///
            /// @return The number of siblings.
            virtual int level_size(int level) const = 0;
            /// @brief Send sample up one level.
            ///
            /// Send sample to root of the level.  If no receive has
            /// been posted samples are not sent and no exception is
            /// thrown.
            ///
            /// @param [in] level The level that is sending the sample.
            ///
            /// @param [in] sample The sample message sent from the
            ///        local process.
            virtual void send_sample(int level, const struct geopm_sample_message_s &sample) = 0;
            /// @brief Send policy down one level.
            ///
            /// Called only by a root process of the level.  Send
            /// policy down to each member of the level.  If no
            /// receive has been posted then the policy is not sent
            /// and no exception is thrown.
            ///
            /// @param [in] level The level to where the policy is
            ///        being sent down.
            ///
            /// @param [in] policy A vector of policies, one for each
            ///        child node of the calling process to be sent.
            virtual void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy) = 0;
            /// @brief Get samples from children.
            ///
            /// Called only by root process of the level.  Output is a
            /// vector of samples from each member of the level.
            /// Throws geopm::Exception with err_value() of
            /// GEOPM_ERROR_SAMPLE_INCOMPLETE if message has not been
            /// received by all members of the level since last call.
            ///
            /// @param [in] level The level which is sending samples up.
            ///
            /// @param [out] sample A vector of sample messages
            ///        collected from the level.
            virtual void get_sample(int level, std::vector<struct geopm_sample_message_s> &sample) = 0;
            /// @brief Get policy from parent.
            ///
            /// Record current policy for calling process on the
            /// level.  Will post another receive for the next update
            /// if the root of the level has sent an update since last
            /// call.  otherwise returns cached policy.  If no policy
            /// has been sent since start-up throws A geopm::Exception
            /// with err_value() of GEOPM_ERROR_POLICY_UNKNOWN.
            ///
            /// @param [in] level The level where the policy is being
            /// received.
            ///
            /// @param [out] policy The current policy message for the
            ///        calling process at the given level.
            virtual void get_policy(int level, struct geopm_policy_message_s &policy) = 0;
            virtual size_t overhead_send(void) = 0;
    };

    class TreeCommunicator : public TreeCommunicatorBase
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
            TreeCommunicator(const std::vector<int> &fan_out, GlobalPolicy *global_policy, const MPI_Comm &comm);
            TreeCommunicator(const TreeCommunicator &other);
            /// @brief TreeCommunicator destructor, virtual.
            virtual ~TreeCommunicator();
            int num_level(void) const;
            int root_level(void) const;
            int level_rank(int level) const;
            int level_size(int level) const;
            void send_sample(int level, const struct geopm_sample_message_s &sample);
            void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy);
            void get_sample(int level, std::vector<struct geopm_sample_message_s> &sample);
            void get_policy(int level, struct geopm_policy_message_s &policy);
            size_t overhead_send(void);
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
            GlobalPolicy *m_global_policy;
            /// Intermediate levels
            std::vector<TreeCommunicatorLevel *> m_level;
    };

    class SingleTreeCommunicator : public TreeCommunicatorBase
    {
        public:
            /// @brief SingleTreeCommunicator constructor.
            ///
            /// Supports the TreeCommunicator interface when
            /// allocation is running on one node only.
            ///
            /// @param [in] global_policy Determines the policy for
            ///        the run.
            SingleTreeCommunicator(GlobalPolicy *global_policy);
            SingleTreeCommunicator(const SingleTreeCommunicator &other);
            virtual ~SingleTreeCommunicator();
            int num_level(void) const;
            int root_level(void) const;
            int level_rank(int level) const;
            int level_size(int level) const;
            void send_sample(int level, const struct geopm_sample_message_s &sample);
            void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy);
            void get_sample(int level, std::vector<struct geopm_sample_message_s> &sample);
            void get_policy(int level, struct geopm_policy_message_s &policy);
            size_t overhead_send(void);
        protected:
            GlobalPolicy *m_policy;
            struct geopm_sample_message_s m_sample;
    };

}

#endif

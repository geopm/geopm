/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef CONTROLLER_HPP_INCLUDE
#define CONTROLLER_HPP_INCLUDE

#include <vector>
#include <string>
#include <mpi.h>

#include "TreeCommunicator.hpp"
#include "PlatformFactory.hpp"
#include "DeciderFactory.hpp"
#include "Region.hpp"
#include "GlobalPolicy.hpp"
#include "Profile.hpp"
#include "geopm_time.h"
#include "geopm_message.h"


namespace geopm
{
    /// @brief Class used to lauch or step the global energy
    ///        optimization power management algorithm.
    ///
    /// The Controller class enables several methods of control:
    /// explicit stepping of the control algorithm by the application
    /// under control, running the control algorithm as a distinct
    /// processes from the application under control, running the
    /// control algorithm as a separate pthread owned by the
    /// application process under control, or the application under
    /// control can spawn a new MPI communicator that runs the control
    /// algorithm in conjunction with its own distinct MPI_COMM_WORLD.
    /// Each of these methods has different requirements and trade
    /// offs.
    ///
    ///
    /// The Controller is the central execution class for the geopm
    /// runtime, and interacts with many other geopm classes and
    /// coordinates their actions.  The controller gathers policy data
    /// from the GlobalPolicy, application profile data from the
    /// Profile (through shared memory), and hardware profile
    /// information from the Platform.  It uses the TreeCommunicator
    /// class to communicate policy and sample data between compute
    /// nodes.
    ///
    /// The Controller class is the C++ implementation of the
    /// geopm_ctl_c interface.  The class methods support the C
    /// interface defined for use with the geopm_ctl_c structure and
    /// are named accordingly.  The geopm_ctl_c structure is an
    /// opaque reference to the Controller class.

    class Controller
    {
        public:
            /// @brief Controller constructor.
            ///
            /// The Controller construction requires a reference to a
            /// GlobalPolicy object, the shared memory key used to
            /// communicate with the application, and the MPI
            /// communicator that the controller is running on.
            ///
            /// @param [in] global_policy A reference to the
            ///        GlobalPolicy object used to configure policy
            ///        for the entire allocation.
            ///
            /// @param [in] shmem_base Shared memory key base string
            ///        to which the rank ID is appended to denote the
            ///        shared memory key that each computational
            ///        application rank will write to.
            ///
            /// @param [in] comm The MPI communicator that supports
            ///        the control messages.
            Controller(const GlobalPolicy *global_policy, const std::string &shmem_base, MPI_Comm comm);
            /// @brief Controller destructor, virtual.
            virtual ~Controller();
            /// @brief Run control algorighm.
            ///
            /// Steps the control algorithm continuously until the
            /// shutdown signal is received.  Since this is a blocking
            /// call that never returns, it is intended that profiling
            /// information is provided through POSIX shared memory.
            void run(void);
            /// @brief Execute one step in the hierarchical control
            ///        algorithm.
            ///
            /// The call to step() is a blocking call for those
            /// processes that are the lowest MPI rank on the compute
            /// node (based on the communicator that was used to
            /// construct the Controller) and a no-op for those that
            /// are not.  A call to this method sends samples up the
            /// tree, then policies down the tree, and finally if the
            /// policy has been changed for the node of the calling
            /// process then the policy is enforced by writing MSR
            /// values.
            void step(void);
            void pthread(const pthread_attr_t *attr, pthread_t *thread);
            void spawn(void);
            int num_level(void) const;
            void leaf_decider(const LeafDecider *leaf_decider);
            void tree_decider(int level, const TreeDecider *tree_decider);
            void process_samples(const int level, const std::vector<struct geopm_sample_message_s> &sample);
            void enforce_child_policy(const int region_id, const int level, const Policy &policy);
        protected:
            int walk_down(void);
            int walk_up(void);
            bool m_is_node_root;
            int m_max_fanout;
            struct geopm_time_s m_time_zero;
            std::vector<int> m_fan_out;
            const GlobalPolicy *m_global_policy;
            TreeCommunicator *m_tree_comm;
            std::vector<Decider *> m_tree_decider;
            Decider *m_leaf_decider;
            DeciderFactory *m_decider_factory;
            PlatformFactory *m_platform_factory;
            Platform *m_platform;
            ProfileSampler *m_sampler;
            // Per level vector of maps from region identifier to region object
            std::vector<std::map <long, Region *> > m_region;
    };
}

#endif

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

#ifndef CONTROLLER_HPP_INCLUDE
#define CONTROLLER_HPP_INCLUDE

#include <vector>
#include <string>
#include <stack>

#include "SampleRegulator.hpp"
#include "TreeCommunicator.hpp"
#include "PlatformFactory.hpp"
#include "DeciderFactory.hpp"
#include "Region.hpp"
#include "GlobalPolicy.hpp"
#include "Profile.hpp"
#include "Tracer.hpp"
#include "geopm_time.h"
#include "geopm_plugin.h"
#include "Comm.hpp"

namespace geopm
{
    /// @brief Class used to launch or step the global extensible
    ///        open power manager algorithm.
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
            /// @param [in] comm The MPI communicator that supports
            ///        the control messages.
            Controller(IGlobalPolicy *global_policy, IComm *comm);
            /// @brief Controller destructor, virtual.
            virtual ~Controller();
            /// @brief Run control algorithm.
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
            /// @brief Run control algorithm as a separate thread.
            ///
            /// Creates a POSIX thread running the control algorithm
            /// continuously until the shutdown signal is received.
            /// This is intended to be called by the application under
            /// control.  With this method of launch the supporting
            /// MPI implementation must be enabled for
            /// MPI_THREAD_MULTIPLE using MPI_Init_thread().
            ///
            /// @param [in] attr The POSIX thread attributes applied
            ///        when thread is created.
            ///
            /// @param [out] thread Pointer to the POSIX thread that
            ///        is created.
            void pthread(const pthread_attr_t *attr, pthread_t *thread);
            /// @brief Run control algorithm on a separate MPI
            ///        communicator.
            ///
            /// Uses MPI_Comm_spawn() to spawn a new communicator with
            /// one process per compute node.  The geopmctl
            /// application will be spawned on this communicator which
            /// will communicate with the primary compute application
            /// through POSIX shared memory.
            void spawn(void);
            /// @brief Number of levels in the control hierarchy.
            ///
            /// Returns the depth of the balanced tree used for
            /// hierarchical control.  The tree always has a root and
            /// at least one set of children, so the minimum number of
            /// levels is two.
            ///
            ///  @return Number of hierarchy levels.
            int num_level(void) const;
            void generate_report(void);
            /// @brief Reset system to initial state.
            ///
            /// This will remove the shared memory keys and will reset
            /// all MSR values that GEO can alter to the value that was
            /// read at GEO startup.
            void reset(void);
        protected:
            enum m_controller_const_e {
                M_MAX_FAN_OUT = 16,
                M_SHMEM_REGION_SIZE = 12288,
            };
            void signal_handler(void);
            void check_signal(void);
            void connect(void);
            void walk_down(void);
            void walk_up(void);
            void override_telemetry(double progress);
            void update_region(void);
            void update_epoch(std::vector<struct geopm_telemetry_message_s> &telemetry);
            bool m_is_node_root;
            int m_max_fanout;
            std::vector<int> m_fan_out;
            const IGlobalPolicy *m_global_policy;
            ITreeCommunicator *m_tree_comm;
            std::vector<IDecider *> m_decider;
            DeciderFactory *m_decider_factory;
            PlatformFactory *m_platform_factory;
            Platform *m_platform;
            IProfileSampler *m_sampler;
            ISampleRegulator *m_sample_regulator;
            ITracer *m_tracer;
            std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > m_prof_sample;
            std::vector<struct geopm_msr_message_s> m_msr_sample;
            std::vector<struct geopm_telemetry_message_s> m_telemetry_sample;
            // Per level vector of maps from region identifier to region object
            std::vector<std::map <uint64_t, IRegion *> > m_region;
            std::vector<IPolicy *> m_policy;
            std::vector<struct geopm_policy_message_s> m_last_policy_msg;
            std::vector<struct geopm_sample_message_s> m_last_sample_msg;
            std::vector<uint64_t> m_region_id;
            // Per rank vector counting number of entries into MPI.
            std::vector<uint64_t> m_num_mpi_enter;
            std::vector<bool> m_is_epoch_changed;
            uint64_t m_region_id_all;
            bool m_do_shutdown;
            bool m_is_connected;
            int m_update_per_sample;
            int m_sample_per_control;
            int m_control_count;
            int m_rank_per_node;
            double m_epoch_time;
            double m_mpi_sync_time;
            double m_mpi_agg_time;
            double m_hint_ignore_time;
            double m_ignore_agg_time;
            uint64_t m_sample_count;
            uint64_t m_throttle_count;
            double m_throttle_limit_mhz;
            // Per rank vector tracking time of last entry into MPI.
            std::vector<struct geopm_time_s> m_mpi_enter_time;
            struct geopm_time_s m_app_start_time;
            double m_counter_energy_start;
            IComm *m_ppn1_comm;
            int m_ppn1_rank;
    };
}

#endif

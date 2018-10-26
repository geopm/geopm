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

#ifndef PROFILE_HPP_INCLUDE
#define PROFILE_HPP_INCLUDE

#include <stdint.h>
#include <vector>
#include <string>
#include <list>
#include <memory>

namespace geopm
{
    class Comm;
    class ISharedMemoryUser;
    class IControlMessage;
    class IPlatformTopo;
    class IProfileTable;
    class IProfileThreadTable;
    class ISampleScheduler;

    /// @brief Enables application profiling and application feedback
    ///        to the control algorithm.
    ///
    /// The information gathered by the Profile class identifies
    /// regions of code, progress within regions, and global
    /// synchronization points in the application.  Regions of code
    /// define periods in the application during which control
    /// parameters are tuned with the expectation that control
    /// parameters for a region can be optimized independently of
    /// other regions.  In this way a region is associated with a set
    /// of control parameters which can be optimized, and future time
    /// intervals associated with the same region will benefit from
    /// the application of control parameters which were determined
    /// from tuning within previous occurrences of the region.  There
    /// are two competing motivations for defining a region within the
    /// application.  The first is to identify a section of code that
    /// has distinct compute, memory or network characteristics.  The
    /// second is to avoid defining these regions such that they are
    /// nested within each other, as nested regions are ignored, and
    /// only the outer most region is used for tuning when nesting
    /// occurs.  Identifying progress within a region can be used to
    /// alleviate load imbalance in the application under the
    /// assumption that the region is bulk synchronous.  Under the
    /// assumption that the application employs an iterative algorithm
    /// which synchronizes periodically the user can alleviate load
    /// imbalance on larger time scales than the regions provide.
    /// This is done by marking the end of the outer most loop, or the
    /// "epoch."
    ///
    /// The Profile class is the C++ implementation of the
    /// computational application side interface to the GEOPM
    /// profiler.  The class methods support the C interface defined
    /// for use with the geopm_prof_c structure and are named
    /// accordingly.  The geopm_prof_c structure is an opaque
    /// reference to the Profile class.
    class IProfile
    {
        public:
            IProfile() = default;
            IProfile(const IProfile &other) = default;
            virtual ~IProfile() = default;
            /// @brief Register a region of code to be profiled.
            ///
            /// The statistics gathered for each region are aggregated
            /// in the final report, and the power policy will be
            /// determined distinctly for each region.  The
            /// registration of a region is idempotent, and the first
            /// call will have more overhead than subsequent
            /// attempts to re-register the same region.
            ///
            /// @param [in] region_name Unique name that identifies
            ///        the region being profiled.  This name will be
            ///        printed next to the region statistics in the
            ///        report.
            ///
            /// @param [in] hint Value from the
            ///        #geopm_hint_e structure which is used to
            ///        derive a starting policy before the application
            ///        has been profiled.
            ///
            /// @return Returns the region_id which is a unique
            ///         identifier derived from the region_name.  This
            ///         value is passed to Profile::enter(),
            ///         Profile::exit(), Profile::progress and
            ///         Profile::sample() to associate these calls with
            ///         the registered region.
            virtual uint64_t region(const std::string region_name, long hint) = 0;
            /// @brief Mark a region entry point.
            ///
            /// Called to denote the beginning of region of code that
            /// was assigned the region_id when it was registered.
            /// Nesting of regions is not supported: calls to this
            /// method from within a region previously entered but not
            /// yet exited are silently ignored.
            ///
            /// @param [in] region_id The identifier returned by
            ///        Profile::region() when the region was
            ///        registered.
            virtual void enter(uint64_t region_id) = 0;
            /// @brief Mark a region exit point.
            ///
            /// Called to denote the end of a region of code that was
            /// assigned the region_id when it was registered.
            /// Nesting of regions is not supported: calls to this
            /// method that are not exiting from the oldest unclosed
            /// entry point with the same region_id are silently
            /// ignored.
            ///
            /// @param [in] region_id The identifier returned by
            ///        Profile::region() when the region was
            ///        registered.
            virtual void exit(uint64_t region_id) = 0;
            /// @brief Signal fractional progress through a region.
            ///
            /// Signals the fractional amount of work completed within
            /// the phase.  This normalized progress reporting is used
            /// to identify processes that are closer or further away
            /// from completion, and resources can be shifted to those
            /// processes which are further behind.  Calls to this
            /// method from within a nested region are ignored.
            ///
            /// @param [in] region_id The identifier returned by
            ///        Profile::region() when the region was
            ///        registered.
            ///
            /// @param [in] fraction The fractional progress
            ///        normalized to be between 0.0 and 1.0 (zero on
            ///        entry one on completion).
            virtual void progress(uint64_t region_id, double fraction) = 0;
            /// @brief Signal pass through outer loop.
            ///
            /// Called once for each pass through the outer most
            /// computational loop executed by the application.  This
            /// function call should occur exactly once in the
            /// application source at the beginning of the loop that
            /// encapsulates the primary computational region of the
            /// application.
            virtual void epoch(void) = 0;
            virtual void shutdown(void) = 0;
            virtual std::shared_ptr<IProfileThreadTable> tprof_table(void) = 0;
    };

    class Profile : public IProfile
    {
        public:
            /// @brief Profile constructor.
            ///
            /// The Profile object is used by the application to
            /// instrument regions of code and post profile
            /// information to a shared memory region to be read by
            /// the geopm::Controller process.
            ///
            /// @param [in] prof_name Name associated with the
            ///        profile.  This name will be printed in the
            ///        header of the report.
            ///
            /// @param [in] comm The application's MPI communicator.
            ///        Each rank of this communicator will report to a
            ///        separate shared memory region.  One
            ///        geopm::Controller on each compute node will
            ///        consume the output from each rank running on
            ///        the compute node.
            Profile(const std::string &prof_name, std::unique_ptr<Comm> comm);
            /// @brief Profile testable constructor.
            ///
            /// @param [in] prof_name Name associated with the
            ///        profile.  This name will be printed in the
            ///        header of the report.
            ///
            /// @param [in] key_base Shmem key prefix.
            ///
            /// @param [in] comm The application's MPI communicator.
            ///        Each rank of this communicator will report to a
            ///        separate shared memory region.  One
            ///        geopm::Controller on each compute node will
            ///        consume the output from each rank running on
            ///        the compute node.
            ///
            /// @param [in] ctl_msg Preconstructed ControlMessage instance,
            ///        bypasses shmem creation.
            ///
            /// @param [in] topo Preconstructed PlatformTopo instance,
            ///        bypasses singleton discovery.
            ///
            /// @param [in] ctl_msg Preconstructed ProfileTable instance,
            ///        bypasses shmem creation.
            ///
            /// @param [in] ctl_msg Preconstructed ProfileThreadTable instance,
            ///        bypasses shmem creation.
            ///
            /// @param [in] ctl_msg Preconstructed SampleScheduler instance.
            Profile(const std::string &prof_name, const std::string &key_base, std::unique_ptr<Comm> comm,
                    std::unique_ptr<IControlMessage> ctl_msg, IPlatformTopo &topo, std::unique_ptr<IProfileTable> table,
                    std::shared_ptr<IProfileThreadTable> t_table, std::unique_ptr<ISampleScheduler> scheduler);
            /// @brief Test constructor.
            Profile(const std::string &prof_name, const std::string &key_base, std::unique_ptr<Comm> comm,
                    std::unique_ptr<IControlMessage> ctl_msg, IPlatformTopo &topo, std::unique_ptr<IProfileTable> table,
                    std::shared_ptr<IProfileThreadTable> t_table, std::unique_ptr<ISampleScheduler> scheduler,
                    std::shared_ptr<Comm> reduce_comm);
            /// @brief Profile destructor, virtual.
            virtual ~Profile();
            uint64_t region(const std::string region_name, long hint) override;
            void enter(uint64_t region_id) override;
            void exit(uint64_t region_id) override;
            void progress(uint64_t region_id, double fraction) override;
            void epoch(void) override;
            void shutdown(void) override;
            std::shared_ptr<IProfileThreadTable> tprof_table(void) override;
            void init_prof_comm(std::unique_ptr<Comm> comm, int &shm_num_rank);
            void init_ctl_msg(const std::string &sample_key);
            /// @brief Fill in rank affinity list.
            ///
            /// Uses PlatformTopo to determine the cpuset the current
            /// process is bound to. This information is used to fill
            /// in a set containing all CPUs we can run on. This is used
            /// to communicate with the geopm runtime the number of ranks
            /// as well as their affinity masks.
            void init_cpu_list(int num_cpu);
            void init_cpu_affinity(int shm_num_rank);
            void init_tprof_table(const std::string &tprof_key, IPlatformTopo &topo);
            void init_table(const std::string &sample_key);
        private:
            enum m_profile_const_e {
                M_PROF_SAMPLE_PERIOD = 1,
            };

            /// @brief Post profile sample.
            ///
            /// Called to derive a sample based on the profiling
            /// information collected.  This sample is posted to the
            /// geopm::Controller through shared memory.
            void sample(void);
            /// @brief Print profile report to a file.
            ///
            /// Writes a profile report to a file with the given
            /// file_name.  This should be called only after all
            /// profile data has been collected, just prior to
            /// application termination.
            ///
            /// @param [in] file_name The base file name for the
            ///        output report.  There may be suffixes appended
            ///        to this name if multiple files are created.
            ///
            void print(const std::string file_name);
            bool m_is_enabled;
            /// @brief holds the string name of the profile.
            std::string m_prof_name;
            /// @brief Holds the 64 bit unique region identifier
            ///        for the current region.
            uint64_t m_curr_region_id;
            /// @brief Holds the number of ranks that enter a region in
            ///        order to keep track of nested regions.
            int m_num_enter;
            /// @brief Holds the rank's current progress in the region.
            double m_progress;
            /// @brief Attaches to the shared memory region for
            ///        control messages.
            std::unique_ptr<ISharedMemoryUser> m_ctl_shmem;
            /// @brief Holds a pointer to the shared memory region
            ///        used to pass control messages to and from the geopm
            ///        runtime.
            std::unique_ptr<IControlMessage> m_ctl_msg;
            /// @brief Attaches to the shared memory region for
            ///        passing samples to the geopm runtime.
            std::unique_ptr<ISharedMemoryUser> m_table_shmem;
            /// @brief Hash table for sample messages contained in
            ///        shared memory.
            std::unique_ptr<IProfileTable> m_table;
            std::unique_ptr<ISharedMemoryUser> m_tprof_shmem;
            std::shared_ptr<IProfileThreadTable> m_tprof_table;
            std::unique_ptr<ISampleScheduler> m_scheduler;
            /// @brief Holds a list of cpus that the rank process is
            ///        bound to.
            std::list<int> m_cpu_list;
            /// @brief Communicator consisting of the root rank on each
            ///        compute node.
            std::shared_ptr<Comm> m_shm_comm;
            /// @brief The process's rank in MPI_COMM_WORLD.
            int m_rank;
            /// @brief The process's rank in m_shm_comm.
            int m_shm_rank;
            uint64_t m_parent_region;
            double m_parent_progress;
            int m_parent_num_enter;
            std::shared_ptr<Comm> m_reduce_comm;
            double m_overhead_time;
            double m_overhead_time_startup;
            double m_overhead_time_shutdown;
    };
}

#endif

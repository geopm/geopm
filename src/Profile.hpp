/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <memory>
#include <stack>

namespace geopm
{
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
    /// has distinct compute, memory, or network characteristics.  The
    /// second is to avoid defining these regions such that they are
    /// nested within each other, as nested regions are ignored and
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
    class Profile
    {
        public:
            Profile() = default;
            Profile(const Profile &other) = default;
            virtual ~Profile() = default;
            static Profile &default_profile(void);

            /// @brief Explicitly connect to the controller.
            virtual void init(void) = 0;

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
            ///         value is passed to Profile::enter() and
            ///         Profile::exit() to associate these calls with
            ///         the registered region.
            virtual uint64_t region(const std::string &region_name, long hint) = 0;
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
            /// @brief Update the total work for all CPUs. This method
            ///        should be called by one thread in the same
            ///        parallel region with the total work units
            ///        expected to be completed by the entire group.
            ///
            /// @param [in] num_work_unit The total work units for all
            ///        threads in the same parallel region.
            virtual void thread_init(uint32_t num_work_unit) = 0;
            /// @brief Mark one unit of work completed by the thread
            ///        on this CPU.
            //
            /// @param [in] cpu The Linux logical CPU obtained with
            ///        get_cpu().
            virtual void thread_post(int cpu) = 0;

            virtual void enable_pmpi(void) = 0;

            /// @brief Returns the Linux logical CPU index that the
            ///        calling thread is executing on, and caches the
            ///        result to be used in future calls.  This method
            ///        should be used by callers to report the correct
            ///        cpu to thread_init() and thread_post().
            static int get_cpu(void);
    };

    class Comm;
    class SharedMemory;
    class ControlMessage;
    class ProfileTable;
    class ApplicationRecordLog;
    class ApplicationStatus;

    class ProfileImp : public Profile
    {
        public:
            /// @brief ProfileImp constructor.
            ///
            /// The ProfileImp object is used by the application to
            /// instrument regions of code and post profile
            /// information to a shared memory region to be read by
            /// the geopm::Controller process.
            ProfileImp();
            /// @brief ProfileImp testable constructor.
            ///
            /// @param [in] prof_name Name associated with the
            ///        profile.  This name will be printed in the
            ///        header of the report.
            ///
            /// @param [in] key_base Shmem key prefix.
            ///
            /// @param [in] report Report file name.
            ///
            /// @param [in] timeout Application connection timeout.
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
            /// @param [in] num_cpu Number of CPUs for the platform
            ///
            /// @param [in] cpu_set Set of CPUs assigned to the
            ///        process owning the Profile object
            ///
            /// @param [in] table Preconstructed ProfileTable instance,
            ///        bypasses shmem creation.
            ProfileImp(const std::string &prof_name,
                       const std::string &key_base,
                       const std::string &report,
                       double timeout,
                       std::shared_ptr<Comm> comm,
                       std::shared_ptr<ControlMessage> ctl_msg,
                       int num_cpu,
                       std::set<int> cpu_set,
                       std::shared_ptr<ProfileTable> table,
                       std::shared_ptr<Comm> reduce_comm,
                       std::shared_ptr<ApplicationStatus> app_status,
                       std::shared_ptr<ApplicationRecordLog> app_record_log);
            /// @brief ProfileImp destructor, virtual.
            virtual ~ProfileImp();
            void init(void) override;
            uint64_t region(const std::string &region_name, long hint) override;
            void enter(uint64_t region_id) override;
            void exit(uint64_t region_id) override;
            void epoch(void) override;
            void shutdown(void) override;
            void thread_init(uint32_t num_work_unit) override;
            void thread_post(int cpu) override;
            virtual void enable_pmpi(void) override;
        protected:
            bool m_is_enabled;
        private:
            void init_prof_comm(std::shared_ptr<Comm> comm, int &shm_num_rank);
            void init_ctl_msg(const std::string &sample_key);
            /// @brief Fill in rank affinity list.
            ///
            /// Uses num_cpu to determine the cpuset the current
            /// process is bound to. This information is used to fill
            /// in a set containing all CPUs we can run on. This is used
            /// to communicate with the geopm runtime the number of ranks
            /// as well as their affinity masks.
            void init_cpu_set(int num_cpu);
            void init_cpu_affinity(int shm_num_rank);
            void init_table(const std::string &sample_key);
            void init_app_status(void);
            void init_app_record_log(void);
            /// @brief Set the hint on all CPUs assigned to this process.
            void set_hint(uint64_t hint);

            enum m_profile_const_e {
                M_PROF_SAMPLE_PERIOD = 1,
            };

            /// @brief Sends the report name and region names across
            ///        to Controller.
            void send_names(const std::string &report_file_name);

            /// @brief holds the string name of the profile.
            std::string m_prof_name;
            std::string m_key_base;
            std::string m_report;
            double m_timeout;
            std::shared_ptr<Comm> m_comm;
            /// @brief Holds the 64 bit unique region identifier
            ///        for the current region.
            uint64_t m_curr_region_id;
            uint64_t m_current_hash;
            /// @brief Attaches to the shared memory region for
            ///        control messages.
            std::unique_ptr<SharedMemory> m_ctl_shmem;
            /// @brief Holds a pointer to the shared memory region
            ///        used to pass control messages to and from the geopm
            ///        runtime.
            std::shared_ptr<ControlMessage> m_ctl_msg;
            int m_num_cpu;
            /// @brief Holds the set of cpus that the rank process is
            ///        bound to.
            std::set<int> m_cpu_set;
            /// @brief Attaches to the shared memory region for
            ///        passing samples to the geopm runtime.
            std::unique_ptr<SharedMemory> m_table_shmem;
            /// @brief Hash table for sample messages contained in
            ///        shared memory.
            std::shared_ptr<ProfileTable> m_table;
            /// @brief Communicator consisting of the root rank on each
            ///        compute node.
            std::shared_ptr<Comm> m_shm_comm;
            /// @brief The process's rank in MPI_COMM_WORLD.
            int m_process;
            /// @brief The process's rank in m_shm_comm.
            int m_shm_rank;
            std::shared_ptr<Comm> m_reduce_comm;

            std::shared_ptr<ApplicationStatus> m_app_status;
            std::shared_ptr<ApplicationRecordLog> m_app_record_log;
            std::stack<uint64_t> m_hint_stack;

            double m_overhead_time;
            double m_overhead_time_startup;
            double m_overhead_time_shutdown;

    };
}

#endif

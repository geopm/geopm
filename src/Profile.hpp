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

#ifndef PROFILE_HPP_INCLUDE
#define PROFILE_HPP_INCLUDE

#include <stdint.h>
#include <string>
#include <list>
#include <forward_list>
#include <fstream>
#include <mpi.h>

#include "geopm_time.h"
#include "geopm_message.h"
#include "SharedMemory.hpp"
#include "ProfileTable.hpp"
#include "SampleScheduler.hpp"
#include "ProfileThread.hpp"

/// @brief Enum encompassing application and
/// geopm runtime state.
enum geopm_status_e {
    GEOPM_STATUS_UNDEFINED = 0,
    GEOPM_STATUS_MAP_BEGIN = 1,
    GEOPM_STATUS_MAP_END = 2,
    GEOPM_STATUS_SAMPLE_BEGIN = 3,
    GEOPM_STATUS_SAMPLE_END = 4,
    GEOPM_STATUS_NAME_BEGIN = 5,
    GEOPM_STATUS_NAME_LOOP_BEGIN = 6,
    GEOPM_STATUS_NAME_LOOP_END = 7,
    GEOPM_STATUS_NAME_END = 8,
    GEOPM_STATUS_SHUTDOWN = 9,
};

enum geopm_profile_e {
    GEOPM_MAX_NUM_CPU = 768
};

/// @brief Structure intended to be shared between
/// the geopm runtime and the application in
/// order to convey status and control information.
struct geopm_ctl_message_s {
    /// @brief Status of the geopm runtime.
    volatile uint32_t ctl_status;
    /// @brief Status of the application.
    volatile uint32_t app_status;
    /// @brief Holds affinities of all application ranks
    /// on the local compute node.
    int cpu_rank[GEOPM_MAX_NUM_CPU];
};

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
            IProfile() {}
            IProfile(const IProfile &other) {}
            virtual ~IProfile() {}
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
    };

    class IProfileRankSampler
    {
        public:
            IProfileRankSampler() {}
            IProfileRankSampler(const IProfileRankSampler &other) {}
            virtual ~IProfileRankSampler() {}
            virtual void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length) = 0;
            virtual size_t capacity(void) = 0;
            virtual bool name_fill(std::set<std::string> &name_set) = 0;
            virtual void report_name(std::string &report_str) = 0;
            virtual void profile_name(std::string &prof_str) = 0;
    };

    class IProfileSampler
    {
        public:
            IProfileSampler() {}
            IProfileSampler(const IProfileSampler &other) {}
            virtual ~IProfileSampler() {}
            virtual size_t capacity(void) = 0;
            virtual void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, MPI_Comm comm) = 0;
            virtual bool do_shutdown(void) = 0;
            virtual bool do_report(void) = 0;
            virtual void region_names(void) = 0;
            virtual void initialize(int &rank_per_node) = 0;
            virtual void cpu_rank(std::vector<int> &cpu_rank) = 0;
            virtual void name_set(std::set<std::string> &region_name) = 0;
            virtual void report_name(std::string &report_str) = 0;
            virtual void profile_name(std::string &prof_str) = 0;
            virtual IProfileThreadTable *tprof_table(void) = 0;
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
            /// @param [in] table_size Size in bytes of shared memory
            ///        region that will be used for posting updates.
            ///        The geopm::Controller is responsible for
            ///        creating the shared memory region that the
            ///        Profile object attaches to.  The
            ///        geopm::Controller is the consumer of the posted
            ///        data that the Profile produces.
            ///
            /// @param [in] comm The application's MPI communicator.
            ///        Each rank of this communicator will report to a
            ///        separate shared memory region.  One
            ///        geopm::Controller on each compute node will
            ///        consume the output from each rank running on
            ///        the compute node.
            Profile(const std::string prof_name, MPI_Comm comm);
            /// @brief Profile destructor, virtual.
            virtual ~Profile();
            uint64_t region(const std::string region_name, long hint);
            void enter(uint64_t region_id);
            void exit(uint64_t region_id);
            void progress(uint64_t region_id, double fraction);
            void epoch(void);
            void shutdown(void);
            IProfileThreadTable *tprof_table(void);
        protected:
            enum m_profile_const_e {
                M_PROF_SAMPLE_PERIOD = 1,
            };
            /// @brief Fill in rank affinity list.
            ///
            /// Uses hwloc to determine the cpuset the current
            /// process is bound to. This information is used to fill
            /// in a set containing all CPUs we can run on. This is used
            /// to communicate with the geopm runtime the number of ranks
            /// as well as their affinity masks.
            void init_cpu_list(void);
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
            /// @param [in] verbosity Gives the verbosity level for
            ///        the report. If zero is given, no report is
            ///        generated.  In the future reports with higher
            ///        verbosity level will include more details about
            ///        the run.  Currently there is just one type of
            ///        report created.
            void print(const std::string file_name, int verbosity);
            bool m_is_enabled;
            /// @brief holds the string name of the profile.
            std::string m_prof_name;
            /// @brief Holds the 64 bit unique region identifier
            ///        for the current region.
            uint64_t m_curr_region_id;
            /// @brief Holds the number of ranks that enter a region in
            ///        order to keep track of nested regions.
            int m_num_enter;
            /// @brief Holds the count of progress reports in order to
            ///        create a sample when the count reaches some sample limit.
            int m_num_progress;
            /// @brief Holds the rank's current progress in the region.
            double m_progress;
            /// @brief Holds a pointer to the shared memory region
            ///        used for passing sample data to the geopm runtime.
            void *m_table_buffer;
            /// @brief Attaches to the shared memory region for
            ///        control messages.
            ISharedMemoryUser *m_ctl_shmem;
            /// @brief Holds a pointer to the shared memory region
            ///        used to pass control messages to and from the geopm
            ///        runtime.
            struct geopm_ctl_message_s *m_ctl_msg;
            /// @brief Attaches to the shared memory region for
            ///        passing samples to the geopm runtime.
            ISharedMemoryUser *m_table_shmem;
            /// @brief Hash table for sample messages contained in
            ///        shared memory.
            IProfileTable *m_table;
            ISharedMemoryUser *m_tprof_shmem;
            IProfileThreadTable *m_tprof_table;
            const double M_OVERHEAD_FRAC;
            ISampleScheduler *m_scheduler;
            /// @brief Holds a list of cpus that the rank process is
            ///        bound to.
            std::list<int> m_cpu_list;
            /// @brief Communicator consisting of the root rank on each
            ///        compute node.
            MPI_Comm m_shm_comm;
            /// @brief The process's rank in MPI_COMM_WORLD.
            int m_rank;
            /// @brief The process's rank in m_shm_comm.
            int m_shm_rank;
            /// @brief Tracks the first call to epoch.
            bool m_is_first_sync;
            uint64_t m_parent_region;
            double m_parent_progress;
            int m_parent_num_enter;
            double m_overhead_time;
            double m_overhead_time_startup;
            double m_overhead_time_shutdown;
    };

    /// @brief Retrieves sample data from a single application rank through
    ///        a shared memory interface.
    ///
    /// The ProfileRankSampler is the runtime side interface to the shared
    /// memory region for a single rank of the application. It can retrieve
    /// samples from the shared hash table for that rank.
    class ProfileRankSampler : public IProfileRankSampler
    {
        public:
            /// @brief ProfileRankSampler constructor.
            ///
            /// The ProfileRankSampler constructor takes in a unique shared
            /// memory key for the rank as well as the size of the hash table
            /// to be shared with the application rank. It creates the shared
            /// memory region and the hash table that the application will
            /// attach to.
            ///
            /// @param [in] shm_key Shared memory key unique to a
            ///        specific rank.
            ///
            /// @param [in] table_size Size of the hash table to create in
            ///        the shared memory region.
            ProfileRankSampler(const std::string shm_key, size_t table_size);
            /// @brief ProfileRankSampler destructor.
            ///
            /// Cleans up the hash table and shared memory region.
            virtual ~ProfileRankSampler();
            /// @brief Returns the samples present in the hash table.
            ///
            /// Fills in a portion of a vector specified by a vector iterator.
            /// It is assumed the vector is already sized greater than or
            /// equal to the maximum number of samples we can return. This value
            /// can be queried with the capacity() method. Internally the samples
            /// are aggregated for later reporting functionality.
            ///
            /// @param [in] content_begin Vector iterator at which to begin inserting
            ///        sample messages.
            ///
            /// @param [out] length The number of samples that were inserted.
            void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length);
            /// @brief Retrieve the maximum capacity of the hash table.
            ///
            /// @return The maximum number of samples that can possibly
            ///         be returned.
            size_t capacity(void);
            /// @brief Retrieve region names from the application process.
            ///
            /// Coordinates with the application process to retrieve the
            /// profile name, region names, and the file name to write
            /// the report to.
            ///
            /// @return Returns true if finished retrieving names from the
            ///         application, else returns false.
            bool name_fill(std::set<std::string> &name_set);
            void report_name(std::string &report_str);
            void profile_name(std::string &prof_str);
            IProfileThreadTable *tprof_table(void);
        protected:
            /// Holds the shared memory region used for sampling from the
            /// application process.
            ISharedMemory *m_table_shmem;
            /// The hash table which stores application process samples.
            IProfileTable *m_table;
            ISharedMemory *m_tprof_shmem;
            IProfileThreadTable *m_tprof_table;
            /// Holds the initial state of the last region entered.
            struct geopm_prof_message_s m_region_entry;
            /// Holds the initial state of the last region entered.
            struct geopm_prof_message_s m_epoch_entry;
            /// Holds the profile name string.
            std::string m_prof_name;
            /// Holds the file name for the post-process report.
            std::string m_report_name;
            /// Holds the set of region string names.
            std::set<std::string> m_name_set;
            /// Holds the status of the name_fill operation.
            bool m_is_name_finished;
    };

    /// @brief Retrieves sample data from the set of application ranks on
    ///        a single node.
    ///
    /// The ProfileSampler class is the geopm runtime side interface to
    /// the GEOPM profiler. It retrieves samples from all application ranks
    /// on a single compute node. It is also the interface to the shared
    /// memory region used to coordinate between the geopm runtime and
    /// the MPI application.
    class ProfileSampler : public IProfileSampler
    {
        public:
            /// @brief ProfileSampler constructor.
            ///
            /// Constructs a shared memory region for coordination between
            /// the geopm runtime and the MPI application.
            ///
            /// @param [in] table_size The size of the hash table that will
            ///        be created for each application rank.
            ProfileSampler(size_t table_size);
            /// @brief ProfileSampler destructor.
            virtual ~ProfileSampler();
            /// @brief Retrieve the maximum capacity of all the per-rank
            ///        hash tables.
            ///
            /// @return The maximum number of samples that can possibly
            ///         be returned.
            size_t capacity(void);
            /// @brief Returns the samples present in all the per-rank
            ///        hash tables.
            ///
            /// Fills in a portion of a vector which is assumed to be already
            /// sized greater than or equal to the maximum number of samples
            /// we can return. This value can be queried with the capacity()
            /// method.
            ///
            /// @param [in] content Vector to be filled with per-node
            ///        sample messages.
            ///
            /// @param [out] length The number of samples that were inserted.
            void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, MPI_Comm comm);
            /// @brief Check if the application is shutting down.
            ///
            /// Queries the control shared memory region to test if the
            /// application status in shutdown.
            ///
            /// @return Return true if application is shutting down, else
            ///         returns false.
            bool do_shutdown(void);
            /// @brief Generate a post-run report for a single node.
            ///
            /// Generates a post-run report by telling each ProfileRankSampler
            /// to dump its per-region statistics to a file descriptor.
            bool do_report(void);
            void region_names(void);
            /// @brief Initialize shared memory regions.
            ///
            /// Coordinates with the application to initialize shared memory
            /// and create ProfileRankSamplers for each MPI application rank
            /// running on the local compute node.
            ///
            /// @param [out] rank_per_node number of mpi ranks
            /// running on the node.
            void initialize(int &rank_per_node);
            /// @brief Retrieve a vector to the affinities of all
            ///        application ranks.
            ///
            /// Resizes the input vector cpu_rank to number of Linux
            /// online CPUs in the system.  Each element of the vector
            /// is indexed by the Linux CPU ID, and the value assigned
            /// is the MPI rank running on the CPU (or -1 if no rank
            /// has been affinitized).
            ///
            /// @param [out] cpu_rank Vector to be filled with the MPI
            ///        rank for each Linux CPU, set to -1 if no MPI
            ///        rank is affinitized.
            void cpu_rank(std::vector<int> &cpu_rank);
            void name_set(std::set<std::string> &region_name);
            void report_name(std::string &report_str);
            void profile_name(std::string &prof_str);
            IProfileThreadTable *tprof_table(void);
        protected:
            /// Holds the shared memory region used for application coordination
            /// and control.
            ISharedMemory *m_ctl_shmem;
            /// Pointer to the control structure used for application coordination
            /// and control.
            struct geopm_ctl_message_s *m_ctl_msg;
            /// List of per-rank samplers for each MPI application rank running
            /// on the local compute node.
            std::forward_list<IProfileRankSampler *> m_rank_sampler;
            /// Size of the hash tables to create for each MPI application rank
            /// running on the local compute node..
            const size_t m_table_size;
            std::set<std::string> m_name_set;
            std::string m_report_name;
            std::string m_profile_name;
            bool m_do_report;
            ISharedMemory *m_tprof_shmem;
            IProfileThreadTable *m_tprof_table;

    };
}

#endif

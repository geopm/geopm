/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROFILE_HPP_INCLUDE
#define PROFILE_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <memory>
#include <stack>
#include <map>

#include "geopm_hash.h"
#include "geopm_hint.h"
#include "config.h"


/****************************************/
/* Encode/decode function for region_id */
/****************************************/

enum geopm_region_id_e {
    GEOPM_REGION_ID_EPOCH =        1ULL << 63, /* Signaling the start of an epoch, no associated Region */
    GEOPM_REGION_ID_MPI =          1ULL << 62, /* Execution of MPI calls */
};

static inline uint64_t geopm_region_id_hash(uint64_t region_id)
{
    uint64_t ret = ((region_id << 32) >> 32);

    if (GEOPM_REGION_HASH_INVALID == ret) {
        ret = GEOPM_REGION_HASH_UNMARKED;
    }
    return ret;
}

static inline int geopm_region_id_is_mpi(uint64_t region_id)
{
    return (region_id & GEOPM_REGION_ID_MPI) ? 1 : 0;
}

static inline uint64_t geopm_region_id_hint(uint64_t region_id)
{
    uint64_t ret;
    if (GEOPM_REGION_HASH_UNMARKED == region_id) {
        ret = GEOPM_REGION_HINT_UNKNOWN;
    }
    else if (geopm_region_id_is_mpi(region_id)) {
        ret = GEOPM_REGION_HINT_NETWORK;
    }
    else {
        ret = region_id >> 32;
        if (!ret || ret >= GEOPM_NUM_REGION_HINT) {
            ret = GEOPM_REGION_HINT_UNKNOWN;
        }
    }
    return ret;
}

static inline uint64_t geopm_region_id_set_hint(uint64_t hint_type, uint64_t region_id)
{
    return (region_id | (hint_type << 32));
}

static inline int geopm_region_id_hint_is_equal(uint64_t hint_type, uint64_t region_id)
{
    return (region_id & (hint_type << 32)) ? 1 : 0;
}


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
            Profile &operator=(const Profile &other) = default;
            virtual ~Profile() = default;
            static Profile &default_profile(void);

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

            virtual std::vector<std::string> region_names(void) = 0;

            /// @brief Returns the Linux logical CPU index that the
            ///        calling thread is executing on, and caches the
            ///        result to be used in future calls.  This method
            ///        should be used by callers to report the correct
            ///        CPU to thread_init() and thread_post().
            static int get_cpu(void);
    };

    class SharedMemory;
    class ApplicationRecordLog;
    class ApplicationStatus;
    class ServiceProxy;

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
            /// @param [in] report Report file name.
            ///
            /// @param [in] num_cpu Number of CPUs for the platform
            ///
            /// @param [in] cpu_set Set of CPUs assigned to the
            ///        process owning the Profile object
            ///
            ProfileImp(const std::string &prof_name,
                       const std::string &report,
                       int num_cpu,
                       std::set<int> cpu_set,
                       std::shared_ptr<ApplicationStatus> app_status,
                       std::shared_ptr<ApplicationRecordLog> app_record_log,
                       bool do_profile,
                       std::shared_ptr<ServiceProxy> service_proxy);
            /// @brief ProfileImp destructor, virtual.
            virtual ~ProfileImp();
            uint64_t region(const std::string &region_name, long hint) override;
            void enter(uint64_t region_id) override;
            void exit(uint64_t region_id) override;
            void epoch(void) override;
            void shutdown(void) override;
            void thread_init(uint32_t num_work_unit) override;
            void thread_post(int cpu) override;
            std::vector<std::string> region_names(void) override;
        protected:
            bool m_is_enabled;
        private:
            void init_cpu_set(int num_cpu);
            void init_app_status(void);
            void init_app_record_log(void);
            /// @brief Set the hint on all CPUs assigned to this process.
            void set_hint(uint64_t hint);

            enum m_profile_const_e {
                M_PROF_SAMPLE_PERIOD = 1,
            };

            /// @brief holds the string name of the profile.
            std::string m_prof_name;
            std::string m_report;
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
            int m_num_cpu;
            /// @brief Holds the set of CPUs that the rank process is
            ///        bound to.
            std::set<int> m_cpu_set;

            std::shared_ptr<ApplicationStatus> m_app_status;
            std::shared_ptr<ApplicationRecordLog> m_app_record_log;
            std::stack<uint64_t> m_hint_stack;

            double m_overhead_time;
            double m_overhead_time_startup;
            double m_overhead_time_shutdown;
            bool m_do_profile;
            std::map<std::string, uint64_t> m_region_names;
#ifdef GEOPM_DEBUG
            /// @brief The list of known region identifiers.
            std::set<uint64_t> m_region_ids;
#endif
            std::shared_ptr<ServiceProxy> m_service_proxy;
    };
}

#endif

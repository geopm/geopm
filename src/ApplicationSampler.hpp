/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APPLICATIONSAMPLER_HPP_INCLUDE
#define APPLICATIONSAMPLER_HPP_INCLUDE

#include <cstdint>

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "geopm_time.h"

namespace geopm
{
    class ApplicationRecordLog;
    class ProfileSampler;
    struct record_s;
    struct short_region_s;

    class ApplicationSampler
    {
        public:
            /// @brief Singleton accessor for the application sampler.
            static ApplicationSampler &application_sampler(void);
            /// @brief Returns set of region hashes associated with
            ///        application network functions.
            /// @return Set of netowrk function region hashes.
            static std::set<uint64_t> region_hash_network(void);
            /// @brief Returns the default shared memory key, which consists
            ///        of the prefix "/geopm-shm-" concatenated with the euid.
            static std::string default_shmkey(void);
            /// @brief Set the reference time that will be used for
            ///        all future record time reporting.
            /// @param [in] start_time The reference zero time.
            virtual void time_zero(const geopm_time_s &start_time) = 0;
            /// @brief Update the record buffer by clearing out old
            ///        records and providing a new cache for
            ///        subsequent calls to the get_records()
            ///        method. Also update cache of application
            ///        status for use by hint and progress APIs.
            virtual void update(const geopm_time_s &curr_time) = 0;
            /// @brief Get all of the application events that have
            ///        been recorded since the last call to
            ///        update_records().
            /// @return Vector of application event records.
            virtual std::vector<record_s> get_records(void) const = 0;
            virtual short_region_s get_short_region(uint64_t event_signal) const = 0;
            /// @brief Called after observing a EVENT_NAME_KEY event
            ///        to get a map from any hash returned in a previous
            ///        record to the string that generated the hash.
            ///        The result includes the names of all entered
            ///        regions and the profile name.
            /// @param [in] name_key The signal from the name key
            ///        event record.
            /// @return A map from record-provided hash value to the
            ///         string the record refers to.
            virtual std::map<uint64_t, std::string> get_name_map(uint64_t name_key) const = 0;
            /// @brief Get the region hash associated with a CPU.
            ///
            /// Returns the most recently sampled value for the region
            /// hash associated with the Linux logical CPU specified
            /// by the user.  An exception is raised if the value of
            /// cpu_idx is negative or greater or equal to
            /// platform_topo().num_domain(GEOPM_DOMAIN_CPU).
            ///
            /// @param [in] cpu_idx The index of the linux logical CPU
            ///        to query.
            /// @return The region hash associated with the CPU.
            virtual uint64_t cpu_region_hash(int cpu_idx) const = 0;
            /// @brief Get the hint associated with a CPU.
            ///
            /// Returns the most recently sampled value for the hint
            /// associated with the Linux logical CPU specified by the
            /// user.  An exception is raised if the value of cpu_idx
            /// is negative or greater or equal to
            /// platform_topo().num_domain(GEOPM_DOMAIN_CPU).
            ///
            /// @param [in] cpu_idx The index of the linux logical CPU
            ///        to query.
            /// @return The hint associated with the CPU.
            virtual uint64_t cpu_hint(int cpu_idx) const = 0;
            /// @brief Get the amount of time a CPU has been measured
            ///        running with a hint.
            ///
            /// Returns a total amount of time in seconds that a CPU
            /// was measured to be running with the hint value on the
            /// Linux logical CPU specified by the user.  An exception
            /// is raised if the value of cpu_idx is negative or
            /// greater or equal to
            /// platform_topo().num_domain(GEOPM_DOMAIN_CPU) or if the
            /// specified hint is invalid.
            ///
            /// @param [in] cpu_idx The index of the linux logical CPU
            ///        to query.
            ///
            /// @return The total time in seconds since the
            ///         applications started.
            virtual double cpu_hint_time(int cpu_idx, uint64_t hint) const = 0;
            /// @brief Get the progress reported on a CPU.
            ///
            /// Returns the most recently sampled value for the
            /// fraction of the work units completed by the thread
            /// running on the specified CPU.  If the CPU queried is
            /// not currently executing a thread that is reporting
            /// progress, then the value NAN is returned.  An
            /// exception is raised if the value of cpu_idx is
            /// negative or greater or equal to
            /// platform_topo().num_domain(GEOPM_DOMAIN_CPU).
            ///
            /// @param [in] cpu_idx The index of the linux logical CPU
            ///        to query.
            ///
            /// @return Value between 0.0 and 1.0 representing the
            ///         fraction of work completed, or NAN.
            virtual double cpu_progress(int cpu_idx) const = 0;
            /// @brief Get the process ID for each linux logical CPU.
            ///
            /// A vector of length the number of CPUs on the system is
            /// returned.  If a CPU has been claimed by a process, the
            /// returned value in the vector corresponding to the CPU
            /// will be the process identifier.  If a CPU is unclaimed
            /// the value will be populated with -1.
            ///
            /// @return A vector of process identifiers indexed over
            ///         the GEOPM_DOMAIN_CPU.
            virtual std::vector<int> per_cpu_process(void) const = 0;
            /// @brief Connect with an application using a key
            ///
            /// Called by the Controller to set up all channels of
            /// communication with the application using the provided
            /// string as a key.  This string corresponds to the
            /// default shared memory key specified by the system,
            /// used by the application that is connected.
            /// The default shared memory key consists of the prefix
            /// "/geopm-shm-" concatenated with the euid.
            ///
            /// @param [in] shm_key String known to the
            ///        application that will be used to identify a
            ///        channel of communication.
            virtual void connect(const std::string &shm_key) = 0;

            // Deprecated API's below for access to legacy objects
            virtual void set_sampler(std::shared_ptr<ProfileSampler> sampler) = 0;
            virtual std::shared_ptr<ProfileSampler> get_sampler(void) = 0;
        protected:
            virtual ~ApplicationSampler() = default;
            ApplicationSampler() = default;
        private:
            static std::set<uint64_t> region_hash_network_once(void);
    };
}

#endif

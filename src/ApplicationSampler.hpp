/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
    class ProfileSampler;
    class ProfileIOSample;
    class EpochRuntimeRegulator;
    struct record_s;

    class ApplicationSampler
    {
        public:
            /// @brief Singleton accessor for the application sampler.
            static ApplicationSampler &application_sampler(void);
            /// @brief Returns set of region hashes associated with
            ///        application network functions.
            /// @return Set of netowrk function region hashes.
            static std::set<uint64_t> region_hash_network(void);
            /// @brief Set the reference time that will be used for
            ///        all future record time reporting.
            /// @param [in] start_time The reference zero time.
            virtual void time_zero(const geopm_time_s &start_time) = 0;
            /// @brief Update the record buffer by clearing out old
            ///        records and providing a new cache for
            ///        subsequent calls to the get_records() method.
            virtual void update_records(void) = 0;
            /// @brief Get all of the application events that have
            ///        been recorded since the last call to
            ///        update_records().
            /// @return Vector of application event records.
            virtual std::vector<record_s> get_records(void) const = 0;
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
            /// @brief Sample the current hint for every cpu.
            /// @return Vector over Linux logical CPU of the GEOPM
            ///         region hint currently being executed.
            virtual std::vector<uint64_t> per_cpu_hint(void) const = 0;
            /// @brief Sample the current progress for every cpu.
            /// @return Vector over Linux logical CPU of the region
            ///         progress.
            virtual std::vector<double> per_cpu_progress(void) const = 0;
            /// @brief Return a per-cpu vector of the process mapped
            ///        to each cpu.
            virtual std::vector<int> per_cpu_process(void) const = 0;

            // Deprecated API's below for access to legacy objects
            virtual void set_sampler(std::shared_ptr<ProfileSampler> sampler) = 0;
            virtual std::shared_ptr<ProfileSampler> get_sampler(void) = 0;
            virtual void set_regulator(std::shared_ptr<EpochRuntimeRegulator> regulator) = 0;
            virtual std::shared_ptr<EpochRuntimeRegulator> get_regulator(void) = 0;
            virtual void set_io_sample(std::shared_ptr<ProfileIOSample> io_sample) = 0;
            virtual std::shared_ptr<ProfileIOSample> get_io_sample(void) = 0;
        protected:
            virtual ~ApplicationSampler() = default;
            ApplicationSampler() = default;
    };
}

#endif

/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef PROFILEIOSAMPLE_HPP_INCLUDE
#define PROFILEIOSAMPLE_HPP_INCLUDE

#include <vector>

#include "geopm_time.h"

struct geopm_prof_message_s;

namespace geopm
{
    class ProfileIOSample
    {
        public:
            ProfileIOSample() {}
            virtual ~ProfileIOSample() {}
            virtual void finalize_unmarked_region() = 0;
            /// @brief Update internal state with a batch of samples from the
            ///        application.
            virtual void update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end) = 0;
            virtual void update_thread(const std::vector<double> &thread_progress) = 0;
            /// @brief Return the region ID that each CPU is running,
            ///        which is the region of the rank running on that
            ///        CPU.
            virtual std::vector<uint64_t> per_cpu_region_id(void) const = 0;
            /// @brief Return the current progress through the region
            ///        on each CPU.
            /// @param [in] extrapolation_time The timestamp to use to
            ///        estimate the current progress through the
            ///        region based on the previous two samples.
            virtual std::vector<double> per_cpu_progress(const struct geopm_time_s &extrapolation_time) const = 0;
            virtual std::vector<double> per_cpu_thread_progress(void) const = 0;
            /// @brief Return the last runtime of the given region
            ///        for the rank running on each CPU.
            /// @param [in] region_id Region ID for the region of interest.
            virtual std::vector<double> per_cpu_runtime(uint64_t region_id) const = 0;
            /// @brief Return the total time from the start of the
            ///        application until now.
            virtual double total_app_runtime(void) const = 0;
            /// @brief Returns the local-node rank running on each CPU.
            virtual std::vector<int> cpu_rank(void) const = 0;
    };
}

#endif

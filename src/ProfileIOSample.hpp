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

#ifndef PROFILEIOSAMPLE_HPP_INCLUDE
#define PROFILEIOSAMPLE_HPP_INCLUDE

#include <vector>
#include <map>

#include "geopm_message.h"

namespace geopm
{
    template <typename T> class CircularBuffer;

    class IProfileIOSample
    {
        public:
            IProfileIOSample() = default;
            virtual ~IProfileIOSample() = default;
            virtual void update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end) = 0;
            virtual std::vector<uint64_t> per_cpu_region_id(void) = 0;
            virtual std::vector<double> per_cpu_progress(const struct geopm_time_s &extrapolation_time) = 0;
    };

    class ProfileIOSample : public IProfileIOSample
    {
        public:
            ProfileIOSample(const std::vector<int> &cpu_rank);
            virtual ~ProfileIOSample();
            void update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end) override;
            std::vector<uint64_t> per_cpu_region_id(void) override;
            std::vector<double> per_cpu_progress(const struct geopm_time_s &extrapolation_time) override;
        private:
            struct m_rank_sample_s {
                struct geopm_time_s timestamp;
                double progress;
            };
           enum m_interp_type_e {
                M_INTERP_TYPE_NONE = 0,
                M_INTERP_TYPE_NEAREST = 1,
                M_INTERP_TYPE_LINEAR = 2,
            };
            std::vector<double> per_rank_progress(const struct geopm_time_s &extrapolation_time);
            /// @brief Number of ranks running on the node.
            size_t m_num_rank;
            /// @brief The rank index of the rank running on each CPU.
            std::vector<int> m_cpu_rank;
            /// @brief A map from the MPI rank reported in the
            ///        ProfileSampler data to the node local rank
            ///        index.
            std::map<int, int> m_rank_idx_map;
            /// @brief The region_id of each rank derived from the
            ///        stored ProfileSampler data used for
            ///        extrapolation.
            std::vector<uint64_t> m_region_id;
            /// @brief Per rank record of last profile samples in
            ///        m_region_id_prev
            std::vector<CircularBuffer<struct m_rank_sample_s> > m_rank_sample_buffer;
            /// @brief Vector to multiply with signal_domain_matrix to
            /// project into control domains
            std::vector<double> m_aligned_signal;
    };
}

#endif

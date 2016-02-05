/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#ifndef REGION_HPP_INCLUDE
#define REGION_HPP_INCLUDE

#include <stdint.h>
#include <string>
#include <stack>

#include "geopm_message.h"
#include "Observation.hpp"
#include "Policy.hpp"

namespace geopm
{
    /// @brief This class encapsulates all recorded data for a
    ///        specific application execution region.
    class Region
    {
        public:
            /// @brief The enumeration of statistics that are computed
            ///        by the stats() method.
            enum m_stat_type_e {
                /// The number of stored samples used in calculations.
                M_STAT_TYPE_NSAMPLE,
                /// The mean value of the stored samples.
                M_STAT_TYPE_MEAN,
                /// The median value of the stored samples.
                M_STAT_TYPE_MEDIAN,
                /// The standard deviation of the stored samples.
                M_STAT_TYPE_STDDEV,
                /// The minimum value among the stored samples.
                M_STAT_TYPE_MIN,
                /// The maximum value among the stored samples.
                M_STAT_TYPE_MAX,
                /// The number of statistics gathered by the stats()
                /// method, and the length of the result array.
                M_NUM_STAT_TYPE
            };
            enum m_const_e {
                M_NUM_SAMPLE_HISTORY = 8,
            };
            /// @brief Default constructor.
            /// @param [in] identifier Unique 64 bit region identifier.
            /// @param [in] hint geopm_policy_hint_e describing the compute
            ///             characteristics of this region
            /// @param [in] num_domain Number of control domains.
            Region(uint64_t identifier, int hint, int num_domain, int level);
            /// @brief Default destructor.
            virtual ~Region();
            void insert(std::stack<struct geopm_telemetry_message_s> &telemetry_stack);
            void insert(const std::vector<struct geopm_sample_message_s> &sample);
            /// @brief Retrieve the unique region identifier.
            /// @return 64 bit region identifier.
            uint64_t identifier(void) const;
            /// @brief Retrieve the compute characteristic hint for this region.
            /// @return geopm_policy_hint_e describing the compute characteristics
            /// of this region.
            int hint(void) const;
            /// @brief Return an aggregated sample to send up the tree.
            /// Called once this region has converged to send a sample
            /// up to the next level of the tree.
            /// @param [out] Sample message structure to fill in.
            void sample_message(struct geopm_sample_message_s &sample);
            /// Returns the latest value
            double signal(int domain_idx, int signal_type);
            /// @brief Retrieve the statistics for a domain of control.
            ///
            /// Get the statistics for a given domain of control and a
            /// signal type for the buffered data associated with the
            /// application region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @param [out] result A double array of length
            ///        M_NUM_STAT_TYPE which contains the computed
            ///        statistics as enumerated in m_stat_type_e.
            void statistics(int domain_idx, int signal_type, double result[]) const;
            /// @brief Integrate a signal over time.
            ///
            /// Computes the integral of the signal over the interval
            /// of time spanned by the samples stored in the region
            /// which where gathered since the applications most
            /// recent entry into the region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            double integrate_time(int domain_idx, int signal_type, double &delta_time, double &integral) const;
        protected:
            /// @brief Holds a unique 64 bit region identifier.
            const uint64_t m_identifier;
            /// @brief Holds the compute characteristic hint for this
            ///        region.
            const int m_hint;
            /// @brief Have we converged for this region.
            const unsigned m_num_domain;
            const unsigned m_level;
            std::vector<double> m_signal_matrix;
            std::vector<struct geopm_telemetry_message_s> m_entry_telemetry;
            std::vector<struct geopm_sample_message_s> m_domain_sample;
            std::vector<bool> m_is_dirty_domain_sample;
            struct geopm_sample_message_s m_curr_sample;
            /// @brief Circular buffer is over time, vector is indexed over both domains and signals
            CircularBuffer<std::vector<double> > m_domain_buffer;
            /// @brief time stamp for each entry in the m_domain_buffer
            CircularBuffer<struct geopm_time_s> m_time_buffer;
    };
}

#endif

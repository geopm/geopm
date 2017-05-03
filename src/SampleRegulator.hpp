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

#ifndef SAMPLEREGULATOR_HPP_INCLUDE
#define SAMPLEREGULATOR_HPP_INCLUDE

#include <vector>
#include <set>
#include <map>

#include "CircularBuffer.hpp"
#include "geopm_message.h"

namespace geopm
{
    /// @brief Class merges Platform and Profile time series data.
    ///
    /// The SampleRegulator class is a functor used by the Controller
    /// to create coherent samples while merging data collected from
    /// the Platform and the ProfileSampler.  The ProfileSampler
    /// progress and runtime data is collected asynchronously by each
    /// MPI rank whenever the application MPI ranks use the Profile
    /// interface.  The Platform sample comprises a vector of doubles
    /// collected synchronously.  The cadence of the Platform samples
    /// is determined by the Controller.  The application progress and
    /// runtime data for the region are written some time prior to the
    /// collection of the Platform sample and each have an associated
    /// time stamp.  The SampleRegulator extrapolates the application
    /// provided progress and runtime for each MPI rank to the time
    /// when the Platform was sampled.  The extrapolated application
    /// data is then bundled with the hardware data.  This bundled
    /// data is transformed into a basis that linearly combines
    /// measurements of the same metric so that the combination is
    /// representative of the given metric for each domain of control.
    /// This transformation matrix is an input which is provided by
    /// the Platform.
    ///
    /// The class is a functor and has only one public member
    /// function, the operator ().  This class has been separated from
    /// the Controller to encapsulate this one operation and
    /// facilitate independent testing of this feature.  The
    /// operator () method is broken down into four steps, each
    /// represented by a private method, this enables individual
    /// testing of each step.
    class ISampleRegulator
    {
        public:
            ISampleRegulator() {}
            ISampleRegulator(const ISampleRegulator &other) {}
            virtual ~ISampleRegulator() {}
            /// @brief The parenthesis operator which implements the
            /// SampleRegulator functor.
            ///
            /// This operator implements the public functionality of
            /// the SampleRegulator class, see class description for
            /// more details.  In short, the data from the Platform
            /// object is bundled with extrapolated ProfileSampler
            /// data and transformed to describe the domains of
            /// control.
            ///
            /// @param [in] platform_sample_time The time when the
            /// Platform was sampled which is the time that the
            /// ProfileSampler data is extrapolated to.
            ///
            /// @param [in] signal_domain_matrix Matrix used to
            /// transform the extrapolated data to model the domains
            /// of control.  This matrix is provided by the Platform.
            /// See the Platform for more information.
            ///
            /// @param [in] platform_sample_begin A vector iterator
            /// referencing the beginning of the Platform sample data.
            ///
            /// @param [in] platform_sample_end A vector iterator
            /// referencing the end of the Platform sample data.
            ///
            /// @param [in] prof_sample_begin A vector iterator
            /// referencing the beginning of the ProfileSampler sample
            /// data.
            ///
            /// @param [in] prof_sample_end A vector iterator
            /// referencing the end of the ProfileSampler sample data.
            ///
            /// @param [out] telemetry A vector of
            /// geopm_telemetry_message_s structures which are in
            /// the order of the domains of control defined in a
            /// Policy object.
            virtual void operator () (const struct geopm_time_s &platform_sample_time,
                                      std::vector<double>::const_iterator platform_sample_begin,
                                      std::vector<double>::const_iterator platform_sample_end,
                                      std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                      std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end,
                                      std::vector<double> &aligned_signal,
                                      std::vector<uint64_t> &region_id) = 0;
            virtual const std::map<int, int> &rank_idx_map(void) const = 0;
    };

    class SampleRegulator : public ISampleRegulator
    {
        public:
            /// @brief SampleRegulator constructor.
            ///
            /// Creates data structures used for mapping rank reported
            /// in the profile message to the node local rank which is
            /// used to index intermediate vectors used in the
            /// computation.
            ///
            /// @param [in] cpu_rank A vector of length total number
            /// of CPUs which gives the MPI rank running on each CPU.
            /// Note that each rank may run on multiple CPUs but it is
            /// assumed that each CPU is allocated to a specific MPI
            /// rank.
            SampleRegulator(const std::vector<int> &cpu_rank);
            /// @brief SampleRegulator destructor, virtual.
            virtual ~SampleRegulator();
            void operator () (const struct geopm_time_s &platform_sample_time,
                              std::vector<double>::const_iterator platform_sample_begin,
                              std::vector<double>::const_iterator platform_sample_end,
                              std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                              std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end,
                              std::vector<double> &aligned_signal,
                              std::vector<uint64_t> &region_id);
            const std::map<int, int> &rank_idx_map(void) const;
        protected:
            /// @brief Insert ProfileSampler data.
            void insert(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end);
            /// @brief Insert Platform data.
            void insert(std::vector<double>::const_iterator platform_sample_begin,
                        std::vector<double>::const_iterator platform_sample_end);
            /// @brief Align ProfileSampler data to the time the
            /// Platform data was collected.
            void align(const struct geopm_time_s &timestamp);
            /// @brief Structure to hold a single rank sample.
            struct m_rank_sample_s {
                struct geopm_time_s timestamp;
                double progress;
                double runtime;
            };
            enum m_num_rank_signal_e {
                M_NUM_RANK_SIGNAL = 2,
            };
            enum m_interp_type_e {
                M_INTERP_TYPE_NONE = 0,
                M_INTERP_TYPE_NEAREST = 1,
                M_INTERP_TYPE_LINEAR = 2,
            };
            /// @brief Number of MPI ranks on the node under control.
            int m_num_rank;
            /// @brief A map from the MPI rank reported in the
            /// ProfileSampler data to the node local rank index.
            std::map<int, int> m_rank_idx_map;
            /// @brief The region_id of the stored ProfileSampler data
            /// used for interpolation.
            std::vector<uint64_t> m_region_id;
            /// @brief Per rank record of last profile samples in
            /// m_region_id_prev
            std::vector<ICircularBuffer<struct m_rank_sample_s> *> m_rank_sample_prev;
            /// @brief The platform sample time.
            struct geopm_time_s m_aligned_time;
            /// @brief Vector to multiply with signal_domain_matrix to
            /// project into control domains
            std::vector<double> m_aligned_signal;
            size_t m_num_platform_signal;
    };
}

#endif

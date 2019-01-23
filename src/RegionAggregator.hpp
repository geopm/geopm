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

#ifndef REGIONAGGREGATOR_HPP_INCLUDE
#define REGIONAGGREGATOR_HPP_INCLUDE

#include <cstdint>
#include <cmath>

#include <map>
#include <string>
#include <set>

namespace geopm
{
    class IRegionAggregator
    {
        public:
            IRegionAggregator() = default;
            virtual ~IRegionAggregator() = default;
            /// @brief Push required PlatformIO signals (EPOCH_COUNT).
            virtual void init(void) = 0;
            /// @brief Push a signal to be accumulated per-region.  It
            ///        must be a valid signal available through
            ///        PlatformIO.  Note that unlike other signals
            ///        this is a total accumulated per region by
            ///        subtracting the value of the signal at the
            ///        region exit from the region entry.  Region
            ///        entry and exit are not exact and are determined
            ///        by the value of the REGION_ID# signal at the
            ///        time of read_batch().  This aggregation should
            ///        only be used for signals that are monotonically
            ///        increasing, such as time.
            /// @param [in] signal_name Name of the signal to sample
            ///         and aggregate.
            /// @param [in] domain_type Domain type over which the
            ///        region ID and signal should be sampled.
            /// @param [in] domain_idx Domain over which the region ID
            ///        and signal should be sampled.
            /// @return Index of signal to be used with sample().
            ///         This index matches the return value of
            ///         PlatformIO::push_signal() for the same signal.
            virtual int push_signal_total(const std::string &signal_name,
                                          int domain_type,
                                          int domain_idx) = 0;
            /// @brief Sample a signal that has been pushed to
            ///        accumlate as per-region values.  Note that
            ///        unlike other signals this is a total
            ///        accumulated per region by subtracting the value
            ///        of the signal at the region exit from the
            ///        region entry.  Region entry and exit are not
            ///        exact and are determined by the value of the
            ///        REGION_ID# signal at the time of read_batch().
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal_total.
            /// @param [in] region_id The region ID to look up data
            ///        for.
            /// @return Total accumulated value for the signal for one
            ///        region.
            virtual double sample_total(int signal_idx, uint64_t region_id) = 0;
            /// @brief Update stored totals for each signal after
            ///        PlatformIO::read_batch has been called.  This
            ///        should be called with every PlatformIO update
            ///        because sample_total() maybe not be called
            ///        until the end of execution.
            virtual void read_batch(void) = 0;
            /// @brief Returns the set of region IDs tracked by this
            ///        object.
            virtual std::set<uint64_t> tracked_region_ids(void) const = 0;
    };

    class IPlatformIO;

    class RegionAggregator : public IRegionAggregator
    {
        public:
            RegionAggregator();
            RegionAggregator(IPlatformIO &platio);
            void init(void) override;
            int push_signal_total(const std::string &signal_idx,
                                  int domain_type,
                                  int domain_idx) override;
            double sample_total(int signal_idx, uint64_t region_id) override;
            void read_batch(void) override;
            std::set<uint64_t> tracked_region_ids(void) const override;
        private:
            IPlatformIO &m_platform_io;
            std::map<int, int> m_region_id_idx;
            struct m_region_data_s
            {
                double total = 0.0;
                double last_entry_value = NAN;
            };
            // Data for each combination of signal index and region ID
            std::map<std::pair<int, uint64_t>, m_region_data_s> m_region_sample_data;
            std::map<int, uint64_t> m_last_region_id;
            int m_epoch_count_idx;
            bool m_in_epoch;
            std::map<uint64_t, bool> m_is_ignore;
    };
}

#endif

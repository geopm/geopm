/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SAMPLEAGGREGATORIMP_HPP_INCLUDE
#define SAMPLEAGGREGATORIMP_HPP_INCLUDE

#include <cmath>

#include <map>

#include "geopm/SampleAggregator.hpp"

namespace geopm
{
    class PlatformIO;
    class SumAccumulator;
    class AvgAccumulator;

    class SampleAggregatorImp : public SampleAggregator
    {
        public:
            SampleAggregatorImp();
            SampleAggregatorImp(PlatformIO &platio);
            int push_signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx) override;
            int push_signal_total(const std::string &signal_idx,
                                  int domain_type,
                                  int domain_idx) override;
            int push_signal_average(const std::string &signal_idx,
                                    int domain_type,
                                    int domain_idx) override;
            void update(void) override;
            double sample_application(int signal_idx) override;
            double sample_epoch(int signal_idx) override;
            double sample_region(int signal_idx, uint64_t region_hash) override;
            double sample_epoch_last(int signal_idx) override;
            double sample_region_last(int signal_idx, uint64_t region_hash) override;
            void period_duration(double duration) override;
            int get_period(void) override;
            double sample_period_last(int signal_idx) override;

        private:
            // All of the data relating to a pushed "total" signal
            struct m_sum_signal_s {
                // Value of the signal from last control interval
                double sample_last;
                // PlatformIO signal index to get the region hash
                int region_hash_idx;
                // Value of the hash from last control interval
                uint64_t region_hash_last;
                // PlatformIO signal index to get the epoch count
                int epoch_count_idx;
                // Value of the epoch count from last control interval
                int epoch_count_last;
                // Accumulator for application totals (always updated)
                std::shared_ptr<SumAccumulator> app_accum;
                // Accumulator for epoch totals (updated after first epoch call)
                std::shared_ptr<SumAccumulator> epoch_accum;
                // Accumulator for periodic totals (always updated)
                std::shared_ptr<SumAccumulator> period_accum;
                // Map from region hash to an accumulator for that region
                std::map<uint64_t, std::shared_ptr<SumAccumulator> > region_accum;
                // Iterator pointing to the region_hash_last map location
                std::map<uint64_t, std::shared_ptr<SumAccumulator> >::iterator region_accum_it;
            };

            // All of the data relating to each pushed "average" signal
            struct m_avg_signal_s {
                // Time stamp from last control interval
                double time_last;
                // PlatformIO signal index to get the region hash
                int region_hash_idx;
                // Value of the hash from last control interval
                uint64_t region_hash_last;
                // PlatformIO signal index to get the epoch count
                int epoch_count_idx;
                // Value of the epoch count from last control interval
                int epoch_count_last;
                // Accumulator for application totals (always updated)
                std::shared_ptr<AvgAccumulator> app_accum;
                // Accumulator for epoch totals (updated after first epoch call)
                std::shared_ptr<AvgAccumulator> epoch_accum;
                // Accumulator for periodic totals (always updated)
                std::shared_ptr<AvgAccumulator> period_accum;
                // Map from region hash to an accumulator for that region
                std::map<uint64_t, std::shared_ptr<AvgAccumulator> > region_accum;
                // Iterator pointing to the region_hash_last map location
                std::map<uint64_t, std::shared_ptr<AvgAccumulator> >::iterator region_accum_it;
            };

            void update_total(void);
            void update_average(void);
            double sample_epoch_helper(int signal_idx, bool is_last);
            double sample_region_helper(int signal_idx, uint64_t region_hash, bool is_last);
            uint64_t sample_to_hash(double sample);

            PlatformIO &m_platform_io;
            // PlatformIO signal index for time of last sample
            int m_time_idx;
            bool m_is_updated;
            // Map from index returned by push_signal_total() to the
            // signal structure
            std::map<int, m_sum_signal_s> m_sum_signal;
            // Map from index returned by push_signal_average() to the
            // signal structure
            std::map<int, m_avg_signal_s> m_avg_signal;
            double m_period_duration;
            int m_period_last;
    };
}

#endif

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSTFREQUENCYLIMITDETECTOR_HPP_INCLUDE
#define SSTFREQUENCYLIMITDETECTOR_HPP_INCLUDE

#include <FrequencyLimitDetector.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace geopm
{
    class SSTFrequencyLimitDetector : public FrequencyLimitDetector
    {
        public:
            SSTFrequencyLimitDetector(
                PlatformIO &platform_io,
                const PlatformTopo &platform_topo);
            void update_max_frequency_estimates(
                const std::vector<double> &observed_core_frequencies) override;
            std::vector<std::pair<unsigned int, double> > get_core_frequency_limits(
                unsigned int core_idx) const override;
            double get_core_low_priority_frequency(unsigned int core_idx) const override;

        private:
            // Return true if the given frequency is limited or almost limited
            // by the given limit.
            bool is_frequency_near_limit(double frequency, double limit);

            PlatformIO &m_platform_io;
            unsigned int m_package_count;
            size_t m_core_count;
            std::vector<int> m_clos_association_signals;
            std::vector<int> m_frequency_limit_signals;
            std::vector<int> m_sst_tf_enable_signals;
            const double M_CPU_FREQUENCY_MAX;
            const double M_CPU_FREQUENCY_STICKER;
            const double M_CPU_FREQUENCY_STEP;
            const double M_ALL_CORE_TURBO_FREQUENCY;
            std::vector<unsigned int> m_bucket_hp_cores;
            const double M_LOW_PRIORITY_SSE_FREQUENCY;
            const double M_LOW_PRIORITY_AVX2_FREQUENCY;
            const double M_LOW_PRIORITY_AVX512_FREQUENCY;

            // Vectors of license level frequency limits by bucket number
            std::vector<double> m_bucket_sse_frequency;
            std::vector<double> m_bucket_avx2_frequency;
            std::vector<double> m_bucket_avx512_frequency;
            std::vector<std::pair<unsigned int, double> > m_sse_hp_tradeoffs;
            std::vector<std::pair<unsigned int, double> > m_avx2_hp_tradeoffs;
            std::vector<std::pair<unsigned int, double> > m_avx512_hp_tradeoffs;
            std::vector<std::vector<int> > m_cores_in_packages;
            std::vector<std::vector<std::pair<unsigned int, double> > > m_core_frequency_limits;
            std::vector<double> m_core_lp_frequencies;
    };
}

#endif

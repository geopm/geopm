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

#include "config.h"

#include "SampleAggregatorImp.hpp"

#include "geopm.h"
#include "geopm_internal.h"
#include "geopm_hash.h"
#include "geopm_debug.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Accumulator.hpp"
#include "IOGroup.hpp"

namespace geopm
{
    std::unique_ptr<SampleAggregator> SampleAggregator::make_unique(void)
    {
        return geopm::make_unique<SampleAggregatorImp>();
    }

    SampleAggregatorImp::SampleAggregatorImp()
        : SampleAggregatorImp(platform_io())
    {

    }

    SampleAggregatorImp::SampleAggregatorImp(PlatformIO &platio)
        : m_platform_io(platio)
        , m_time_idx(m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
        , m_is_updated(false)
    {

    }

    int SampleAggregatorImp::push_signal(const std::string &signal_name,
                                         int domain_type,
                                         int domain_idx)
    {
        int result = -1;
        switch (m_platform_io.signal_behavior(signal_name)) {
            case IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT:
                throw Exception("SampleAggregregator::push_signal(): signal_name \"" +
                                signal_name +
                                "\" is constant and cannot be summarized over time.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
            case IOGroup::M_SIGNAL_BEHAVIOR_LABEL:
                throw Exception("SampleAggregregator::push_signal(): signal_name \"" +
                                signal_name +
                                "\" is label and cannot be summarized over time.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);

            case IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE:
                result = push_signal_total(signal_name, domain_type, domain_idx);
                break;
            case IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE:
                result = push_signal_average(signal_name, domain_type, domain_idx);
                break;
            default:
                GEOPM_DEBUG_ASSERT(false, "PlatformIO::signal_behavior() returned enum that is out of bounds");
                break;
        }
        return result;
    }

    int SampleAggregatorImp::push_signal_total(const std::string &signal_name,
                                               int domain_type,
                                               int domain_idx)
    {
        if (m_is_updated) {
           throw Exception("SampleAggregatorImp::push_signal_total(): called after update()",
                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = m_platform_io.push_signal(signal_name, domain_type, domain_idx);
        auto bad_it = m_avg_signal.find(result);
        if (bad_it != m_avg_signal.end()) {
           throw Exception("SampleAggregatorImp::push_signal_total(): signal already pushed for average",
                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto signal_it = m_sum_signal.find(result);
        if (signal_it == m_sum_signal.end()) {
            m_sum_signal[result] = {
                NAN,
                m_platform_io.push_signal("REGION_HASH", domain_type, domain_idx),
                GEOPM_REGION_HASH_INVALID,
                m_platform_io.push_signal("EPOCH_COUNT", domain_type, domain_idx),
                0,
                SumAccumulator::make_unique(),
                SumAccumulator::make_unique(),
                SumAccumulator::make_unique(),
                {},
                {},
           };
        }
        return result;
    }

    int SampleAggregatorImp::push_signal_average(const std::string &signal_name,
                                                 int domain_type,
                                                 int domain_idx)
    {
        if (m_is_updated) {
           throw Exception("SampleAggregatorImp::push_signal_average(): called after update()",
                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = m_platform_io.push_signal(signal_name, domain_type, domain_idx);
        auto bad_it = m_sum_signal.find(result);
        if (bad_it != m_sum_signal.end()) {
           throw Exception("SampleAggregatorImp::push_signal_average(): signal already pushed for total",
                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto signal_it = m_avg_signal.find(result);
        if (signal_it == m_avg_signal.end()) {
            m_avg_signal[result] = {
                NAN,
                m_platform_io.push_signal("REGION_HASH", domain_type, domain_idx),
                GEOPM_REGION_HASH_INVALID,
                m_platform_io.push_signal("EPOCH_COUNT", domain_type, domain_idx),
                0,
                AvgAccumulator::make_unique(),
                AvgAccumulator::make_unique(),
                AvgAccumulator::make_unique(),
                {},
                {},
           };
        }
        return result;
    }

    template<typename type>
    typename std::map<uint64_t, std::shared_ptr<type> >::iterator
    sample_aggregator_emplace_hash(
        std::map<uint64_t, std::shared_ptr<type> > &hash_accum_map,
        uint64_t hash)
    {
        typename std::map<uint64_t, std::shared_ptr<type> >::iterator result;
        result = hash_accum_map.find(hash);
        if (result == hash_accum_map.end()) {
            auto empl_ret = hash_accum_map.emplace(hash, type::make_unique());
            result = empl_ret.first;
        }
        return result;
    }

    template <typename type>
    void sample_aggregator_update_epoch(type &signal, int epoch_count)
    {
        // If the epoch count has changed, call the exit/enter
        if (epoch_count != signal.epoch_count_last) {
            if (signal.epoch_count_last != 0) {
                signal.epoch_accum->exit();
            }
            signal.epoch_accum->enter();
            signal.epoch_count_last = epoch_count;
        }
    }

    template <typename type>
    void sample_aggregator_update_hash_exit(type &signal, uint64_t hash)
    {
        if (signal.region_hash_last != hash) {
            // If we have exited a valid region, call exit()
            if (signal.region_hash_last != GEOPM_REGION_HASH_UNMARKED) {
                signal.region_accum_it->second->exit();
            }
        }
    }

    template <typename type>
    void sample_aggregator_update_hash_enter(type &signal, uint64_t hash)
    {
        if (signal.region_hash_last != hash) {
            // If we have entered a valid region, call enter()
            if (hash != GEOPM_REGION_HASH_UNMARKED) {
                signal.region_accum_it->second->enter();
            }
        }
    }

    template <typename type>
    void sample_aggregator_update_period(type &signal, int period)
    {
        if (period != 0) {
            signal.period_accum->exit();
        }
        signal.period_accum->enter();
    }

    uint64_t SampleAggregatorImp::sample_to_hash(double sample)
    {
        uint64_t result = sample;
        if (std::isnan(sample)) {
            result = GEOPM_REGION_HASH_INVALID;
        }
        return result;
    }

    void SampleAggregatorImp::period_duration(double duration)
    {
        m_period_duration = duration;
    }

    int SampleAggregatorImp::get_period(void)
    {
        int result = 0;
        if (m_period_duration) {
            double time = m_platform_io.sample(m_time_idx);
            result = static_cast<int>(time / m_period_duration);
        }
        return result;
    }

    void SampleAggregatorImp::update_total(void)
    {
        int period = get_period();
        // Update all of the sum aggregators
        for (auto &signal_it : m_sum_signal) {
            int signal_idx = signal_it.first;
            m_sum_signal_s &signal = signal_it.second;
            double sample = m_platform_io.sample(signal_idx);
            uint64_t hash = sample_to_hash(m_platform_io.sample(signal.region_hash_idx));
            int epoch_count = m_platform_io.sample(signal.epoch_count_idx);
            if (!m_is_updated) {
                // On first call just initialize the signal values
                signal.sample_last = sample;
                signal.region_hash_last = hash;
                signal.epoch_count_last = epoch_count;
                signal.region_accum_it = sample_aggregator_emplace_hash(signal.region_accum, hash);
            }
            else if (hash != GEOPM_REGION_HASH_INVALID) {
                // Measure the change since the last update
                double delta = sample - signal.sample_last;
                // Update that application totals
                signal.app_accum->update(delta);
                // If we have observed our first epoch, update epoch totals
                if (signal.epoch_count_last != 0) {
                    signal.epoch_accum->update(delta);
                }
                // Update the periodic totals
                signal.period_accum->update(delta);
                // Update region totals
                signal.region_accum_it->second->update(delta);
                sample_aggregator_update_epoch(signal, epoch_count);
                sample_aggregator_update_hash_exit(signal, hash);
                if (signal.region_hash_last != hash) {
                    signal.region_accum_it = sample_aggregator_emplace_hash(signal.region_accum, hash);
                }
                sample_aggregator_update_hash_enter(signal, hash);
                if (period != m_period_last) {
                    sample_aggregator_update_period(signal, period);
                }
                signal.region_hash_last = hash;
                signal.sample_last = sample;
            }
        }
    }

    void SampleAggregatorImp::update_average(void)
    {
        int period = get_period();
        // Update all of the average aggregators
        for (auto &signal_it : m_avg_signal) {
            int signal_idx = signal_it.first;
            m_avg_signal_s &signal = signal_it.second;
            double time = m_platform_io.sample(m_time_idx);
            double sample = m_platform_io.sample(signal_idx);
            uint64_t hash = sample_to_hash(m_platform_io.sample(signal.region_hash_idx));
            int epoch_count = m_platform_io.sample(signal.epoch_count_idx);
            if (!m_is_updated) {
                // On first call just initialize the signal values
                signal.time_last = time;
                signal.region_hash_last = hash;
                signal.epoch_count_last = epoch_count;
                signal.region_accum_it = sample_aggregator_emplace_hash(signal.region_accum, hash);
            }
            else if (hash != GEOPM_REGION_HASH_INVALID) {
                // Measure the time change since the last update
                double delta = time - signal.time_last;
                // Update that application totals
                signal.app_accum->update(delta, sample);
                // If we have observed our first epoch, update epoch totals
                if (signal.epoch_count_last != 0) {
                    signal.epoch_accum->update(delta, sample);
                }
                // Update the periodic totals
                signal.period_accum->update(delta, sample);
                // Update region totals
                signal.region_accum_it->second->update(delta, sample);
                sample_aggregator_update_epoch(signal, epoch_count);
                sample_aggregator_update_hash_exit(signal, hash);
                if (signal.region_hash_last != hash) {
                    signal.region_accum_it = sample_aggregator_emplace_hash(signal.region_accum, hash);
                }
                sample_aggregator_update_hash_enter(signal, hash);
                if (period != m_period_last) {
                    sample_aggregator_update_period(signal, period);
                }
                signal.region_hash_last = hash;
                signal.time_last = time;
            }
        }
    }

    void SampleAggregatorImp::update(void)
    {
        update_total();
        update_average();
        m_period_last = get_period();
        m_is_updated = true;
    }

    double SampleAggregatorImp::sample_application(int signal_idx)
    {
        double result = NAN;
        auto sum_it = m_sum_signal.find(signal_idx);
        if (sum_it != m_sum_signal.end()) {
            result = sum_it->second.app_accum->total();
        }
        else {
            auto avg_it = m_avg_signal.find(signal_idx);
            if (avg_it == m_avg_signal.end()) {
                throw Exception("SampleAggregator::sample_application(): Invalid signal index: signal index not pushed with push_signal_total() or push_signal_average()",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = avg_it->second.app_accum->average();
        }
        return result;
    }

    double SampleAggregatorImp::sample_epoch_helper(int signal_idx, bool is_last)
    {
        double result = NAN;
        auto sum_it = m_sum_signal.find(signal_idx);
        if (sum_it != m_sum_signal.end()) {
            if (is_last) {
                result = sum_it->second.epoch_accum->interval_total();
            }
            else {
                result = sum_it->second.epoch_accum->total();
            }
        }
        else {
            auto avg_it = m_avg_signal.find(signal_idx);
            if (avg_it == m_avg_signal.end()) {
                throw Exception("SampleAggregator::sample_epoch(): Invalid signal index: signal index not pushed with push_signal_total() or push_signal_average()",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (is_last) {
                result = avg_it->second.epoch_accum->interval_average();
            }
            else {
                result = avg_it->second.epoch_accum->average();
            }
        }
        return result;
    }

    double SampleAggregatorImp::sample_region_helper(int signal_idx, uint64_t region_hash, bool is_last)
    {
        double result = NAN;
        auto sum_it = m_sum_signal.find(signal_idx);
        if (sum_it != m_sum_signal.end()) {
            const auto &region_accum = sum_it->second.region_accum;
            auto region_accum_it = region_accum.find(region_hash);
            if (region_accum_it == region_accum.end()) {
                result = 0.0;
            }
            else {
                auto accum_ptr = region_accum_it->second;
                if (is_last) {
                    result = accum_ptr->interval_total();
                }
                else {
                    result = accum_ptr->total();
                }
            }
        }
        else {
            auto avg_it = m_avg_signal.find(signal_idx);
            if (avg_it == m_avg_signal.end()) {
                throw Exception("SampleAggregator::sample_region(): Invalid signal index: signal index not pushed with push_signal_total() or push_signal_average()",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            const auto &region_accum = avg_it->second.region_accum;
            auto region_accum_it = region_accum.find(region_hash);
            if (region_accum_it != region_accum.end()) {
                auto accum_ptr = region_accum_it->second;
                if (is_last) {
                    result = accum_ptr->interval_average();
                }
                else {
                    result = accum_ptr->average();
                }
            }
        }
        return result;
    }

    double SampleAggregatorImp::sample_region(int signal_idx, uint64_t region_hash)
    {
        if (region_hash == GEOPM_REGION_HASH_EPOCH) {
            return sample_epoch(signal_idx);
        }
        else if (region_hash == GEOPM_REGION_HASH_APP) {
            return sample_application(signal_idx);
        }
        return sample_region_helper(signal_idx, region_hash, false);
    }

    double SampleAggregatorImp::sample_region_last(int signal_idx, uint64_t region_hash)
    {
        if (region_hash == GEOPM_REGION_HASH_EPOCH) {
            return sample_epoch_last(signal_idx);
        }
        return sample_region_helper(signal_idx, region_hash, true);
    }

    double SampleAggregatorImp::sample_epoch(int signal_idx)
    {
        return sample_epoch_helper(signal_idx, false);
    }

    double SampleAggregatorImp::sample_epoch_last(int signal_idx)
    {
        return sample_epoch_helper(signal_idx, true);
    }

    double SampleAggregatorImp::sample_period_last(int signal_idx)
    {
        double result = NAN;
        if (m_period_duration != 0.0) {
            auto sum_it = m_sum_signal.find(signal_idx);
            if (sum_it != m_sum_signal.end()) {
                result = sum_it->second.period_accum->interval_total();
            }
            else {
                auto avg_it = m_avg_signal.find(signal_idx);
                if (avg_it == m_avg_signal.end()) {
                    throw Exception("SampleAggregator::sample_period(): Invalid signal index: signal index not pushed with push_signal_total() or push_signal_average()",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                result = avg_it->second.period_accum->interval_average();
            }
        }
        return result;
    }

}

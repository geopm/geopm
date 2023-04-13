/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ProcessRegionAggregator.hpp"

#include "ApplicationSampler.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "record.hpp"
#include "geopm_time.h"

namespace geopm
{
    std::unique_ptr<ProcessRegionAggregator> ProcessRegionAggregator::make_unique(void)
    {
        return geopm::make_unique<ProcessRegionAggregatorImp>();
    }

    ProcessRegionAggregatorImp::ProcessRegionAggregatorImp()
        : ProcessRegionAggregatorImp(ApplicationSampler::application_sampler())
    {

    }

    ProcessRegionAggregatorImp::ProcessRegionAggregatorImp(ApplicationSampler &sampler)
        : m_app_sampler(sampler)
    {
        m_num_process = m_app_sampler.client_pids().size();
        if (m_num_process == 0) {
            throw Exception("ProcessRegionAggregator: expected at least one process",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void ProcessRegionAggregatorImp::update(void)
    {
        geopm_time_s time_zero = geopm::time_zero();
        auto records = m_app_sampler.get_records();
        for (const auto &rec: records) {
            if (rec.event == EVENT_REGION_ENTRY) {
                int process = rec.process;
                uint64_t region_hash = rec.signal;
                double entry_time = geopm_time_diff(&time_zero, &(rec.time));
                auto proc = m_region_info.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(process),
                                                  std::forward_as_tuple());
                auto region = proc.first->second.emplace(std::piecewise_construct,
                                                         std::forward_as_tuple(region_hash),
                                                         std::forward_as_tuple());
                region.first->second.last_entry_time = entry_time;
            }
            else if (rec.event == EVENT_REGION_EXIT) {
                int process = rec.process;
                uint64_t region_hash = rec.signal;
                double exit_time = geopm_time_diff(&time_zero, &(rec.time));
                auto proc = m_region_info.find(process);
                if (proc == m_region_info.end()) {
                    throw Exception("ProcessRegionAggregator: region exit without entry",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                auto region = proc->second.find(region_hash);
                if (region == proc->second.end()) {
                    throw Exception("ProcessRegionAggregator: region exit without entry",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                region->second.total_runtime += exit_time - region->second.last_entry_time;
                region->second.total_count += 1;
            }
            else if (rec.event == EVENT_SHORT_REGION) {
                int process = rec.process;
                int short_idx = rec.signal;
                auto short_region = m_app_sampler.get_short_region(short_idx);
                uint64_t region_hash = short_region.hash;

                auto proc = m_region_info.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(process),
                                                  std::forward_as_tuple());
                auto region = proc.first->second.emplace(std::piecewise_construct,
                                                         std::forward_as_tuple(region_hash),
                                                         std::forward_as_tuple());
                region.first->second.total_runtime += short_region.total_time;
                region.first->second.total_count += short_region.num_complete;
            }
        }
    }

    double ProcessRegionAggregatorImp::get_runtime_average(uint64_t region_hash) const
    {
        double total = 0;
        for (const auto &kv : m_region_info) {
            auto it = kv.second.find(region_hash);
            if (it != kv.second.end()) {
                total += it->second.total_runtime;
            }
        }
        total = total / m_num_process;
        return total;
    }

    double ProcessRegionAggregatorImp::get_count_average(uint64_t region_hash) const
    {
        double total = 0;
        for (const auto &kv : m_region_info) {
            auto it = kv.second.find(region_hash);
            if (it != kv.second.end()) {
                total += it->second.total_count;
            }
        }
        total = total / m_num_process;
        return total;
    }
}

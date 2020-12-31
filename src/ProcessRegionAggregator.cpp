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

#include "ProcessRegionAggregator.hpp"

#include "ApplicationSampler.hpp"
#include "Helper.hpp"
#include "record.hpp"
#include "geopm_debug.hpp"

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
        std::set<int> procs;
        for (const auto &pp : m_app_sampler.per_cpu_process()) {
            if (pp != -1) {
                procs.insert(pp);
            }
        }
        m_num_process = procs.size();
        if (m_num_process == 0) {
            throw Exception("ProcessRegionAggregator: expected at least one process",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void ProcessRegionAggregatorImp::update(void)
    {
        auto records = m_app_sampler.get_records();
        for (const auto &rec: records) {
            if (rec.event == EVENT_REGION_ENTRY) {
                int process = rec.process;
                uint64_t region_hash = rec.signal;
                double entry_time = rec.time;
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
                double exit_time = rec.time;
                auto proc = m_region_info.find(process);
                GEOPM_DEBUG_ASSERT(proc != m_region_info.end(),
                                   "ProcessRegionAggregator: region exit without entry");
                auto region = proc->second.find(region_hash);
                GEOPM_DEBUG_ASSERT(region != proc->second.end(),
                                   "ProcessRegionAggregator: region exit without entry");
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

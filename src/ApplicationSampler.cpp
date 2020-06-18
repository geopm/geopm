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

#include <map>
#include <functional>

#include "ApplicationSamplerImp.hpp"
#include "ProfileSampler.hpp"
#include "ProfileIOSample.hpp"
#include "EpochRuntimeRegulator.hpp"
#include "Exception.hpp"
#include "RecordFilter.hpp"
#include "Environment.hpp"
#include "ValidateRecord.hpp"
#include "record.hpp"

namespace geopm
{
    ApplicationSampler &ApplicationSampler::application_sampler(void)
    {
        static ApplicationSamplerImp instance;
        return instance;
    }

    ApplicationSamplerImp::ApplicationSamplerImp()
        : ApplicationSamplerImp(environment().do_record_filter() ?
                                // use record filter from environment
                                [](void) {
                                    return RecordFilter::make_unique(environment().record_filter());
                                } :
                                // otherwise, do not filter records
                                std::function<std::unique_ptr<RecordFilter>()>()
                                )
    {

    }

    ApplicationSamplerImp::ApplicationSamplerImp(std::function<std::unique_ptr<RecordFilter>()> filter_factory)
        : m_time_zero(geopm::time_zero())
        , m_filter_factory(filter_factory)
        , m_is_filtered(filter_factory)
    {

    }

    void ApplicationSamplerImp::time_zero(const geopm_time_s &start_time)
    {
        m_time_zero = start_time;
    }

    void ApplicationSamplerImp::update_records_epoch(const geopm_prof_message_s &msg)
    {
        double time = geopm_time_diff(&m_time_zero, &(msg.timestamp));
        int process = msg.rank;
        int event = EVENT_EPOCH_COUNT;
        m_process_s &proc_it = get_process(process);
        ++(proc_it.epoch_count);
        uint64_t epoch_count = proc_it.epoch_count;
        m_record_buffer.emplace_back(record_s {
            .time = time,
            .process = process,
            .event = event,
            .signal = epoch_count,
        });
    }

    void ApplicationSamplerImp::update_records_mpi(const geopm_prof_message_s &msg)
    {
        double time = geopm_time_diff(&m_time_zero, &(msg.timestamp));
        int process = msg.rank;
        int event = EVENT_HINT;
        uint64_t hint = GEOPM_REGION_HINT_NETWORK;
        if (msg.progress == 1.0) {
            m_process_s &proc_it = get_process(process);
            hint = proc_it.hint;
        }
        m_record_buffer.emplace_back(record_s {
            .time = time,
            .process = process,
            .event = event,
            .signal = hint,
        });
    }

    void ApplicationSamplerImp::update_records_entry(const geopm_prof_message_s &msg)
    {
        double time = geopm_time_diff(&m_time_zero, &(msg.timestamp));
        int process = msg.rank;
        uint64_t hash = geopm_region_id_hash(msg.region_id);
        int event = EVENT_REGION_ENTRY;
        // Record event for change of region ID
        m_record_buffer.emplace_back(record_s {
              .time = time,
              .process = process,
              .event = event,
              .signal = hash,
        });
        // Record event for change of hint
        uint64_t hint = geopm_region_id_hint(msg.region_id);
        m_process_s &proc_it = get_process(process);
        proc_it.hint = hint;
        m_record_buffer.emplace_back(record_s {
            .time = time,
            .process = process,
            .event = EVENT_HINT,
            .signal = hint,
        });
    }

    void ApplicationSamplerImp::update_records_exit(const geopm_prof_message_s &msg)
    {
        // Set common values for all events
        double time = geopm_time_diff(&m_time_zero, &(msg.timestamp));
        int process = msg.rank;
        /// Handle outer region entry and exit
        uint64_t hash = geopm_region_id_hash(msg.region_id);
        int event = EVENT_REGION_EXIT;
        // Record event for change of region ID
        m_record_buffer.emplace_back(record_s {
            .time = time,
            .process = process,
            .event = event,
            .signal = hash,
        });
        // Record event for change of hint
        uint64_t hint = GEOPM_REGION_HINT_UNKNOWN;
        m_process_s &proc_it = get_process(process);
        proc_it.hint = hint;
        m_record_buffer.emplace_back(record_s {
            .time = time,
            .process = process,
            .event = EVENT_HINT,
            .signal = hint,
        });
    }

    void ApplicationSamplerImp::update_records_progress(const geopm_prof_message_s &msg)
    {
        /// @todo handle calls to progress
    }

    void ApplicationSamplerImp::update_records(void)
    {
        m_record_buffer.clear();
        for (auto &cache_it : m_sampler->sample_cache()) {
            if (geopm_region_id_is_epoch(cache_it.region_id)) {
                update_records_epoch(cache_it);
            }
            else if (geopm_region_id_is_mpi(cache_it.region_id)) {
                update_records_mpi(cache_it);
            }
            else if (cache_it.progress == 0.0) {
                update_records_entry(cache_it);
            }
            else if (cache_it.progress == 1.0) {
                update_records_exit(cache_it);
            }
            else {
                update_records_progress(cache_it);
            }
        }

        std::vector<record_s> tmp_buffer;
        for (auto &record_it : m_record_buffer) {
            auto &proc = get_process(record_it.process);
            if (m_is_filtered) {
                for (auto &filtered_it : proc.filter->filter(record_it)) {
                    proc.valid.check(filtered_it);
                    tmp_buffer.push_back(filtered_it);
                }
            }
            else {
                proc.valid.check(record_it);
            }
        }
        if (m_is_filtered) {
            m_record_buffer = tmp_buffer;
        }
    }

    std::vector<record_s> ApplicationSamplerImp::get_records(void) const
    {
        return m_record_buffer;
    }


    std::map<uint64_t, std::string> ApplicationSamplerImp::get_name_map(uint64_t name_key) const
    {
        throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "() is not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return {};
    }

    std::vector<uint64_t> ApplicationSamplerImp::per_cpu_hint(void) const
    {
        throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "() is not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return {};
    }

    std::vector<double> ApplicationSamplerImp::per_cpu_progress(void) const
    {
        throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "() is not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return {};
    }

    std::vector<int> ApplicationSamplerImp::per_cpu_process(void) const
    {
        return m_sampler->cpu_rank();
    }

    void ApplicationSamplerImp::set_sampler(std::shared_ptr<ProfileSampler> sampler)
    {
        m_sampler = sampler;
        // Each message can create two records
        m_record_buffer.reserve(2 * m_sampler->capacity());
    }

    std::shared_ptr<ProfileSampler> ApplicationSamplerImp::get_sampler(void)
    {
        return m_sampler;
    }

    void ApplicationSamplerImp::set_regulator(std::shared_ptr<EpochRuntimeRegulator> regulator)
    {
        m_regulator = regulator;
    }

    std::shared_ptr<EpochRuntimeRegulator> ApplicationSamplerImp::get_regulator(void)
    {
        return m_regulator;
    }

    void ApplicationSamplerImp::set_io_sample(std::shared_ptr<ProfileIOSample> io_sample)
    {
        m_io_sample = io_sample;
    }

    std::shared_ptr<ProfileIOSample> ApplicationSamplerImp::get_io_sample(void)
    {
        return m_io_sample;
    }

    ApplicationSamplerImp::m_process_s &ApplicationSamplerImp::get_process(int process)
    {
        auto emp_it = m_process_map.emplace(process, m_process_s {
            .epoch_count = 0LL,
            .hint = GEOPM_REGION_HINT_UNKNOWN,
            .filter = nullptr,
        });
        if (emp_it.second && m_is_filtered) {
            emp_it.first->second.filter = m_filter_factory();
        }
        return emp_it.first->second;
    }
}

/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef APPLICATIONSAMPLERIMP_HPP_INCLUDE
#define APPLICATIONSAMPLERIMP_HPP_INCLUDE

#include "ApplicationSampler.hpp"
#include "ValidateRecord.hpp"

struct geopm_prof_message_s;
namespace geopm
{
    class RecordFilter;
    class ApplicationStatus;
    class SharedMemory;
    class PlatformTopo;

    class ApplicationSamplerImp : public ApplicationSampler
    {
        public:
            struct m_process_s {
                std::shared_ptr<RecordFilter> filter;
                ValidateRecord valid;
                std::shared_ptr<SharedMemory> record_log_shmem;
                std::shared_ptr<ApplicationRecordLog> record_log;
                std::vector<record_s> records;
                std::vector<short_region_s> short_regions;
            };
            ApplicationSamplerImp();
            ApplicationSamplerImp(std::shared_ptr<ApplicationStatus> status,
                                  const PlatformTopo &platform_topo,
                                  const std::map<int, m_process_s> &process_map,
                                  bool is_filtered,
                                  const std::string &filter_name,
                                  const std::vector<bool> &is_cpu_active);
            virtual ~ApplicationSamplerImp();
            void time_zero(const geopm_time_s &start_time) override;
            void update(const geopm_time_s &curr_time) override;
            std::vector<record_s> get_records(void) const override;
            short_region_s get_short_region(uint64_t event_signal) const override;
            std::map<uint64_t, std::string> get_name_map(uint64_t name_key) const override;
            uint64_t cpu_region_hash(int cpu_idx) const override;
            uint64_t cpu_hint(int cpu_idx) const override;
            double cpu_hint_time(int cpu_idx, uint64_t hint) const override;
            double cpu_progress(int cpu_idx) const override;
            std::vector<int> per_cpu_process(void) const override;
            void connect(const std::string &shm_key) override;
            int sampler_cpu(void);

            void set_sampler(std::shared_ptr<ProfileSampler> sampler) override;
            std::shared_ptr<ProfileSampler> get_sampler(void) override;
        private:
            std::shared_ptr<ProfileSampler> m_sampler;
            struct geopm_time_s m_time_zero;
            std::vector<record_s> m_record_buffer;
            std::vector<short_region_s> m_short_region_buffer;
            std::shared_ptr<SharedMemory> m_status_shmem;
            std::shared_ptr<ApplicationStatus> m_status;
            const PlatformTopo &m_topo;
            int m_num_cpu;
            std::map<int, m_process_s> m_process_map;
            const bool m_is_filtered;
            const std::string m_filter_name;
            static const std::map<uint64_t, double> m_hint_time_init;
            std::vector<std::map<uint64_t, double> > m_hint_time;
            std::vector<bool> m_is_cpu_active;
            geopm_time_s m_update_time;
            bool m_is_first_update;
            std::vector<uint64_t> m_hint_last;
    };
}

#endif

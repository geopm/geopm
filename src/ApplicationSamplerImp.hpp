/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APPLICATIONSAMPLERIMP_HPP_INCLUDE
#define APPLICATIONSAMPLERIMP_HPP_INCLUDE

#include "ApplicationSampler.hpp"
#include "ValidateRecord.hpp"
#include "geopm_hint.h"

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
                std::vector<int> cpus;
            };
            ApplicationSamplerImp();
            ApplicationSamplerImp(std::shared_ptr<ApplicationStatus> status,
                                  const PlatformTopo &platform_topo,
                                  const std::map<int, m_process_s> &process_map,
                                  bool is_filtered,
                                  const std::string &filter_name,
                                  const std::vector<bool> &is_cpu_active,
                                  bool do_profile,
                                  const std::string &profile_name);
            virtual ~ApplicationSamplerImp() = default;
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
            void connect(void) override;
            int sampler_cpu(void);
        private:
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
            std::vector<std::array<double, GEOPM_NUM_REGION_HINT>> m_hint_time;
            std::vector<bool> m_is_cpu_active;
            geopm_time_s m_update_time;
            bool m_is_first_update;
            std::vector<uint64_t> m_hint_last;
            bool m_do_profile;
            std::vector<int> m_per_cpu_process;
            std::string m_profile_name;
    };
}

#endif

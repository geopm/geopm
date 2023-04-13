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
    class Scheduler;

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
                                  const std::vector<bool> &is_cpu_active,
                                  bool do_profile,
                                  const std::string &profile_name,
                                  const std::map<int, std::set<int> > &client_cpu_map,
                                  std::shared_ptr<Scheduler> scheduler);
            virtual ~ApplicationSamplerImp() = default;
            void time_zero(const geopm_time_s &start_time) override;
            void update(const geopm_time_s &curr_time) override;
            std::vector<record_s> get_records(void) const override;
            short_region_s get_short_region(uint64_t event_signal) const override;
            uint64_t cpu_region_hash(int cpu_idx) const override;
            uint64_t cpu_hint(int cpu_idx) const override;
            double cpu_hint_time(int cpu_idx, uint64_t hint) const override;
            double cpu_progress(int cpu_idx) const override;
            void connect(const std::vector<int> &client_pids) override;
            std::vector<int> client_pids(void) const override;
            std::set<int> client_cpu_set(int client_pid) const override;
            int sampler_cpu(void);
        private:
            std::map<int, m_process_s> connect_record_log(const std::vector<int> &client_pids);
            void connect_status(void);
            std::map<int, std::set<int> > update_client_cpu_map(const std::vector<int> &client_pids);
            std::map<int, std::set<int> > update_client_cpu_map_helper(const std::vector<int> &client_pids);
            void update_cpu_active(void);
            struct geopm_time_s m_time_zero;
            std::vector<record_s> m_record_buffer;
            std::vector<short_region_s> m_short_region_buffer;
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
            std::string m_profile_name;
            std::map<int, std::set<int> > m_client_cpu_map;
            std::shared_ptr<Scheduler> m_scheduler;
    };
}

#endif

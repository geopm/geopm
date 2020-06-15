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

#ifndef APPLICATIONSAMPLERIMP_HPP_INCLUDE
#define APPLICATIONSAMPLERIMP_HPP_INCLUDE

#include "ApplicationSampler.hpp"
#include "ValidateRecord.hpp"


struct geopm_prof_message_s;
namespace geopm
{
    class RecordFilter;

    class ApplicationSamplerImp : public ApplicationSampler
    {
        public:
            ApplicationSamplerImp();
            ApplicationSamplerImp(std::function<std::unique_ptr<RecordFilter>()> filter_factory);
            virtual ~ApplicationSamplerImp() = default;
            void time_zero(const geopm_time_s &start_time) override;
            void update_records(void) override;
            std::vector<record_s> get_records(void) const override;
            std::map<uint64_t, std::string> get_name_map(uint64_t name_key) const override;
            std::vector<uint64_t> per_cpu_hint(void) const override;
            std::vector<double> per_cpu_progress(void) const override;
            std::vector<int> per_cpu_process(void) const override;

            void set_sampler(std::shared_ptr<ProfileSampler> sampler) override;
            std::shared_ptr<ProfileSampler> get_sampler(void) override;
            void set_regulator(std::shared_ptr<EpochRuntimeRegulator> regulator) override;
            std::shared_ptr<EpochRuntimeRegulator> get_regulator(void) override;
            void set_io_sample(std::shared_ptr<ProfileIOSample> io_sample) override;
            std::shared_ptr<ProfileIOSample> get_io_sample(void) override;
        private:
            struct m_process_s {
                uint64_t epoch_count;
                uint64_t hint;
                std::shared_ptr<RecordFilter> filter;
                ValidateRecord valid;
            };
            void update_records_epoch(const geopm_prof_message_s &msg);
            void update_records_mpi(const geopm_prof_message_s &msg);
            void update_records_entry(const geopm_prof_message_s &msg);
            void update_records_exit(const geopm_prof_message_s &msg);
            void update_records_progress(const geopm_prof_message_s &msg);
            m_process_s &get_process(int process);
            std::shared_ptr<ProfileSampler> m_sampler;
            std::shared_ptr<EpochRuntimeRegulator> m_regulator;
            std::shared_ptr<ProfileIOSample> m_io_sample;
            struct geopm_time_s m_time_zero;
            std::vector<record_s> m_record_buffer;
            std::map<int, m_process_s> m_process_map;
            std::function<std::unique_ptr<RecordFilter>()> m_filter_factory;
            bool m_is_filtered;
    };
}

#endif

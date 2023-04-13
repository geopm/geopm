/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROFILETRACERIMP_HPP_INCLUDE
#define PROFILETRACERIMP_HPP_INCLUDE

#include "config.h"

#include "ProfileTracer.hpp"
#include "ApplicationSampler.hpp"
#include "geopm_time.h"

namespace geopm
{
    struct record_s;

    class ProfileTracerImp : public ProfileTracer
    {
        public:
            ProfileTracerImp(const std::string &start_time);
            ProfileTracerImp(const std::string &start_time,
                             const geopm_time_s &time_zero,
                             size_t buffer_size,
                             bool is_trace_enabled,
                             const std::string &file_name,
                             const std::string &host_name,
                             ApplicationSampler& application_sampler = ApplicationSampler::application_sampler());
            virtual ~ProfileTracerImp();
            void update(const std::vector<record_s> &records);
         private:
             enum m_column_e {
                M_COLUMN_TIME,
                M_COLUMN_PROCESS,
                M_COLUMN_EVENT,
                M_COLUMN_SIGNAL,
                M_NUM_COLUMN
            };
            bool m_is_trace_enabled;
            std::unique_ptr<CSV> m_csv;
            geopm_time_s m_time_zero;
            static ApplicationSampler* m_application_sampler;
            static std::string event_format(double value);
    };
}

#endif

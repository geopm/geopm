/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROFILETRACER_HPP_INCLUDE
#define PROFILETRACER_HPP_INCLUDE

#include <vector>
#include <utility>
#include <fstream>
#include <memory>

struct geopm_prof_message_s;

namespace geopm
{
    class CSV;
    struct record_s;

    class ProfileTracer
    {
        public:
            static std::unique_ptr<ProfileTracer> make_unique(const std::string &start_time);
            virtual ~ProfileTracer() = default;
            virtual void update(const std::vector<record_s> &records) = 0;
    };
}

#endif

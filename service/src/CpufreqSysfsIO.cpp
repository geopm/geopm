/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "CpufreqSysfsIO.hpp"

namespace geopm
{
    CpufreqSysfsIO::CpufreqSysfsIO()
        : m_properties {}
        , m_name_map {}
    {

    }

    std::vector<std::string> CpufreqSysfsIO::signal_names(void) const
    {
        std::vector<std::string> result;
        return result;
    }

    std::vector<std::string> CpufreqSysfsIO::control_names(void) const
    {
        std::vector<std::string> result;
        return result;
    }

    std::string CpufreqSysfsIO::signal_path(const std::string &signal_name,
                                            int domain_type,
                                            int domain_idx)
    {
        std::string result;
        return result;
    }

    std::string CpufreqSysfsIO::control_path(const std::string &control_name,
                             int domain_type,
                             int domain_idx) const
    {
        std::string result;
        return result;
    }

    double CpufreqSysfsIO::signal_parse(const std::string &signal_name,
                                        const std::string &content) const
    {
        double result = 0.0;
        return result;
    }

    std::string CpufreqSysfsIO::control_gen(const std::string &control_name,
                                            double setting) const
    {
        std::string result;
        return result;
    }


    double CpufreqSysfsIO::signal_parse(int properties_id,
                                        const std::string &content) const
    {
        double result = 0.0;
        return result;
    }

    std::string CpufreqSysfsIO::control_gen(int properties_id,
                                            double setting) const
    {
        std::string result;
        return result;
    }

    std::string CpufreqSysfsIO::driver(void) const
    {
        std::string result;
        return result;
    }

    struct SysfsIO::properties_s CpufreqSysfsIO::properties(const std::string &name) const
    {
        struct SysfsIO::properties_s result {};
        return result;
    }

    std::string CpufreqSysfsIO::properties_json(void) const
    {
        std::string result;
        return result;
    }
}

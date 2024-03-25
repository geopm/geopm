/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "EnvironmentParser.hpp"

#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    std::vector<std::pair<std::string, int> > environment_signal_parser(const std::set<std::string> &valid_signals,
                                                                        const std::string &environment_variable_contents)
    {
        std::vector<std::pair<std::string, int> > result_data_structure;

        auto individual_signals = geopm::string_split(environment_variable_contents, ",");
        for (const auto &signal : individual_signals) {
            auto signal_domain = geopm::string_split(signal, "@");
            if (valid_signals.find(signal_domain[0]) == valid_signals.end()) {
                throw Exception("Invalid signal : " + signal_domain[0],
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            if (signal_domain.size() == 2) {
                result_data_structure.push_back(std::make_pair(
                    signal_domain[0],
                    geopm::PlatformTopo::domain_name_to_type(signal_domain[1])
                ));
            }
            else if (signal_domain.size() == 1) {
                result_data_structure.push_back(std::make_pair(signal_domain[0], GEOPM_DOMAIN_BOARD));
            }
            else {
                throw Exception("EnvironmentImp::signal_parser(): Environment trace extension contains signals with multiple \"@\" characters.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }

        return result_data_structure;
    }
}

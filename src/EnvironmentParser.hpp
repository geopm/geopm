/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENVIRONMENTPARSER_HPP_INCLUDE
#define ENVIRONMENTPARSER_HPP_INCLUDE

#include <string>
#include <vector>
#include <set>

namespace geopm
{
    /// @brief EnvironmentParser class parses signal lists from the
    /// GEOPM_REPORT_SIGNALS and GEOPM_TRACE_SIGNALS environment
    /// variables.
    std::vector<std::pair<std::string, int> > environment_signal_parser(const std::set<std::string> &valid_signals,
                                                                        const std::string &environment_variable_contents);
}
#endif

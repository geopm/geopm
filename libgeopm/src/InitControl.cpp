/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "InitControl.hpp"

#include <iostream>
#include <stdexcept>
#include <regex>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"

#include "geopm/PlatformIOProf.hpp"

namespace geopm
{
    std::unique_ptr<InitControl> InitControl::make_unique(void)
    {
        return geopm::make_unique<InitControlImp>();
    }

    InitControlImp::InitControlImp()
        : InitControlImp(PlatformIOProf::platform_io())
    {

    }

    InitControlImp::InitControlImp(PlatformIO &platform_io)
        : m_platform_io(platform_io)
    {

    }

    void InitControlImp::parse_input(const std::string &input_file)
    {
        std::string file_data = read_file(input_file);
        std::vector<std::string> lines = string_split(file_data, "\n");

        // (      Start capture group
        // [+-]?  Optional + or -
        // [0-9]* Zero or more digits
        // [.]?   Optional .
        // [0-9]+ One or more digits
        // (?:    Begin non-capture group
        // [eE]?  Optional e or E
        // [+-]?  Optional + or -
        // [0-9]+ One or more digits
        // )?     End non-capture group, optional
        // )      End capture group
        std::regex sci_notation_regex("([+-]?[0-9]*[.]?[0-9]+(?:[eE]?[+-]?[0-9]+)?)");

        // (            Start capture group
        // 0            Mandatory start character
        // [xX]         Allow "0x" or "0X" only
        // [0-9a-fA-F]+ One or more digits or hex letters
        // )            End capture group
        std::regex hex_regex("(0[xX][0-9a-fA-F]+)");

        // ^\\s*  String begins with zero or more whitespace characters
        // (\\S+) Capture group #1 for one or more non-whitespace characters (CONTROL NAME)
        // \\s+   One or more whitespace characters
        // (\\w+) Capture group #2 for one or more alphanumeric characters (DOMAIN NAME)
        // \\s+   One or more whitespace characters
        // (\\d+) Capture group #3 for one or more digits (DOMAIN INDEX)
        // \\s+   One or more whitespace characters
        // (\\S+) Capture group #4 for one or more non-whitespace characters (SETTING)
        // \\s*   Zero or more whitespace characters
        std::regex request_regex("^\\s*(\\S+)\\s+(\\w+)\\s+(\\d+)\\s+(\\S+)\\s*");
        std::smatch match_line;

        for (const auto &line : lines) {
            // Line parsing is done in 2 phases:
            //   1. Parse the entire line into match_line with capture groups defined in request_regex.
            //   2. Parse the SETTING value into match_setting with the capture group defined
            //      in hex_regex, then if no match sci_notation_regex.  Continue to parse it into
            //      a double if it is valid.
            std::regex_search(line, match_line, request_regex);

            if (match_line.empty()) { // No match; possible bad input or comment
                auto comment_pos = line.find_first_not_of(" \t");
                if (comment_pos == std::string::npos) {
                    continue;
                }
                else if (line[comment_pos] != '#') {
                    throw Exception("Invalid line comment or missing fields while parsing: " + line,
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            else { // Match; Continue to try parsing the setting
                if (string_begins_with(match_line[1].str(), "#")) {
                    continue;
                }
                if (match_line.suffix().str().size() > 0 && !string_begins_with(match_line.suffix(), "#")) {
                    throw Exception("Syntax error: " + line,
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }

                std::smatch match_setting;
                std::string setting(match_line[4].str());
                std::regex_search(setting, match_setting, hex_regex);

                if (match_setting.empty() || match_setting.suffix().str().size() > 0) {
                    std::regex_search(setting, match_setting, sci_notation_regex);
                    if (match_setting.empty()) {
                        throw Exception("Missing setting value while parsing: " + line,
                                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }

                    if (match_setting.suffix().str().size() > 0) {
                        throw Exception("Improperly formatted setting value encountered while parsing: " + line +
                                        " bad input: " + match_setting.suffix().str(),
                                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }
                }

                try {
                    // Expected line format:
                    //         1           2            3         4
                    //   CONTROL_NAME DOMAIN_NAME DOMAIN_INDEX SETTING
                    m_requests.push_back({match_line[1],
                                          PlatformTopo::domain_name_to_type(match_line[2]),
                                          std::stoi(match_line[3]),
                                          std::stod(match_setting[1])});
                }
                catch (const std::invalid_argument &ex) {
                    throw Exception("Invalid domain index: " + std::string(ex.what())
                                    + ": parsing: " + line,
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }
#ifdef GEOPM_DEBUG
        if (m_requests.empty()) {
            std::cerr << "Warning: <geopm> InitControl: No controls present in input file." << std::endl;
        }
#endif
    }

    void InitControlImp::write_controls(void) const
    {
        for (const auto &[name, domain, index, setting] : m_requests) {
#ifdef GEOPM_DEBUG
            std::cout << "Info: <geopm> InitControl: Setting " << name << " "
                      << domain << " " << index << " " << setting << std::endl;
#endif
            m_platform_io.write_control(name, domain, index, setting);
        }
    }
}

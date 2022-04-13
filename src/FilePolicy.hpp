/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FILEPOLICY_HPP_INCLUDE
#define FILEPOLICY_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>

namespace geopm
{
    class FilePolicy
    {
        public:
            FilePolicy() = delete;
            FilePolicy(const FilePolicy &other) = delete;
            FilePolicy(const std::string &policy_path,
                       const std::vector<std::string> &policy_names);
            virtual ~FilePolicy() = default;
            /// @brief Read policy values from a JSON file.
            /// @param [in] policy_path Location of the policy JSON file.
            /// @param [in] policy_names Expected policy field names
            ///             as determined by the Agent.
            /// @return The policy values read.
            std::vector<double> get_policy(void);
        private:
            std::map<std::string, double> parse_json(const std::string &path);
            std::vector<double> m_policy;
            const std::string m_policy_path;
            const std::vector<std::string> m_policy_names;
    };
}

#endif

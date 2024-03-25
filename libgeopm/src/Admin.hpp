/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADMIN_HPP_INCLUDE
#define ADMIN_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace geopm
{
    class OptionParser;
    class Admin
    {
        public:
            Admin();
            Admin(const std::string &default_config_path,
                  const std::string &override_config_path);
            void main(int argc,
                      const char **argv,
                      std::ostream &std_out,
                      std::ostream &std_err);
            std::string run(bool do_default,
                            bool do_override);
            OptionParser parser(std::ostream &std_out,
                                std::ostream &std_err);
            std::string default_config(void);
            std::string override_config(void);
            std::string check_node(void);
            void check_config(const std::map<std::string, std::string> &config_map,
                              std::vector<std::string> &policy_names,
                              std::vector<double> &policy_vals);
            std::string print_config(const std::map<std::string, std::string> &config_map,
                                     const std::map<std::string, std::string> &override_map,
                                     const std::vector<std::string> &policy_names,
                                     const std::vector<double> &policy_vals);
            static std::vector<std::string> dup_keys(const std::map<std::string, std::string> &map_a,
                                                     const std::map<std::string, std::string> &map_b);
        private:
            std::string m_default_config_path;
            std::string m_override_config_path;
    };
}

#endif

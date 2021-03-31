/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
                  const std::string &override_config_path,
                  int cpuid_local);
            void main(int argc,
                      const char **argv,
                      std::ostream &std_out,
                      std::ostream &std_err);
            std::string run(bool do_default,
                            bool do_override,
                            bool do_allowlist,
                            int cpuid);
            OptionParser parser(std::ostream &std_out,
                                std::ostream &std_err);
            std::string default_config(void);
            std::string override_config(void);
            std::string allowlist(int cpuid);
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
            int m_cpuid_local;
    };
}

#endif

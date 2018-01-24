/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include <signal.h>
#include <map>
#include <sstream>
#include <string>

#include "geopm_sched.h"
#include "PlatformTopo.hpp"
#include "Exception.hpp"

namespace geopm
{
    IPlatformTopo &platform_topo(void)
    {
        static PlatformTopo instance;
        return instance;
    }

    PlatformTopo::PlatformTopo()
        : PlatformTopo("")
    {

    }

    PlatformTopo::PlatformTopo(const std::string &lscpu_file_name)
        : m_lscpu_file_name(lscpu_file_name)
    {
        std::map<std::string, std::string> lscpu_map;
        lscpu(lscpu_map);
        parse_lscpu(lscpu_map, m_num_package, m_core_per_package, m_thread_per_core);
        parse_lscpu_numa(lscpu_map, m_numa_map);
    }

    PlatformTopo::~PlatformTopo()
    {

    }

    int PlatformTopo::num_domain(int domain_type) const
    {
        int result = 0;
        switch (domain_type) {
            case M_DOMAIN_BOARD:
                result = 1;
                break;
            case M_DOMAIN_PACKAGE:
                result = m_num_package;
                break;
            case M_DOMAIN_CORE:
                result = m_num_package * m_core_per_package;
                break;
            case M_DOMAIN_CPU:
                result = m_num_package * m_core_per_package * m_thread_per_core;
                break;
            case M_DOMAIN_BOARD_MEMORY:
                for (const auto &it : m_numa_map) {
                    if (it.size()) {
                        ++result;
                    }
                }
                break;
            case M_DOMAIN_PACKAGE_MEMORY:
                for (const auto &it : m_numa_map) {
                    if (!it.size()) {
                        ++result;
                    }
                }
                break;
            case M_DOMAIN_BOARD_NIC:
            case M_DOMAIN_PACKAGE_NIC:
            case M_DOMAIN_BOARD_ACCELERATOR:
            case M_DOMAIN_PACKAGE_ACCELERATOR:
                throw Exception("PlatformTopo::num_domain() no support yet for NIC or ACCELERATOR",
                                GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                break;
            case M_DOMAIN_INVALID:
            default:
                break;
        }
        return result;
    }

    void PlatformTopo::domain_cpus(int domain_type,
                                   int domain_idx,
                                   std::set<int> &cpu_idx) const
    {
        throw Exception("PlatformTopo::domain_cpus(): method not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int PlatformTopo::define_cpu_group(const std::vector<int> &cpu_domain_idx)
    {
        throw Exception("PlatformTopo::define_cpu_group(): method not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int PlatformTopo::domain_idx(int domain_type,
                                 int cpu_idx) const
    {
        int result = -1;
        int num_cpu = num_domain(M_DOMAIN_CPU);
        if (cpu_idx >= 0 && cpu_idx < num_cpu) {
            switch (domain_type) {
                case M_DOMAIN_BOARD:
                    result = 0;
                    break;
                case M_DOMAIN_PACKAGE:
                {
                    int core_idx = cpu_idx % (m_num_package * m_core_per_package);
                    result = core_idx / m_core_per_package;
                }
                    break;
                case M_DOMAIN_CORE:
                    result = cpu_idx % (m_num_package * m_core_per_package);
                    break;
                case M_DOMAIN_CPU:
                    result = cpu_idx;
                    break;
                case M_DOMAIN_BOARD_MEMORY:
                {
                    int numa_idx = 0;
                    for (const auto &set_it : m_numa_map) {
                        for (const auto &cpu_it : set_it) {
                            if (cpu_it == cpu_idx) {
                                result = numa_idx;
                                break;
                            }
                        }
                        if (result != -1) {
                            break;
                        }
                        ++numa_idx;
                    }
                }
                    break;
                case M_DOMAIN_PACKAGE_MEMORY:
                case M_DOMAIN_BOARD_NIC:
                case M_DOMAIN_PACKAGE_NIC:
                case M_DOMAIN_BOARD_ACCELERATOR:
                case M_DOMAIN_PACKAGE_ACCELERATOR:
                    throw Exception("PlatformTopo::domain_idx() no support yet for PACKAGE_MEMORY, NIC, or ACCELERATOR",
                                    GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                    break;
                case M_DOMAIN_INVALID:
                default:
                    break;
            }
        }
        return result;
    }



    void PlatformTopo::parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
                                   int &num_package,
                                   int &core_per_package,
                                   int &thread_per_core)
    {
        const std::string keys[5] = {"CPU(s)",
                                     "Thread(s) per core",
                                     "Core(s) per socket",
                                     "Socket(s)",
                                     "NUMA node(s)"};
        int values[5] = {};
        const int num_values = sizeof(values) / sizeof(values[0]);

        for (int i = 0; i < num_values; ++i) {
            auto it = lscpu_map.find(keys[i]);
            if (it == lscpu_map.end()) {
                throw Exception("PlatformTopo: parsing lscpu output, key not found",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            values[i] = atoi(it->second.c_str());
            if (!values[i]) {
                throw Exception("PlatformTopo: parsing lscpu output, value not converted",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        num_package = values[3];
        int num_core = values[2] * num_package;
        core_per_package = num_core / num_package;
        thread_per_core = values[1];
        if (num_package * core_per_package * thread_per_core != values[0]) {
            throw Exception("PlatformTopo: parsing lscpu output, inconsistant values or come CPUs are not online",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void PlatformTopo::parse_lscpu_numa(std::map<std::string, std::string> lscpu_map,
                                        std::vector<std::set<int> > &numa_map)
    {
        bool is_node_found = true;
        for (int node_idx = 0; is_node_found; ++node_idx) {
            std::ostringstream numa_key;
            numa_key << "NUMA node" << node_idx << " CPU(s)";
            auto lscpu_it = lscpu_map.find(numa_key.str());
            if (lscpu_it == lscpu_map.end()) {
                is_node_found = false;
            }
            else {
                numa_map.push_back({});
                auto cpu_set_it = numa_map.end() - 1;
                if (lscpu_it->second.size() < 2 ||
                    lscpu_it->second.substr(0,2) != "0x") {
                    throw Exception("PlatformTopo: parsing lscpu output, numa mask does not begin with 0x",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                std::string hex_mask = lscpu_it->second.substr(2);
                int cpu_idx = 0;
                for (auto hmc = hex_mask.rbegin(); hmc != hex_mask.rend(); ++hmc) {
                    std::stringstream ss;
                    uint32_t hmb;
                    ss << std::hex << *hmc;
                    ss >> hmb;
                    for (int bit_idx = 0; bit_idx != 4; ++bit_idx) {
                        if (hmb & 1U) {
                            cpu_set_it->insert(cpu_idx);
                        }
                        hmb = hmb >> 1;
                        ++cpu_idx;
                    }
                }
            }
        }
    }

    FILE *PlatformTopo::open_lscpu(void)
    {
        FILE *result = nullptr;
        if (m_lscpu_file_name.size()) {
            result = fopen(m_lscpu_file_name.c_str(), "r");
            if (!result) {
                throw Exception("PlatformTopo::open_lscpu(): Could not open lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        else {
            int err = geopm_sched_popen("lscpu -x", &result);
            if (err) {
                throw Exception("PlatformTopo::open_lscpu(): Could not popen lscpu command",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        return result;
    }

    void PlatformTopo::close_lscpu(FILE *fid)
    {
        if (m_lscpu_file_name.size()) {
            int err = fclose(fid);
            if (err) {
                throw Exception("PlatformTopo::close_lscpu(): Could not fclose lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        else {
            int err = pclose(fid);
            if (err) {
                throw Exception("PlatformTopo::close_lscpu(): Could not fclose lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
    }

    void PlatformTopo::lscpu(std::map<std::string, std::string> &lscpu_map)
    {
        std::string result;

        FILE *fid = open_lscpu();

        std::string line;
        while (!feof(fid)) {
            char cline[1024] = {};
            if (fgets(cline, 1024, fid)) {
                line = cline;
                size_t colon_pos = line.find(":");
                if (colon_pos != std::string::npos) {
                    std::string key(line.substr(0, colon_pos));
                    std::string value(line.substr(colon_pos + 1));
                    size_t ws_pos = value.find_first_not_of(" \t");
                    if (ws_pos &&
                        ws_pos < value.size() - 1 &&
                        ws_pos != std::string::npos) {
                        // Trim leading white space and '\n' from end of line
                        value = value.substr(ws_pos, value.size() - ws_pos - 1);
                    }
                    if (key.size()) {
                        lscpu_map.emplace(key, value);
                    }
                }
            }
        }
    }
}

/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#include <cpuid.h>

#include "geopm_sched.h"
#include "PlatformTopo.hpp"
#include "Exception.hpp"

#include "config.h"

int geopm_read_cpuid(void)
{
    uint32_t key = 1; //processor features
    uint32_t proc_info = 0;
    uint32_t model;
    uint32_t family;
    uint32_t ext_model;
    uint32_t ext_family;
    uint32_t ebx, ecx, edx;
    const uint32_t model_mask = 0xF0;
    const uint32_t family_mask = 0xF00;
    const uint32_t extended_model_mask = 0xF0000;
    const uint32_t extended_family_mask = 0xFF00000;

    __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

    model = (proc_info & model_mask) >> 4;
    family = (proc_info & family_mask) >> 8;
    ext_model = (proc_info & extended_model_mask) >> 16;
    ext_family = (proc_info & extended_family_mask) >> 20;

    if (family == 6) {
        model+=(ext_model << 4);
    }
    else if (family == 15) {
        model+=(ext_model << 4);
        family+=ext_family;
    }

    return ((family << 8) + model);
}

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
        : M_LSCPU_FILE_NAME("/tmp/geopm-lscpu.log")
        , M_TEST_LSCPU_FILE_NAME(lscpu_file_name)
        , m_do_fclose(true)
    {
        std::map<std::string, std::string> lscpu_map;
        lscpu(lscpu_map);
        parse_lscpu(lscpu_map, m_num_package, m_core_per_package, m_thread_per_core);
        parse_lscpu_numa(lscpu_map, m_numa_map);
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
                /// @todo Add support for NIC and accelerators to PlatformTopo.
                result = 0;
                break;
            case M_DOMAIN_INVALID:
                throw Exception("PlatformTopo::num_domain(): invalid domain specified",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
            default:
                throw Exception("PlatformTopo::num_domain(): invalid domain specified",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        return result;
    }

    std::set<int> PlatformTopo::domain_cpus(int domain_type,
                                            int domain_idx) const
    {
        std::set<int> cpu_idx;
        cpu_idx.clear();
        switch (domain_type) {
            case M_DOMAIN_BOARD:
                for (auto numa_cpus : m_numa_map) {
                    cpu_idx.insert(numa_cpus.begin(), numa_cpus.end());
                }
                break;
            case M_DOMAIN_PACKAGE:
                for (int thread_idx = 0;
                     thread_idx != m_thread_per_core;
                     ++thread_idx) {
                    for (int core_idx = domain_idx * m_core_per_package;
                         core_idx != (domain_idx + 1) * m_core_per_package;
                         ++core_idx) {
                        cpu_idx.insert(core_idx + thread_idx * m_core_per_package * m_num_package);
                    }
                }
                break;
            case M_DOMAIN_CORE:
                for (int thread_idx = 0;
                     thread_idx != m_thread_per_core;
                     ++thread_idx) {
                    cpu_idx.insert(domain_idx + thread_idx * m_core_per_package * m_num_package);
                }
                break;
            case M_DOMAIN_CPU:
                cpu_idx.insert(domain_idx);
                break;
            case M_DOMAIN_BOARD_MEMORY:
                cpu_idx = m_numa_map[domain_idx];
                break;
            default:
                throw Exception("PlatformTopo::domain_cpus(domain_type=" +
                                std::to_string(domain_type) +
                                ") support not yet implemented",
                                GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                break;
        }
        return cpu_idx;
    }

    int PlatformTopo::define_cpu_group(const std::vector<int> &cpu_domain_idx)
    {
        /// @todo Add support for define_cpu_group() method
        throw Exception("PlatformTopo::define_cpu_group(): method not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int PlatformTopo::domain_idx(int domain_type,
                                 int cpu_idx) const
    {
        int result = -1;
        int num_cpu = num_domain(M_DOMAIN_CPU);
        int core_idx = 0;
        int numa_idx = 0;
        if (cpu_idx >= 0 && cpu_idx < num_cpu) {
            switch (domain_type) {
                case M_DOMAIN_BOARD:
                    result = 0;
                    break;
                case M_DOMAIN_PACKAGE:
                    core_idx = cpu_idx % (m_num_package * m_core_per_package);
                    result = core_idx / m_core_per_package;
                    break;
                case M_DOMAIN_CORE:
                    result = cpu_idx % (m_num_package * m_core_per_package);
                    break;
                case M_DOMAIN_CPU:
                    result = cpu_idx;
                    break;
                case M_DOMAIN_BOARD_MEMORY:
                    numa_idx = 0;
                    for (const auto &set_it : m_numa_map) {
                        for (const auto &cpu_it : set_it) {
                            if (cpu_it == cpu_idx) {
                                result = numa_idx;
                                // Find the lowest index numa node that contains the cpu.
                                break;
                            }
                        }
                        if (result != -1) {
                            // Find the lowest index numa node that contains the cpu.
                            break;
                        }
                        ++numa_idx;
                    }
                    break;
                case M_DOMAIN_PACKAGE_MEMORY:
                case M_DOMAIN_BOARD_NIC:
                case M_DOMAIN_PACKAGE_NIC:
                case M_DOMAIN_BOARD_ACCELERATOR:
                case M_DOMAIN_PACKAGE_ACCELERATOR:
                    /// @todo Add support for package memory NIC and accelerators to domain_idx() method.
                    throw Exception("PlatformTopo::domain_idx() no support yet for PACKAGE_MEMORY, NIC, or ACCELERATOR",
                                    GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                    break;
                case M_DOMAIN_INVALID:
                default:
                    throw Exception("PlatformTopo::domain_idx() invalid domain specified",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    break;
            }
        }
        else {
            throw Exception("PlatformTopo::domain_idx() cpu index (cpu_idx) out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    bool PlatformTopo::is_domain_within(int inner_domain, int outer_domain) const
    {
        bool result = false;
        static const std::set<int> package_domain = {
            M_DOMAIN_CPU,
            M_DOMAIN_CORE,
            M_DOMAIN_PACKAGE_MEMORY,
            M_DOMAIN_PACKAGE_NIC,
            M_DOMAIN_PACKAGE_ACCELERATOR,
        };
        if (inner_domain == outer_domain) {
            result = true;
        }
        else if (outer_domain == M_DOMAIN_BOARD) {
            // All domains are within the board domain
            result = true;
        }
        else if (outer_domain == M_DOMAIN_CORE &&
                 inner_domain == M_DOMAIN_CPU) {
            // Only the CPU domain is within the core.
            result = true;
        }
        else if (outer_domain == M_DOMAIN_PACKAGE &&
                 package_domain.find(inner_domain) != package_domain.end()) {
            // Everything under the package scope is in the package_domain set.
            result = true;
        }
        else if (outer_domain == M_DOMAIN_BOARD_MEMORY &&
                 inner_domain == M_DOMAIN_CPU) {
            // To support mapping CPU signals to DRAM domain (e.g. power)
            result = true;
        }
        return result;
    }

    std::vector<std::string> IPlatformTopo::domain_names(void)
    {
        return {
            "invalid",
            "board",
            "package",
            "core",
            "cpu",
            "board_memory",
            "package_memory",
            "board_nic",
            "package_nic",
            "board_accelerator",
            "package_accelerator",
        };
    }

    std::map<std::string, int> IPlatformTopo::domain_types(void)
    {
        std::map<std::string, int> result;
        int domain_type = M_DOMAIN_INVALID;
        auto names = domain_names();
        for (const auto &name : names) {
            result[name] = domain_type;
            ++domain_type;
        }
#ifdef GEOPM_DEBUG
        if (names.size() != M_NUM_DOMAIN) {
            throw Exception("IPlatformTopo::m_domain_name has incorrect size.  "
                            "Domains must match m_domain_e in number and order.",
                            GEOPM_ERROR_LOGIC);
        }
        if (result.size() != M_NUM_DOMAIN) {
            throw Exception("IPlatformTopo::m_domain_type has incorrect size.  "
                            "Domain type mapping must match m_domain_name.",
                            GEOPM_ERROR_LOGIC);
        }
#endif
        return result;
    }

    std::string IPlatformTopo::domain_type_to_name(int domain_type)
    {
        if (domain_type <= M_DOMAIN_INVALID || domain_type > M_NUM_DOMAIN) {
            throw Exception("PlatformTopo::domain_type_to_name(): unrecognized domain_type: " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return domain_names()[domain_type];
    }

    int IPlatformTopo::domain_name_to_type(const std::string &domain_name)
    {
        auto m_domain_type = domain_types();
        auto it = m_domain_type.find(domain_name);
        if (it == m_domain_type.end()) {
            throw Exception("PlatformTopo::domain_name_to_type(): unrecognized domain_name: " + domain_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    void PlatformTopo::parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
                                   int &num_package,
                                   int &core_per_package,
                                   int &thread_per_core)
    {
        const std::string keys[6] = {"CPU(s)",
                                     "Thread(s) per core",
                                     "Core(s) per socket",
                                     "Socket(s)",
                                     "NUMA node(s)",
                                     "On-line CPU(s) mask"};
        std::vector<std::string> values(6);

        for (size_t i = 0; i < values.size(); ++i) {
            auto it = lscpu_map.find(keys[i]);
            if (it == lscpu_map.end()) {
                throw Exception("PlatformTopo: parsing lscpu output, key not found: \"" + keys[i] + "\"",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            values[i] = it->second;
            if (values[i].size() == 0) {
                throw Exception("PlatformTopo: parsing lscpu output, value not recorded: " + it->second,
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        num_package = atoi(values[3].c_str());
        int num_core = atoi(values[2].c_str()) * num_package;
        core_per_package = num_core / num_package;
        thread_per_core = atoi(values[1].c_str());

        int total_cores_expected_online = num_package * core_per_package * thread_per_core;
        if (total_cores_expected_online != atoi(values[0].c_str())) {
            // Check how many CPUs are actually online
            std::string online_cpu_mask = values[5];

            if (online_cpu_mask.size() < 2 ||
                online_cpu_mask.substr(0,2) != "0x") {
                throw Exception("PlatformTopo: parsing lscpu output, online CPU mask does not begin with 0x",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            online_cpu_mask = online_cpu_mask.substr(2);
            int online_cpus = 0;
            for (auto hmc = online_cpu_mask.rbegin(); hmc != online_cpu_mask.rend(); ++hmc) {
                uint32_t hmb = std::stoul(std::string(1, *hmc), 0, 16);
                for (int bit_idx = 0; bit_idx != 4; ++bit_idx) {
                    if (hmb & 1U) {
                        ++online_cpus;
                    }
                    hmb = hmb >> 1;
                }
            }
            if (total_cores_expected_online != online_cpus) {
                throw Exception("PlatformTopo: parsing lscpu output, inconsistent values or unable to determine online CPUs",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void PlatformTopo::parse_lscpu_numa(std::map<std::string, std::string> lscpu_map,
                                        std::vector<std::set<int> > &numa_map)
    {
        // TODO: what to do if there are no numa node lines?
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
                    uint32_t hmb = std::stoul(std::string(1, *hmc), 0, 16);
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
        if (M_TEST_LSCPU_FILE_NAME.size()) {
            result = fopen(M_TEST_LSCPU_FILE_NAME.c_str(), "r");
            if (!result) {
                throw Exception("PlatformTopo::open_lscpu(): Could not open test lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        else {
            result = fopen(M_LSCPU_FILE_NAME.c_str(), "r");
            if (!result) {
                int err = geopm_sched_popen("lscpu -x", &result);
                if (err) {
                    throw Exception("PlatformTopo::open_lscpu(): Could not popen lscpu command",
                                    errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                m_do_fclose = false;
            }
        }
        return result;
    }

    void PlatformTopo::close_lscpu(FILE *fid)
    {
        if (m_do_fclose) {
            int err = fclose(fid);
            if (err) {
                throw Exception("PlatformTopo::close_lscpu(): Could not fclose lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        else {
            int err = pclose(fid);
            if (err) {
                throw Exception("PlatformTopo::close_lscpu(): Could not pclose lscpu file",
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
        close_lscpu(fid);
    }
}

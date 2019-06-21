/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "PlatformTopoImp.hpp"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cpuid.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>

#include "geopm_sched.h"
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
    const std::string PlatformTopoImp::M_CACHE_FILE_NAME = "/tmp/geopm-topo-cache";

    PlatformTopoImp &platform_topo_internal(void)
    {
        static PlatformTopoImp instance;
        return instance;
    }

    const PlatformTopo &platform_topo(void)
    {
        return platform_topo_internal();
    }

    PlatformTopoImp::PlatformTopoImp()
        : PlatformTopoImp("")
    {

    }

    PlatformTopoImp::PlatformTopoImp(const std::string &test_cache_file_name)
        : M_TEST_CACHE_FILE_NAME(test_cache_file_name)
        , m_do_fclose(true)
        , m_is_domain_within(GEOPM_NUM_DOMAIN, std::vector<bool> {})
        , m_cpus_domains(GEOPM_NUM_DOMAIN, std::vector<std::set<int> > {})
        , m_rank_map_set(false)
    {
        std::map<std::string, std::string> lscpu_map;
        lscpu(lscpu_map);
        parse_lscpu(lscpu_map, m_num_package, m_core_per_package, m_thread_per_core);
        parse_lscpu_numa(lscpu_map, m_numa_map);

        for (int domain = GEOPM_DOMAIN_BOARD; domain < GEOPM_DOMAIN_MPI_RANK; ++domain) {
            std::vector<std::set<int> > cpus_domain;
            for (int domain_idx = 0; domain_idx < num_domain(domain); ++domain_idx) {
                cpus_domain.push_back(domain_cpus(domain, domain_idx));
            }
            m_cpus_domains[domain] = cpus_domain;
        }

        for (int inner = GEOPM_DOMAIN_BOARD; inner < GEOPM_DOMAIN_MPI_RANK; ++inner) {
            for (int outer = GEOPM_DOMAIN_BOARD; outer < GEOPM_DOMAIN_MPI_RANK; ++outer) {
                m_is_domain_within[inner][outer] = is_domain_within(inner, outer);
            }
        }
    }

    int PlatformTopoImp::get_outer_domain_idx(int inner_domain, int inner_domain_idx, int outer_domain) const
    {
        int result = -1;
        std::set<int> inner_domain_cpus = domain_nested(GEOPM_DOMAIN_CPU, inner_domain, inner_domain_idx);
        if (inner_domain_cpus.size()) {
            int target_cpu = *inner_domain_cpus.begin();
            int num_outer_domain = num_domain(outer_domain);
            for (int outer_domain_idx = 0; outer_domain_idx < num_outer_domain; ++outer_domain_idx) {
                std::set<int> curr_outer_domain_idx_cpus = domain_nested(inner_domain,
                                                                         outer_domain,
                                                                         outer_domain_idx);
                if (curr_outer_domain_idx_cpus.find(target_cpu) != curr_outer_domain_idx_cpus.end()) {
                    result = outer_domain_idx;
                }
            }
        }
        return result;
    }

    bool PlatformTopoImp::is_domain_within(int inner, int outer) const
    {
        std::vector<std::set<int> > inner_cpu_domains = m_cpus_domains[inner];
        std::vector<std::set<int> > outer_cpu_domains = m_cpus_domains[outer];
        bool is_domain_within = true;

        if (inner_cpu_domains.size() == 0 ||
            outer_cpu_domains.size() == 0) {
            is_domain_within = false;
        }
        else if (outer != GEOPM_DOMAIN_BOARD && // everything is on the board
                 inner != outer) {
            for (const auto &inner_cpu_set : inner_cpu_domains) {
                bool is_isec_found = false;
                for (const auto &outer_cpu_set : outer_cpu_domains) {
                    if (is_domain_within) {
                        std::set<int> isec;
                        std::set<int> idiff;
                        std::set<int> odiff;

                        std::set_intersection(inner_cpu_set.begin(), inner_cpu_set.end(),
                                              outer_cpu_set.begin(), outer_cpu_set.end(),
                                              std::inserter(isec, isec.begin()));
                        std::set_difference(inner_cpu_set.begin(), inner_cpu_set.end(),
                                            outer_cpu_set.begin(), outer_cpu_set.end(),
                                            std::inserter(idiff, idiff.begin()));
                        std::set_difference(outer_cpu_set.begin(), outer_cpu_set.end(),
                                            inner_cpu_set.begin(), inner_cpu_set.end(),
                                            std::inserter(odiff, odiff.begin()));
                        // inner_cpu_set intersection outter_cpu_set != {}
                        // AND
                        // inner_cpu_set is a super set of outer_cpu_set
                        if (isec.size() != 0 &&
                            odiff.size() == 0) {
                            is_domain_within = false;
                        }
                        // inner_cpu_set intersection outter_cpu_set != {}
                        if (isec.size() != 0) {
                            is_isec_found = true;
                        }
                    }
                }
                // no combination of inner_cpu_set:outter_cpu_set intersect
                if (!is_isec_found) {
                    is_domain_within = false;
                }
            }
        }
        return is_domain_within;
    }

    void PlatformTopoImp::define_cpu_mpi_rank_map(const std::vector<int> &cpu_domain_idx)
    {
        if (m_rank_map_set) {
            throw Exception("PlatformTopoImp::define_cpu_mpi_rank_map(): called multiple times",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        else if (cpu_domain_idx.size() < (size_t) num_domain(GEOPM_DOMAIN_CPU)) {
            throw Exception("PlatformTopoImp::define_cpu_mpi_rank_map(): cpu_domain_idx incorrectly sized",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        {
            // it is important that these assignments happen together
            // and before the code below as the flag update below will
            // prevent pre-init Exception.
            m_cpu_rank = cpu_domain_idx;
            m_rank_map_set = true;
        }

        std::vector< std::set<int> > rank_domain_cpus;
        for (int rank = 0; rank < num_domain(GEOPM_DOMAIN_MPI_RANK); ++rank) {
            rank_domain_cpus.push_back(domain_cpus(GEOPM_DOMAIN_MPI_RANK, rank));
        }
        m_cpus_domains[GEOPM_DOMAIN_MPI_RANK] = rank_domain_cpus;
        for (int outer = GEOPM_DOMAIN_BOARD; outer < GEOPM_NUM_DOMAIN; ++outer) {
            m_is_domain_within[GEOPM_DOMAIN_MPI_RANK][outer] = is_domain_within(GEOPM_DOMAIN_MPI_RANK, outer);
            m_is_domain_within[outer][GEOPM_DOMAIN_MPI_RANK] = is_domain_within(outer, GEOPM_DOMAIN_MPI_RANK);
        }
    }

    int PlatformTopoImp::num_mpi_rank() const
    {
        if (!m_rank_map_set) {
            throw Exception("PlatformTopoImp::num_mpi_rank(): called before rank map is defined",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::set<int> ranks;
        for (const auto &rank : m_cpu_rank) {
            ranks.insert(rank);
        }
        return ranks.size();
    }

    int PlatformTopoImp::num_domain(int domain_type) const
    {
        int result = 0;
        switch (domain_type) {
            case GEOPM_DOMAIN_BOARD:
                result = 1;
                break;
            case GEOPM_DOMAIN_PACKAGE:
                result = m_num_package;
                break;
            case GEOPM_DOMAIN_CORE:
                result = m_num_package * m_core_per_package;
                break;
            case GEOPM_DOMAIN_CPU:
                result = m_num_package * m_core_per_package * m_thread_per_core;
                break;
            case GEOPM_DOMAIN_BOARD_MEMORY:
                for (const auto &it : m_numa_map) {
                    if (it.size()) {
                        ++result;
                    }
                }
                break;
            case GEOPM_DOMAIN_PACKAGE_MEMORY:
                for (const auto &it : m_numa_map) {
                    if (!it.size()) {
                        ++result;
                    }
                }
                break;
            case GEOPM_DOMAIN_BOARD_NIC:
            case GEOPM_DOMAIN_PACKAGE_NIC:
            case GEOPM_DOMAIN_BOARD_ACCELERATOR:
            case GEOPM_DOMAIN_PACKAGE_ACCELERATOR:
                /// @todo Add support for NIC and accelerators to PlatformTopo.
                result = 0;
                break;
            case GEOPM_DOMAIN_MPI_RANK:
                result = num_mpi_rank();
                break;
            case GEOPM_DOMAIN_INVALID:
                throw Exception("PlatformTopoImp::num_domain(): invalid domain specified",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
            default:
                throw Exception("PlatformTopoImp::num_domain(): invalid domain specified",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        return result;
    }

    std::set<int> PlatformTopoImp::domain_cpus(int domain_type,
                                               int domain_idx) const
    {
        if (domain_type < 0 || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformTopoImp::domain_cpus(): domain_type out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int num_dom = num_domain(domain_type);
        if (domain_idx < 0 || domain_idx >= num_dom) {
            throw Exception("PlatformTopoImp::domain_cpus(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::set<int> cpu_idx;
        switch (domain_type) {
            case GEOPM_DOMAIN_BOARD:
                for (auto numa_cpus : m_numa_map) {
                    cpu_idx.insert(numa_cpus.begin(), numa_cpus.end());
                }
                break;
            case GEOPM_DOMAIN_PACKAGE:
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
            case GEOPM_DOMAIN_CORE:
                for (int thread_idx = 0;
                     thread_idx != m_thread_per_core;
                     ++thread_idx) {
                    cpu_idx.insert(domain_idx + thread_idx * m_core_per_package * m_num_package);
                }
                break;
            case GEOPM_DOMAIN_CPU:
                cpu_idx.insert(domain_idx);
                break;
            case GEOPM_DOMAIN_BOARD_MEMORY:
                cpu_idx = m_numa_map[domain_idx];
                break;
            case GEOPM_DOMAIN_MPI_RANK:
                for (size_t cpu = 0; cpu < m_cpu_rank.size(); ++cpu) {
                    if (domain_idx == m_cpu_rank[cpu]) {
                        cpu_idx.insert(cpu);
                    }
                }
                break;
        }
        return cpu_idx;
    }

    int PlatformTopoImp::domain_idx(int domain_type,
                                    int cpu_idx) const
    {
        int result = -1;
        int num_cpu = num_domain(GEOPM_DOMAIN_CPU);

        if (domain_type < 0 || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformTopoImp::domain_idx(): domain_type out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (cpu_idx < 0 || cpu_idx >= num_cpu) {
            throw Exception("PlatformTopoImp::domain_idx(): cpu_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int core_idx = 0;
        int numa_idx = 0;
        if (cpu_idx >= 0 && cpu_idx < num_cpu) {
            switch (domain_type) {
                case GEOPM_DOMAIN_BOARD:
                    result = 0;
                    break;
                case GEOPM_DOMAIN_PACKAGE:
                    core_idx = cpu_idx % (m_num_package * m_core_per_package);
                    result = core_idx / m_core_per_package;
                    break;
                case GEOPM_DOMAIN_CORE:
                    result = cpu_idx % (m_num_package * m_core_per_package);
                    break;
                case GEOPM_DOMAIN_CPU:
                    result = cpu_idx;
                    break;
                case GEOPM_DOMAIN_BOARD_MEMORY:
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
                case GEOPM_DOMAIN_PACKAGE_MEMORY:
                    break;
                case GEOPM_DOMAIN_BOARD_NIC:
                case GEOPM_DOMAIN_PACKAGE_NIC:
                case GEOPM_DOMAIN_BOARD_ACCELERATOR:
                case GEOPM_DOMAIN_PACKAGE_ACCELERATOR:
                    /// @todo Add support for package memory NIC and accelerators to domain_idx() method.
                    throw Exception("PlatformTopoImp::domain_idx() no support yet for PACKAGE_MEMORY, NIC, or ACCELERATOR",
                                    GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                    break;
                case GEOPM_DOMAIN_MPI_RANK:
                    if (!m_rank_map_set) {
                        throw Exception("PlatformTopoImp::domain_idx(): called before rank map is defined",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                    result = m_cpu_rank[cpu_idx];
                    break;
                case GEOPM_DOMAIN_INVALID:
                default:
                    throw Exception("PlatformTopoImp::domain_idx() invalid domain specified",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    break;
            }
        }
        else {
            throw Exception("PlatformTopoImp::domain_idx() cpu index (cpu_idx) out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    bool PlatformTopoImp::is_nested_domain(int inner_domain, int outer_domain) const
    {
        if (inner_domain == GEOPM_DOMAIN_MPI_RANK ||
            outer_domain == GEOPM_DOMAIN_MPI_RANK) {
            if (!m_rank_map_set) {
                throw Exception("PlatformTopoImp::is_nested_domain(): called before rank map is defined",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        return m_is_domain_within[inner_domain][outer_domain];
    }

    std::set<int> PlatformTopoImp::domain_nested(int inner_domain, int outer_domain, int outer_idx) const
    {
        if (!is_nested_domain(inner_domain, outer_domain)) {
            throw Exception("PlatformTopoImp::domain_nested(): domain type " + std::to_string(inner_domain) +
                            " is not contained within domain type " + std::to_string(outer_domain),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::set<int> inner_domain_idx;
        std::set<int> cpus = domain_cpus(outer_domain, outer_idx);
        for (auto cc : cpus) {
            inner_domain_idx.insert(domain_idx(inner_domain, cc));
        }
        return inner_domain_idx;
    }

    std::vector<std::string> PlatformTopo::domain_names(void)
    {
        std::vector<std::string> result(GEOPM_NUM_DOMAIN);
        for (const auto &it : domain_types()) {
            result.at(it.second) = it.first;
        }
        return result;
    }

    std::map<std::string, int> PlatformTopo::domain_types(void)
    {
        return {
            {"board", GEOPM_DOMAIN_BOARD},
            {"package", GEOPM_DOMAIN_PACKAGE},
            {"core", GEOPM_DOMAIN_CORE},
            {"cpu", GEOPM_DOMAIN_CPU},
            {"board_memory", GEOPM_DOMAIN_BOARD_MEMORY},
            {"package_memory", GEOPM_DOMAIN_PACKAGE_MEMORY},
            {"board_nic", GEOPM_DOMAIN_BOARD_NIC},
            {"package_nic", GEOPM_DOMAIN_PACKAGE_NIC},
            {"board_accelerator", GEOPM_DOMAIN_BOARD_ACCELERATOR},
            {"package_accelerator", GEOPM_DOMAIN_PACKAGE_ACCELERATOR},
            {"mpi_rank", GEOPM_DOMAIN_MPI_RANK}
        };
    }

    std::string PlatformTopo::domain_type_to_name(int domain_type)
    {
        if (domain_type <= GEOPM_DOMAIN_INVALID || domain_type >= GEOPM_NUM_DOMAIN) {
            throw Exception("PlatformTopo::domain_type_to_name(): unrecognized domain_type: " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return domain_names()[domain_type];
    }

    int PlatformTopo::domain_name_to_type(const std::string &domain_name)
    {
        auto m_domain_type = domain_types();
        auto it = m_domain_type.find(domain_name);
        if (it == m_domain_type.end()) {
            throw Exception("PlatformTopo::domain_name_to_type(): unrecognized domain_name: " + domain_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    void PlatformTopo::create_cache(void)
    {
        PlatformTopoImp::create_cache();
    }

    void PlatformTopoImp::create_cache(void)
    {
        PlatformTopoImp::create_cache(M_CACHE_FILE_NAME);
    }

    void PlatformTopoImp::create_cache(const std::string &cache_file_name)
    {
        // If cache file is not present, create it
        struct stat cache_stat;
        if (stat(cache_file_name.c_str(), &cache_stat)) {
            std::string cmd = "out=" + cache_file_name + ";"
                              "lscpu -x > $out && "
                              "chmod a+rw $out";
            FILE *pid;
            int err = geopm_sched_popen(cmd.c_str(), &pid);
            if (err) {
                unlink(cache_file_name.c_str());
                throw Exception("PlatformTopo::create_cache(): Could not popen lscpu command: ",
                         err, __FILE__, __LINE__);
            }
            if (pclose(pid)) {
                unlink(cache_file_name.c_str());
                throw Exception("PlatformTopo::create_cache(): Could not pclose lscpu command: ",
                         errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void PlatformTopoImp::parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
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
                throw Exception("PlatformTopoImp: parsing lscpu output, key not found: \"" + keys[i] + "\"",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            values[i] = it->second;
            if (values[i].size() == 0) {
                throw Exception("PlatformTopoImp: parsing lscpu output, value not recorded: " + it->second,
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
            if (online_cpu_mask.substr(0,2) == "0x") {
                online_cpu_mask = online_cpu_mask.substr(2);
            }
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
                throw Exception("PlatformTopoImp: parsing lscpu output, inconsistent values or unable to determine online CPUs",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void PlatformTopoImp::parse_lscpu_numa(std::map<std::string, std::string> lscpu_map,
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
                std::string hex_mask = lscpu_it->second;
                if (hex_mask.substr(0,2) == "0x") {
                    hex_mask = hex_mask.substr(2);
                }
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

    FILE *PlatformTopoImp::open_lscpu(void)
    {
        FILE *result = nullptr;
        if (M_TEST_CACHE_FILE_NAME.size()) {
            result = fopen(M_TEST_CACHE_FILE_NAME.c_str(), "r");
            if (!result) {
                throw Exception("PlatformTopoImp::open_lscpu(): Could not open test lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        else {
            result = fopen(M_CACHE_FILE_NAME.c_str(), "r");
            if (!result) {
                int err = geopm_sched_popen("lscpu -x", &result);
                if (err) {
                    throw Exception("PlatformTopoImp::open_lscpu(): Could not popen lscpu command",
                                    errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                m_do_fclose = false;
            }
        }
        return result;
    }

    void PlatformTopoImp::close_lscpu(FILE *fid)
    {
        if (m_do_fclose) {
            int err = fclose(fid);
            if (err) {
                throw Exception("PlatformTopoImp::close_lscpu(): Could not fclose lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        else {
            int err = pclose(fid);
            if (err) {
                throw Exception("PlatformTopoImp::close_lscpu(): Could not pclose lscpu file",
                                errno ? errno : GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
    }

    void PlatformTopoImp::lscpu(std::map<std::string, std::string> &lscpu_map)
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

extern "C"
{
    int geopm_topo_num_domain(int domain_type)
    {
        int result = 0;
        try {
            result = geopm::platform_topo().num_domain(domain_type);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_topo_domain_idx(int domain_type, int cpu_idx)
    {
        int result = 0;
        try {
            result = geopm::platform_topo().domain_idx(domain_type, cpu_idx);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_topo_num_domain_nested(int inner_domain, int outer_domain)
    {
        int result = GEOPM_ERROR_INVALID;
        try {
            if (geopm::platform_topo().is_nested_domain(inner_domain, outer_domain)) {
                int num_inner_domain = geopm::platform_topo().num_domain(inner_domain);
                int num_outer_domain = geopm::platform_topo().num_domain(outer_domain);
                if (num_outer_domain > 0 && num_inner_domain > 0) {
                    result = num_inner_domain / num_outer_domain;
                }
            }
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_topo_domain_nested(int inner_domain, int outer_domain, int outer_idx,
                                 size_t num_domain_nested, int *domain_nested)
    {
        int err = 0;
        try {
            int num_domain_nested_ref = geopm_topo_num_domain_nested(inner_domain, outer_domain);
            if (num_domain_nested_ref > 0 &&
                num_domain_nested == (size_t)num_domain_nested_ref) {
                std::set<int> nested_set(geopm::platform_topo().domain_nested(inner_domain, outer_domain, outer_idx));
                if (nested_set.size() == (size_t)num_domain_nested) {
                    int idx = 0;
                    for (const auto &domain : nested_set) {
                        domain_nested[idx] = domain;
                        ++idx;
                    }
                }
                else {
                    err = GEOPM_ERROR_RUNTIME;
                }
            }
            else {
                err = num_domain_nested_ref;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_topo_domain_name(int domain_type, size_t domain_name_max,
                               char *domain_name)
    {
        int err = 0;
        try {
            std::string domain_name_string = geopm::platform_topo().domain_type_to_name(domain_type);
            domain_name[domain_name_max - 1] = '\0';
            strncpy(domain_name, domain_name_string.c_str(), domain_name_max);
            if (domain_name[domain_name_max - 1] != '\0') {
                domain_name[domain_name_max - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_topo_domain_type(const char *domain_name)
    {
        int result = 0;
        try {
            result = geopm::platform_topo().domain_name_to_type(domain_name);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_topo_create_cache(void)
    {
        int err = 0;
        try {
            geopm::PlatformTopo::create_cache();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }
}

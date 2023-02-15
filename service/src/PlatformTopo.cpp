/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "PlatformTopoImp.hpp"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>
#include <cpuid.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "geopm_sched.h"
#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "GPUTopo.hpp"


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

static volatile unsigned g_is_popen_complete = 0;
static struct sigaction g_popen_complete_signal_action;

static void geopm_topo_popen_complete(int signum)
{
    if (signum == SIGCHLD) {
        g_is_popen_complete = 1;
    }
}

static int geopm_topo_popen(const char *cmd, FILE **fid)
{
    int err = 0;
    *fid = NULL;

    struct sigaction save_action;
    g_popen_complete_signal_action.sa_handler = geopm_topo_popen_complete;
    sigemptyset(&g_popen_complete_signal_action.sa_mask);
    g_popen_complete_signal_action.sa_flags = 0;
    err = sigaction(SIGCHLD, &g_popen_complete_signal_action, &save_action);
    if (!err) {
        *fid = popen(cmd, "r");
        while (*fid && !g_is_popen_complete) {

        }
        g_is_popen_complete = 0;
        sigaction(SIGCHLD, &save_action, NULL);
        if (*fid == NULL) {
            err = errno ? errno : GEOPM_ERROR_RUNTIME;
        }
    }
    return err;
}

namespace geopm
{
    const std::string PlatformTopoImp::M_CACHE_FILE_NAME = "/tmp/geopm-topo-cache-" + std::to_string(getuid());
    const std::string PlatformTopoImp::M_SERVICE_CACHE_FILE_NAME = "/run/geopm-service/geopm-topo-cache";

    const PlatformTopo &platform_topo(void)
    {
        static PlatformTopoImp instance;
        return instance;
    }

    PlatformTopoImp::PlatformTopoImp()
        : PlatformTopoImp("")
    {

    }

    PlatformTopoImp::PlatformTopoImp(const std::string &test_cache_file_name)
        : M_TEST_CACHE_FILE_NAME(test_cache_file_name)
    {
        std::map<std::string, std::string> lscpu_map;
        lscpu(lscpu_map);
        parse_lscpu(lscpu_map, m_num_package, m_core_per_package, m_thread_per_core);
        m_numa_map = parse_lscpu_numa(lscpu_map);
        m_gpu_info[GEOPM_DOMAIN_GPU] = parse_lscpu_gpu(lscpu_map, GEOPM_DOMAIN_GPU);
        m_gpu_info[GEOPM_DOMAIN_GPU_CHIP] = parse_lscpu_gpu(lscpu_map, GEOPM_DOMAIN_GPU_CHIP);
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
            case GEOPM_DOMAIN_MEMORY:
                for (const auto &it : m_numa_map) {
                    if (it.size()) {
                        ++result;
                    }
                }
                break;
            case GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY:
                for (const auto &it : m_numa_map) {
                    if (!it.size()) {
                        ++result;
                    }
                }
                break;
            case GEOPM_DOMAIN_NIC:
            case GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC:
                // @todo Add support for NIC to PlatformTopo.
                result = 0;
                break;
            case GEOPM_DOMAIN_GPU:
            case GEOPM_DOMAIN_GPU_CHIP:
                result = m_gpu_info.at(domain_type).size();
                break;
            case GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU:
                result = 0;
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
            case GEOPM_DOMAIN_GPU:
            case GEOPM_DOMAIN_GPU_CHIP:
                cpu_idx = m_gpu_info.at(domain_type).at(domain_idx);
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
            case GEOPM_DOMAIN_MEMORY:
                cpu_idx = m_numa_map[domain_idx];
                break;
            default:
                throw Exception("PlatformTopoImp::domain_cpus(domain_type=" +
                                std::to_string(domain_type) +
                                ") support not yet implemented",
                                GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            case GEOPM_DOMAIN_MEMORY:
            case GEOPM_DOMAIN_GPU:
            case GEOPM_DOMAIN_GPU_CHIP:
                {
                const auto &domain_map = (domain_type == GEOPM_DOMAIN_MEMORY) ?
                                         m_numa_map :
                                         m_gpu_info.at(domain_type);
                for (const auto &set_it : domain_map) {
                    for (const auto &cpu_it : set_it) {
                        if (cpu_it == cpu_idx) {
                            result = numa_idx;
                            // Find the lowest index numa node that contains the CPU.
                            break;
                        }
                    }
                    if (result != -1) {
                        // Find the lowest index numa node that contains the CPU.
                        break;
                    }
                    ++numa_idx;
                }
                }
                break;
            case GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU:
            case GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY:
            case GEOPM_DOMAIN_NIC:
            case GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC:
                /// @todo Add support for package memory NIC and package GPUs to domain_idx() method.
                throw Exception("PlatformTopoImp::domain_idx() no support yet for PACKAGE_INTEGRATED_MEMORY, NIC, or GPU",
                                GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                break;
            case GEOPM_DOMAIN_INVALID:
            default:
                throw Exception("PlatformTopoImp::domain_idx() invalid domain specified",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        return result;
    }

    bool PlatformTopoImp::is_nested_domain(int inner_domain, int outer_domain) const
    {
        bool result = false;
        static const std::set<int> package_domain = {
            GEOPM_DOMAIN_CPU,
            GEOPM_DOMAIN_CORE,
            GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY,
            GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC,
            GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU,
        };
        if (inner_domain == outer_domain) {
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_BOARD) {
            // All domains are within the board domain
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_CORE &&
                 inner_domain == GEOPM_DOMAIN_CPU) {
            // Only the CPU domain is within the core.
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_PACKAGE &&
                 package_domain.find(inner_domain) != package_domain.end()) {
            // Everything under the package scope is in the package_domain set.
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_MEMORY &&
                 inner_domain == GEOPM_DOMAIN_CPU) {
            // To support mapping CPU signals to DRAM domain (e.g. power)
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_GPU &&
                 inner_domain == GEOPM_DOMAIN_CPU) {
            // To support mapping CPU signals to GPU domain
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_GPU &&
                 inner_domain == GEOPM_DOMAIN_GPU_CHIP) {
            // To support mapping GPU SUBDEVICE signals to GPU domain
            result = true;
        }
        else if (outer_domain == GEOPM_DOMAIN_GPU_CHIP &&
                 inner_domain == GEOPM_DOMAIN_CPU) {
            // To support mapping CPU signals to GPU SUBDEVICE domain
            result = true;
        }
        return result;
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
            {"memory", GEOPM_DOMAIN_MEMORY},
            {"package_integrated_memory", GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY},
            {"nic", GEOPM_DOMAIN_NIC},
            {"package_integrated_nic", GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC},
            {"gpu", GEOPM_DOMAIN_GPU},
            {"package_integrated_gpu", GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU},
            {"gpu_chip", GEOPM_DOMAIN_GPU_CHIP},
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
        if (getuid() == 0) {
            PlatformTopoImp::create_cache(M_SERVICE_CACHE_FILE_NAME);
        }
        else {
            PlatformTopoImp::create_cache(M_CACHE_FILE_NAME);
        }
    }

    void PlatformTopoImp::create_cache(const std::string &cache_file_name)
    {
        const GPUTopo &gtopo = geopm::gpu_topo();
        create_cache(cache_file_name, gtopo);
    }

    void PlatformTopoImp::create_cache(const std::string &cache_file_name, const GPUTopo &gtopo)
    {
        // If cache file is not present, or is too old, create it
        bool is_file_ok = false;
        try {
            is_file_ok = check_file(cache_file_name);
        }
        catch (const geopm::Exception &ex) {
            // sysinfo or stat failed; file does not exist (2) or permission denied (13)
            if (ex.err_value() == EACCES) {
                throw; // Permission was denied; Cannot create files at the desired path
            }
        }

        if (is_file_ok == false) {
            mode_t perms;
            if (cache_file_name == M_SERVICE_CACHE_FILE_NAME) {
                perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0o644
            }
            else {
                perms = S_IRUSR | S_IWUSR; // 0o600
            }

            std::string tmp_string = cache_file_name + "XXXXXX";
            char tmp_path[NAME_MAX];
            tmp_path[NAME_MAX - 1] = '\0';
            strncpy(tmp_path, tmp_string.c_str(), NAME_MAX - 1);
            int tmp_fd = mkstemp(tmp_path);
            if (tmp_fd == -1) {
                throw Exception("PlatformTopo::create_cache(): Could not create temp file: ",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            int err = fchmod(tmp_fd, perms);
            if (err) {
                close(tmp_fd);
                throw Exception("PlatformTopo::create_cache(): Could not chmod tmp_path: ",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            close(tmp_fd);

            std::ostringstream cmd;
            cmd << "lscpu -x >> " << tmp_path << ";";

            FILE *pid;
            err = geopm_topo_popen(cmd.str().c_str(), &pid);
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
            if (gtopo.num_gpu() != 0) {
                std::ofstream cache_stream;
                cache_stream.open(tmp_path, std::ios_base::app);
                std::vector<int> gpu_domains = {
                    GEOPM_DOMAIN_GPU,
                    GEOPM_DOMAIN_GPU_CHIP
                };
                for (const auto &domain_type : gpu_domains) {
                    std::string short_name = gpu_short_name(domain_type);
                    int num_domain = gtopo.num_gpu(domain_type);
                    for (int domain_idx = 0; domain_idx != num_domain; ++domain_idx) {
                        cache_stream << "GPU " << short_name << domain_idx << " CPU(s):";
                        std::string delim = " ";
                        for (const auto &cpu_idx : gtopo.cpu_affinity_ideal(domain_type, domain_idx)) {
                            cache_stream << delim << cpu_idx;
                            delim = ",";
                        }
                        cache_stream << "\n";
                    }
                }
                cache_stream.close();
            }
            err = rename(tmp_path, cache_file_name.c_str());
            if (err) {
                throw Exception("PlatformTopo::create_cache(): Could not rename tmp_path: ",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void PlatformTopoImp::parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
                                      int &num_package,
                                      int &core_per_package,
                                      int &thread_per_core)
    {
        std::vector<std::string> keys = {"CPU(s)",
                                         "Thread(s) per core",
                                         "Core(s) per socket",
                                         "Socket(s)",
                                         "On-line CPU(s) mask"};
        std::vector<std::string> values(keys.size());

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
        try {
            num_package = std::stoi(values[3].c_str());
            int num_core = std::stoi(values[2].c_str()) * num_package;
            core_per_package = num_core / num_package;
            thread_per_core = std::stoi(values[1].c_str());
        }
        catch (const std::invalid_argument &ex) {
            throw Exception("PlatformTopoImp: Unable to convert strings to numbers when parsing lscpu output: " + std::string(ex.what()),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int total_cores_expected_online = num_package * core_per_package * thread_per_core;
        if (total_cores_expected_online != atoi(values[0].c_str())) {
            // Check how many CPUs are actually online
            std::string online_cpu_mask = values[4];
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

    std::vector<std::set<int> > PlatformTopoImp::parse_lscpu_numa(const std::map<std::string, std::string> &lscpu_map)
    {
        std::vector<std::set<int> > numa_map;
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
        if (numa_map.empty()) {
            int num_cpu = m_num_package * m_core_per_package * m_thread_per_core;
            numa_map.push_back({});
            for (int cpu_idx = 0; cpu_idx != num_cpu; ++cpu_idx) {
                numa_map[0].insert(cpu_idx);
            }
        }
        return numa_map;
    }

    std::vector<std::set<int> > PlatformTopoImp::parse_lscpu_gpu(const std::map<std::string, std::string> &lscpu_map, int domain_type)
    {
        std::vector<std::set<int> > gpu_map;
        bool is_domain_found = true;
        std::string short_name = gpu_short_name(domain_type);
        for (int domain_idx = 0; is_domain_found; ++domain_idx) {
            std::ostringstream gpu_key;
            gpu_key << "GPU " << short_name << domain_idx << " CPU(s)";
            auto lscpu_it = lscpu_map.find(gpu_key.str());
            if (lscpu_it == lscpu_map.end()) {
                is_domain_found = false;
            }
            else {
                gpu_map.push_back({});
                auto cpu_set_it = gpu_map.end() - 1;
                for (const auto &cpu_str : geopm::string_split(lscpu_it->second, ",")) {
                    cpu_set_it->insert(std::stoi(cpu_str));
                }
            }
        }
        return gpu_map;
    }

    bool PlatformTopoImp::check_file(const std::string &file_path)
    {
        struct sysinfo si;
        int err = sysinfo(&si);
        if (err) {
            throw Exception("PlatformTopoImp::check_file(): sysinfo err: ",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        struct stat file_stat;
        err = stat(file_path.c_str(), &file_stat);
        if (err) {
            throw Exception("PlatformTopoImp::check_file(): stat failure:",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        struct geopm_time_s current_time;
        geopm_time_real(&current_time);

        unsigned int last_boot_time = current_time.t.tv_sec - si.uptime;
        if (file_stat.st_mtime < last_boot_time) {
            return false; // file is older than last boot
        }
        else {
            mode_t expected_perms;
            if (file_path == M_SERVICE_CACHE_FILE_NAME) {
                expected_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0o644
            }
            else {
                expected_perms = S_IRUSR | S_IWUSR; // 0o600
            }

            mode_t actual_perms = file_stat.st_mode & ~S_IFMT;
            if (expected_perms == actual_perms) {
                return true; // file has been created since boot with the right permissions
            }
            else {
                return false;
            }
        }
    }

    std::string PlatformTopoImp::read_lscpu(void)
    {
        std::string result;
        if (M_TEST_CACHE_FILE_NAME.size()) {
            create_cache(M_TEST_CACHE_FILE_NAME);
            result = geopm::read_file(M_TEST_CACHE_FILE_NAME);
        }
        else {
            if (getuid() == 0) {
                create_cache(M_SERVICE_CACHE_FILE_NAME);
                result = geopm::read_file(M_SERVICE_CACHE_FILE_NAME);
            }
            else {
                create_cache(M_CACHE_FILE_NAME);
                result = geopm::read_file(M_CACHE_FILE_NAME);
            }
        }
        return result;
    }

    void PlatformTopoImp::lscpu(std::map<std::string, std::string> &lscpu_map)
    {
        std::string lscpu_contents = read_lscpu();
        std::istringstream lscpu_stream (lscpu_contents);

        std::string line;
        while (std::getline(lscpu_stream, line)) {
            size_t colon_pos = line.find(":");
            if (colon_pos != std::string::npos) {
                std::string key(line.substr(0, colon_pos));
                std::string value(line.substr(colon_pos + 1));
                size_t data_pos = value.find_first_not_of(" \t");
                if (data_pos &&
                    data_pos != std::string::npos) {
                    // Trim leading white space
                    value = value.substr(data_pos);
                }
                if (key.size()) {
                    lscpu_map.emplace(key, value);
                }
            }
        }
    }

    std::string PlatformTopoImp::gpu_short_name(int domain_type)
    {
        static const std::map<int, std::string> gpu_short_name_map {
                {GEOPM_DOMAIN_GPU, "node"},
                {GEOPM_DOMAIN_GPU_CHIP, "chip"}
        };
        return gpu_short_name_map.at(domain_type);
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

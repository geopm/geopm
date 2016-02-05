/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <set>
#include <string>
#include <inttypes.h>
#include <cpuid.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdexcept>
#include <sstream>

#include "geopm_error.h"
#include "Exception.hpp"
#include "Platform.hpp"
#include "PlatformFactory.hpp"
#include "geopm_message.h"

extern "C"
{
    int geopm_platform_msr_save(const char *path)
    {
        int err = 0;
        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform(std::string("rapl"));
            platform->save_msr_state(path);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_platform_msr_restore(const char *path)
    {
        int err = 0;

        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform(std::string("rapl"));
            platform->restore_msr_state(path);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_platform_msr_whitelist(FILE *file_desc)
    {
        int err = 0;
        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform(std::string("rapl"));

            platform->write_msr_whitelist(file_desc);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }
}

namespace geopm
{
    static const uint64_t PKG_POWER_LIMIT_MASK_MAGIC  = 0x0007800000078000ul;

    Platform::Platform()
        : m_imp(NULL)
        , m_num_domain(0)
        , m_control_domain_type(GEOPM_CONTROL_DOMAIN_POWER)
    {

    }

    Platform::Platform(int control_domain_type)
        : m_imp(NULL)
        , m_num_domain(0)
        , m_control_domain_type(control_domain_type)
    {

    }

    Platform::~Platform()
    {

    }

    void Platform::set_implementation(PlatformImp* platform_imp)
    {
        m_imp = platform_imp;
        m_imp->initialize();
    }

    void Platform::name(std::string &plat_name) const
    {
        if (m_imp == NULL) {
            throw Exception("Platform implementation is missing", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        plat_name = m_imp->platform_name();
    }

    int Platform::num_domain(void) const
    {
        return m_imp->num_package_signal();
    }

    const PlatformTopology *Platform::topology(void) const
    {
        return m_imp->topology();
    }

    void Platform::init_transform(const std::vector<int> &cpu_rank)
    {
        const PlatformTopology *platform_topo = topology();
        std::map<int, int> local_rank_map;
        {
            // Create local_rank_map
            std::set<int> rank_set;
            for (auto it = cpu_rank.begin(); it != cpu_rank.end(); ++it) {
                rank_set.insert(*it);
            }
            int count = 0;
            for (auto it = rank_set.begin(); it != rank_set.end(); ++it, ++count) {
                local_rank_map.insert(std::pair<int, int>(*it, count));
            }
        }
        const unsigned num_local_rank = local_rank_map.size();
        std::vector<double> rank_scatter_factor(num_local_rank);
        {
            // Create rank_scatter_factor
            std::fill(rank_scatter_factor.begin(), rank_scatter_factor.end(), 0.0);
            for (auto it = cpu_rank.begin(); it != cpu_rank.end(); ++it) {
                rank_scatter_factor[local_rank_map.find((*it))->second] += 1.0;
            }
            for (auto it = rank_scatter_factor.begin(); it != rank_scatter_factor.end(); ++it) {
                *it = *it ? 1.0 / (*it) : *it;
            }
        }
        const unsigned num_socket = m_imp->num_package();
        std::vector<double> socket_scatter_factor(num_socket);
        std::vector<std::set<unsigned> > socket_cpu_set(num_socket);
        {
            // Create socket_scatter_factor
            std::vector<hwloc_obj_t> socket;
            std::vector<hwloc_obj_t> children;
            platform_topo->domain_by_type(GEOPM_DOMAIN_PACKAGE, socket);
            for (unsigned socket_idx = 0; socket_idx < num_socket; ++socket_idx) {
                platform_topo->children_by_type(GEOPM_DOMAIN_CPU, socket[socket_idx], children);
                socket_scatter_factor[socket_idx] = 1.0 / children.size();
                for (auto child_it = children.begin(); child_it != children.end(); ++child_it) {
                    socket_cpu_set[socket_idx].insert((*child_it)->logical_index);
                }
            }
        }

        const unsigned NUM_RANK_SIGNAL = 2;
        const unsigned control_domain_type = m_control_domain_type == GEOPM_CONTROL_DOMAIN_POWER ?
                                             m_imp->power_control_domain() :
                                             m_imp->frequency_control_domain();
        const unsigned num_out_signal = platform_topo->num_domain(control_domain_type) * GEOPM_NUM_TELEMETRY_TYPE;
        const unsigned num_in_signal = capacity() + num_local_rank * NUM_RANK_SIGNAL;
        const unsigned num_cpu = m_imp->num_logical_cpu();
        const unsigned num_cpu_signal = num_cpu * GEOPM_NUM_TELEMETRY_TYPE;
        const unsigned cpu_offset = num_socket * m_imp->num_package_signal();
        const unsigned rank_offset = cpu_offset + num_cpu * m_imp->num_cpu_signal();

        unsigned i, j, k;
        unsigned i_signal;
        unsigned j_signal;

        m_signal_domain_matrix.resize(num_in_signal * num_out_signal);
        std::vector<double> scatter_matrix(num_in_signal * num_cpu_signal);
        std::vector<double> gather_matrix(num_cpu_signal * num_out_signal);

        // Scatter Matrix
        for (i = 0; i < num_cpu_signal; ++i) {
            unsigned cpu = i / GEOPM_NUM_TELEMETRY_TYPE;
            i_signal = i % GEOPM_NUM_TELEMETRY_TYPE;
            for (j = 0; j < num_in_signal; ++j) {
                double matrix_value = 0.0;
                if (j < num_socket * m_imp->num_package_signal()) { // Signal is per socket
                    j_signal = j % m_imp->num_package_signal();
                    unsigned socket_idx = j / m_imp->num_package_signal();
                    if (i_signal == j_signal &&
                        socket_cpu_set[socket_idx].find(cpu) != socket_cpu_set[socket_idx].end()) {
                        matrix_value = socket_scatter_factor[socket_idx];
                    }
                }
                else if (j < rank_offset) { // Signal is per cpu
                    j_signal = (j - cpu_offset) % m_imp->num_cpu_signal() + m_imp->num_package_signal();
                    if (i_signal == j_signal) {
                        unsigned curr_cpu = (j - cpu_offset) / m_imp->num_cpu_signal();
                        if (curr_cpu == cpu) {
                            matrix_value = 1.0;
                        }
                    }
                }
                else { // Signal is per rank
                    j_signal = (j - rank_offset) % NUM_RANK_SIGNAL + (GEOPM_NUM_TELEMETRY_TYPE - NUM_RANK_SIGNAL);
                    int local_rank = (j - rank_offset) / NUM_RANK_SIGNAL;
                    if (i_signal == j_signal && local_rank == local_rank_map.find(cpu_rank[cpu])->second) {
                        matrix_value = rank_scatter_factor[local_rank];
                    }
                }
                scatter_matrix[i * num_in_signal + j] = matrix_value;
            }
        }

        // Gather Matrix
        std::fill(gather_matrix.begin(), gather_matrix.end(), 0.0);
        std::vector<hwloc_obj_t> control_domain;
        platform_topo->domain_by_type(control_domain_type, control_domain);
        std::vector<hwloc_obj_t> children;
        for(i = 0; i < num_out_signal; ++i) {
            int domain_idx = i / GEOPM_NUM_TELEMETRY_TYPE;
            i_signal = i % GEOPM_NUM_TELEMETRY_TYPE;
            if (i_signal == 0) { // Compute domain_children only once per domain
                platform_topo->children_by_type(GEOPM_DOMAIN_CPU, control_domain[domain_idx], children);
            }
            for (j = 0; j < num_cpu_signal; ++j) {
                j_signal = j % GEOPM_NUM_TELEMETRY_TYPE;
                if (i_signal == j_signal) {
                    unsigned cpu_idx = j / GEOPM_NUM_TELEMETRY_TYPE;
                    for (auto it = children.begin(); it != children.end(); ++it) {
                        if (cpu_idx == (*it)->logical_index) {
                            gather_matrix[i * num_cpu_signal + j] = 1.0;
                        }
                    }
                }
            }
        }

        // Signal Transform Matrix
        double dot;
        for (i = 0; i < num_in_signal; ++i) {
            for (j = 0; j < num_out_signal; ++j) {
                dot = 0.0;
                for (k = 0; k < num_cpu_signal; k++) {
                    dot += gather_matrix[j * num_cpu_signal + k] *
                           scatter_matrix[k * num_in_signal + i];
                }
                // matrix is stored in row major order for fast matrix
                // vector multiply
                m_signal_domain_matrix[j * num_in_signal + i] = dot;
            }
        }
    }

    const std::vector<double> *Platform::signal_domain_transform() const
    {
        return &m_signal_domain_matrix;
    }

    int Platform::num_control_domain(void) const
    {
        return (topology()->num_domain(m_imp->power_control_domain()));
    }

    void Platform::tdp_limit(int percentage) const
    {
        //Get the TDP for each socket and set its power limit to match
        double tdp = 0.0;
        double power_units = pow(2, (double)((m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT") >> 0) & 0xF));
        int packages = m_imp->num_package();
        int64_t pkg_lim, pkg_magic;

        for (int i = 0; i <  packages; i++) {
            tdp = ((double)(m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO") & 0x3fff)) / power_units;
            tdp *= ((double)percentage * 0.01);
            pkg_lim = (int64_t)(tdp * tdp);
            pkg_magic = pkg_lim | (pkg_lim << 32) | PKG_POWER_LIMIT_MASK_MAGIC;
            m_imp->msr_write(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", pkg_magic);
        }
    }

    void Platform::manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const
    {
        //Set the frequency for each cpu
        int64_t freq_perc;
        bool small = false;
        int num_logical_cpus = m_imp->num_logical_cpu();
        int num_real_cpus = m_imp->num_hw_cpu();
        int packages = m_imp->num_package();
        int num_cpus_per_package = num_real_cpus / packages;
        int num_small_cores_per_package = num_cpus_per_package - (num_cpu_max_perf / packages);

        if (num_cpu_max_perf >= num_real_cpus) {
            throw Exception("requested number of max perf cpus is greater than controllable number of frequency domains on the platform",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        for (int i = 0; i < num_logical_cpus; i++) {
            int real_cpu = i % num_real_cpus;
            if (affinity == GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER && num_cpu_max_perf > 0) {
                if ((real_cpu % num_cpus_per_package) < num_small_cores_per_package) {
                    small = true;
                }
            }
            else if (affinity == GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT && num_cpu_max_perf > 0) {
                if (real_cpu < (num_real_cpus - num_cpu_max_perf)) {
                    small = true;
                }
            }
            else {
                small = true;
            }
            if (small) {
                freq_perc = ((int64_t)(frequency * 0.01) << 8) & 0xffff;
                m_imp->msr_write(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL", freq_perc & 0xffff);
            }
            small = false;
        }
    }

    void Platform::save_msr_state(const char *path) const
    {
        uint64_t msr_val;
        int niter = m_imp->num_package();
        std::ofstream restore_file;

        if (path == NULL) {
            throw Exception("Platform(): file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        restore_file.open(path);

        //per package state
        for (int i = 0; i < niter; i++) {
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->msr_offset("PKG_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->msr_offset("PP0_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->msr_offset("DRAM_POWER_LIMIT") << ":" << msr_val << "\n";
        }

        niter = m_imp->num_hw_cpu();

        //per cpu state
        for (int i = 0; i < niter; i++) {
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "PERF_FIXED_CTR_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("PERF_FIXED_CTR_CTRL") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("PERF_GLOBAL_CTRL") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_OVF_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("PERF_GLOBAL_OVF_CTRL") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("IA32_PERF_CTL") << ":" << msr_val << "\n";

        }

        restore_file.close();
    }

    void Platform::restore_msr_state(const char *path) const
    {
        std::ifstream restore_file;
        std::string line;
        std::vector<int64_t> vals;
        std::string item;

        if (path == NULL) {
            throw Exception("Platform(): file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        restore_file.open(path, std::ios_base::in);

        while (std::getline(restore_file,line)) {
            std::stringstream ss(line);
            while (std::getline(ss, item, ':')) {
                vals.push_back((int64_t)atol(item.c_str()));
            }
            if (vals.size() == 4) {
                m_imp->msr_write(vals[0], vals[1], vals[2], vals[3]);
            }
            else {
                throw Exception("error detected in restore file. Could not restore msr states", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            vals.clear();
        }
        restore_file.close();
        remove(path);
    }

    void Platform::write_msr_whitelist(FILE *file_desc) const
    {
        if (file_desc == NULL) {
            throw Exception("Platform(): file descriptor is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_imp->whitelist(file_desc);
    }
}

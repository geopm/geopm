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

// c includes for system programming
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <sys/stat.h>

#include <sstream>
#include <fstream>
#include <iomanip>

#include "geopm_message.h"
#include "Exception.hpp"
#include "PlatformTopology.hpp"
#include "MSRAccess.hpp"
#include "MSRSignal.hpp"
#include "PlatformImp.hpp"
#include "config.h"

namespace geopm
{

    PlatformImp::PlatformImp()
        : m_msr_access(NULL)
        , m_num_logical_cpu(0)
        , m_num_hw_cpu(0)
        , m_num_tile(0)
        , m_num_tile_group(0)
        , m_num_package(0)
        , m_num_core_per_tile(0)
        , m_control_latency_ms({{GEOPM_DOMAIN_CONTROL_POWER,10.0}})
        , m_tdp_pkg_watts(DBL_MIN)
        , m_trigger_offset(0)
        , m_trigger_value(0)
        , m_is_initialized(false)
        , M_MSR_SAVE_FILE_PATH("/tmp/geopm-msr-initial-vals-XXXXXX")
    {

    }

    PlatformImp::PlatformImp(const std::map<int, double> &control_latency,
                             const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> *msr_signal_map,
                             const std::map<std::string, std::pair<off_t, uint64_t> > *msr_control_map)
        : m_msr_access(NULL)
        , m_msr_signal_map_ptr(msr_signal_map)
        , m_msr_control_map_ptr(msr_control_map)
        , m_num_logical_cpu(0)
        , m_num_hw_cpu(0)
        , m_num_tile(0)
        , m_num_tile_group(0)
        , m_num_package(0)
        , m_num_core_per_tile(0)
        , m_control_latency_ms(control_latency)
        , m_tdp_pkg_watts(DBL_MIN)
        , m_trigger_offset(0)
        , m_trigger_value(0)
        , m_is_initialized(false)
        , M_MSR_SAVE_FILE_PATH("/tmp/geopm-msr-initial-vals-XXXXXX")
    {

    }

    PlatformImp::PlatformImp(const PlatformImp &other)
        : m_topology(other.m_topology)
        , m_msr_access(other.m_msr_access)
        , m_msr_signal_map_ptr(other.m_msr_signal_map_ptr)
        , m_msr_control_map_ptr(other.m_msr_control_map_ptr)
        , m_num_logical_cpu(other.m_num_logical_cpu)
        , m_num_hw_cpu(other.m_num_hw_cpu)
        , m_num_cpu_per_core(other.m_num_cpu_per_core)
        , m_num_tile(other.m_num_tile)
        , m_num_tile_group(other.m_num_tile_group)
        , m_num_package(other.m_num_package)
        , m_num_core_per_tile(other.m_num_core_per_tile)
        , m_control_latency_ms(other.m_control_latency_ms)
        , m_tdp_pkg_watts(other.m_tdp_pkg_watts)
        , m_trigger_offset(other.m_trigger_offset)
        , m_trigger_value(other.m_trigger_value)
        , m_msr_save_file_path(other.m_msr_save_file_path)
        , m_is_initialized(other.m_is_initialized)
        , M_MSR_SAVE_FILE_PATH(other.M_MSR_SAVE_FILE_PATH)
    {

    }


    PlatformImp::~PlatformImp()
    {
        delete m_msr_access;
        remove(m_msr_save_file_path.c_str());
    }

    void PlatformImp::initialize()
    {
        if (!m_is_initialized) {
            parse_hw_topology();
            m_msr_access = new MSRAccess(m_msr_signal_map_ptr, m_msr_control_map_ptr, m_topology);
            save_msr_state(M_MSR_SAVE_FILE_PATH.c_str());
            msr_initialize();
            m_is_initialized = true;
        }
    }

    double PlatformImp::package_tdp(void) const
    {
        return m_tdp_pkg_watts;
    }

    double PlatformImp::control_latency_ms(int control_type) const
    {
        auto it = m_control_latency_ms.find(control_type);
        if (it == m_control_latency_ms.end()) {
            throw Exception("PlatformImp::control_latency_ms(): unknown control type: " +
                            std::to_string(control_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return (*it).second;
    }

    void PlatformImp::msr_write(int device_type, int device_index, const std::string &msr_name, uint64_t value)
    {
        off_t offset = m_msr_access->offset(msr_name);
        unsigned long mask = m_msr_access->write_mask(msr_name);
        int cpu_id = 1;
        if (device_type == GEOPM_DOMAIN_PACKAGE) {
            cpu_id = (m_num_hw_cpu / m_num_package) * device_index;
        }
        else if (device_type == GEOPM_DOMAIN_TILE) {
            cpu_id = (m_num_hw_cpu / m_num_tile) * device_index;
        }
        m_msr_access->write(cpu_id, offset, mask, value);
    }

    uint64_t PlatformImp::msr_read(int device_type, int device_index, const std::string &msr_name) const
    {
        off_t offset = m_msr_access->offset(msr_name);
        int cpu_id = 1;
        if (device_type == GEOPM_DOMAIN_PACKAGE) {
            cpu_id = (m_num_hw_cpu / m_num_package) * device_index;
        }
        else if (device_type == GEOPM_DOMAIN_TILE) {
            cpu_id = (m_num_hw_cpu / m_num_tile) * device_index;
        }
        return m_msr_access->read(cpu_id, offset);
    }

    void PlatformImp::parse_hw_topology(void)
    {
        m_num_logical_cpu = m_topology.num_domain(GEOPM_DOMAIN_CPU);
        m_num_package = m_topology.num_domain(GEOPM_DOMAIN_PACKAGE);
        m_num_hw_cpu = m_topology.num_domain(GEOPM_DOMAIN_PACKAGE_CORE);
        m_num_cpu_per_core = m_num_logical_cpu / m_num_hw_cpu;
        m_num_tile = m_topology.num_domain(GEOPM_DOMAIN_TILE);
        m_num_core_per_tile = m_num_hw_cpu / m_num_tile;
    }

    void PlatformImp::save_msr_state(const char *path)
    {
        size_t path_length;
        int niter = m_num_package;
        std::ofstream save_file;
        std::string tmp_path;

        if (path == NULL) {
            throw Exception("PlatformImp(): MSR save file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        path_length = strlen(path);
        if (path_length > NAME_MAX) {
            throw Exception("Save file path too long!", ENAMETOOLONG, __FILE__, __LINE__);
        }

        if (path_length >= 6 && strncmp(path + path_length - 6, "XXXXXX", NAME_MAX) == 0) {
            //The geopmpolicy main tries to open the path before getting here.  If it was successful,
            //a file would be left dangling.
            struct stat buf;
            if (stat(path, &buf) == 0) {
                (void)remove(path);
            }

            char tmp_path_template[NAME_MAX];
            strncpy(tmp_path_template, path, NAME_MAX);
            int fd = mkstemp(tmp_path_template);
            if (fd == -1) {
                std::ostringstream message;
                message << "Cannot create tmp file: " << tmp_path_template;
                throw Exception(message.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            close(fd);

            if (strncmp(M_MSR_SAVE_FILE_PATH.c_str(), path, path_length) == 0) {
                m_msr_save_file_path = tmp_path_template;
            }

            tmp_path = tmp_path_template;
        }
        else {
            tmp_path = path;
        }

        save_file.open(tmp_path);

        if (!save_file.good()) {
            throw Exception("PlatformImp(): MSR save_file stream is bad", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        //per package state
        for (int i = 0; i < niter; i++) {
            build_msr_save_string(save_file, GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT");
            build_msr_save_string(save_file, GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT");
        }

        niter = m_num_hw_cpu;

        //per cpu state
        for (int i = 0; i < niter; i++) {
            build_msr_save_string(save_file, GEOPM_DOMAIN_CPU, i, "PERF_FIXED_CTR_CTRL");
            build_msr_save_string(save_file, GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_CTRL");
            build_msr_save_string(save_file, GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_OVF_CTRL");
            build_msr_save_string(save_file, GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL");
        }

        save_file.close();
    }

    void PlatformImp::build_msr_save_string(std::ofstream &save_file, int device_type, int device_index, std::string name)
    {
        uint64_t msr_val = msr_read(device_type, device_index, name);
        uint64_t mask = m_msr_access->write_mask(name);
        msr_val &= mask;
        save_file << device_type << " " << device_index << " " << name << " " << msr_val << std::endl;
    }

    void PlatformImp::restore_msr_state(const char *path)
    {
        std::ifstream restore_file;
        std::string line;
        std::vector<uint64_t> vals(3);
        std::string item;

        if (path == NULL) {
            throw Exception("PlatformImp(): file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        restore_file.open(path, std::ios_base::in);

        while (std::getline(restore_file,line)) {
            std::istringstream ss(line);
            std::string name;
            ss >> vals[0] >> vals[1] >> name >> vals[2];
            msr_write(vals[0], vals[1], name, vals[2]);
        }
        restore_file.close();
        remove(path);
    }

    void PlatformImp::revert_msr_state(void)
    {
        restore_msr_state(m_msr_save_file_path.c_str());
    }

    bool PlatformImp::is_updated(void)
    {
        uint64_t curr_value = m_msr_access->read(0, m_trigger_offset);
        bool result = (m_trigger_value && curr_value != m_trigger_value);
        m_trigger_value = curr_value;
        return result;
    }


    std::string PlatformImp::msr_save_file_path(void)
    {
        return m_msr_save_file_path;
    }

    size_t PlatformImp::num_signal(void) {
        return m_signal.size();
    }
}

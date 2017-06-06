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

#include "Exception.hpp"
#include "PlatformImp.hpp"
#include "config.h"

namespace geopm
{

    PlatformImp::PlatformImp()
        : m_num_logical_cpu(0)
        , m_num_hw_cpu(0)
        , m_num_tile(0)
        , m_num_tile_group(0)
        , m_num_package(0)
        , m_num_core_per_tile(0)
        , m_control_latency_ms(10.0)
        , m_tdp_pkg_watts(DBL_MIN)
        , m_msr_batch_desc(-1)
        , m_is_batch_enabled(false)
        , m_batch({0, NULL})
        , m_trigger_offset(0)
        , m_trigger_value(0)
        , m_is_initialized(false)
        , M_MSR_SAVE_FILE_PATH("/tmp/geopm-msr-initial-vals-XXXXXX")
    {

    }

    PlatformImp::PlatformImp(int num_energy_signal, int num_counter_signal, double control_latency, const std::map<std::string, std::pair<off_t, unsigned long> > *msr_map_ptr)
        : m_msr_map_ptr(msr_map_ptr)
        , m_num_logical_cpu(0)
        , m_num_hw_cpu(0)
        , m_num_tile(0)
        , m_num_tile_group(0)
        , m_num_package(0)
        , m_num_core_per_tile(0)
        , m_num_energy_signal(num_energy_signal)
        , m_num_counter_signal(num_counter_signal)
        , m_control_latency_ms(control_latency)
        , m_tdp_pkg_watts(DBL_MIN)
        , m_msr_batch_desc(-1)
        , m_is_batch_enabled(false)
        , m_batch({0, NULL})
        , m_trigger_offset(0)
        , m_trigger_value(0)
        , m_is_initialized(false)
        , M_MSR_SAVE_FILE_PATH("/tmp/geopm-msr-initial-vals-XXXXXX")
    {

    }

    PlatformImp::PlatformImp(const PlatformImp &other)
        : m_topology(other.m_topology)
        , m_cpu_file_desc(other.m_cpu_file_desc)
        , m_msr_map_ptr(other.m_msr_map_ptr)
        , m_num_logical_cpu(other.m_num_logical_cpu)
        , m_num_hw_cpu(other.m_num_hw_cpu)
        , m_num_cpu_per_core(other.m_num_cpu_per_core)
        , m_num_tile(other.m_num_tile)
        , m_num_tile_group(other.m_num_tile_group)
        , m_num_package(other.m_num_package)
        , m_num_core_per_tile(other.m_num_core_per_tile)
        , m_num_energy_signal(other.m_num_energy_signal)
        , m_num_counter_signal(other.m_num_counter_signal)
        , m_control_latency_ms(other.m_control_latency_ms)
        , m_tdp_pkg_watts(other.m_tdp_pkg_watts)
        , m_msr_value_last(other.m_msr_value_last)
        , m_msr_overflow_offset(other.m_msr_overflow_offset)
        , m_msr_batch_desc(other.m_msr_batch_desc)
        , m_is_batch_enabled(other.m_is_batch_enabled)
        , m_batch(other.m_batch)
        , m_trigger_offset(other.m_trigger_offset)
        , m_trigger_value(other.m_trigger_value)
        , m_msr_save_file_path(other.m_msr_save_file_path)
        , m_is_initialized(other.m_is_initialized)
        , M_MSR_SAVE_FILE_PATH(other.M_MSR_SAVE_FILE_PATH)
    {
        // Copy C string for m_msr_path
        m_msr_path[NAME_MAX - 1] = '\0';
        strncpy(m_msr_path, other.m_msr_path, NAME_MAX - 1);
    }


    PlatformImp::~PlatformImp()
    {
        if (m_batch.numops) {
            free(m_batch.ops);
        }

        for (int i = 0; i < m_num_logical_cpu; ++i) {
            msr_close(i);
        }

        if (m_msr_batch_desc != -1) {
            close(m_msr_batch_desc);
        }

        remove(m_msr_save_file_path.c_str());
    }

    void PlatformImp::initialize()
    {
        if (!m_is_initialized) {
            parse_hw_topology();
            for (int i = 0; i < m_num_logical_cpu; i++) {
                msr_open(i);
            }
            save_msr_state(M_MSR_SAVE_FILE_PATH.c_str());
            msr_initialize();
            m_is_initialized = true;
        }
    }

    int PlatformImp::num_package(void) const
    {
        return m_num_package;
    }

    int PlatformImp::num_tile(void) const
    {
        return m_num_tile;
    }

    int PlatformImp::num_tile_group(void) const
    {
        return m_num_tile_group;
    }

    int PlatformImp::num_hw_cpu(void) const
    {
        return m_num_hw_cpu;
    }

    int PlatformImp::num_logical_cpu(void) const
    {
        return m_num_logical_cpu;
    }

    int PlatformImp::num_energy_signal(void) const
    {
        return m_num_energy_signal;
    }

    int PlatformImp::num_counter_signal(void) const
    {
        return m_num_counter_signal;
    }

    double PlatformImp::package_tdp(void) const
    {
        return m_tdp_pkg_watts;
    }

    int PlatformImp::num_domain(int domain_type)
    {
        int count;
        switch (domain_type) {
            case GEOPM_DOMAIN_PACKAGE:
                count = m_num_package;
                break;
            case GEOPM_DOMAIN_CPU:
                count = m_num_hw_cpu;
                break;
            case GEOPM_DOMAIN_TILE:
                count = m_num_tile;
                break;
            case GEOPM_DOMAIN_TILE_GROUP:
                count = m_num_tile_group;
                break;
            default:
                count = 0;
                break;
        }
        return count;
    }

    double PlatformImp::control_latency_ms(void) const
    {
        return m_control_latency_ms;
    }

    const PlatformTopology *PlatformImp::topology(void) const
    {
        return &m_topology;
    }

    void PlatformImp::msr_write(int device_type, int device_index, const std::string &msr_name, uint64_t value)
    {
        off_t offset = msr_offset(msr_name);
        unsigned long mask = msr_mask(msr_name);
        msr_write(device_type, device_index, offset, mask, value);
    }

    void PlatformImp::msr_write(int device_type, int device_index, off_t msr_offset, unsigned long msr_mask, uint64_t value)
    {
        uint64_t old_value;
        uint64_t curr_value;

        curr_value = msr_read(device_type, device_index, msr_offset);
        curr_value &= ~msr_mask;

        if (device_type == GEOPM_DOMAIN_PACKAGE)
            device_index = (m_num_hw_cpu / m_num_package) * device_index;
        else if (device_type == GEOPM_DOMAIN_TILE)
            device_index = (m_num_hw_cpu / m_num_tile) * device_index;

        if (m_cpu_file_desc.size() < (uint64_t)device_index) {
            throw Exception("no file descriptor found for cpu device", GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }

        old_value = value;
        value &= msr_mask;
        if (value != old_value) {
            std::ostringstream message;
            message << "MSR value to be written was modified by the mask! Desired = 0x" << std::hex << old_value
                    << " After mask = 0x" << std::hex << value;
            throw Exception(message.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }

        value |= curr_value;

        int rv = pwrite(m_cpu_file_desc[device_index], &value, sizeof(value), msr_offset);
        if (rv != sizeof(value)) {
            std::ostringstream ex_str;
            ex_str << "offset: " << msr_offset << " value: " << value;
            throw Exception(ex_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
    }

    uint64_t PlatformImp::msr_read(int device_type, int device_index, const std::string &msr_name)
    {
        off_t offset = msr_offset(msr_name);
        return msr_read(device_type, device_index, offset);
    }

    uint64_t PlatformImp::msr_read(int device_type, int device_index, off_t msr_offset)
    {
        uint64_t value;
        int index = device_index;

        if (device_type == GEOPM_DOMAIN_PACKAGE)
            index = (m_num_logical_cpu / m_num_package) * device_index;
        else if (device_type == GEOPM_DOMAIN_TILE)
            index = (m_num_logical_cpu / m_num_tile) * device_index;

        if (m_cpu_file_desc.size() < (uint64_t)index) {
            throw Exception("no file descriptor found for cpu device", GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
        int rv = pread(m_cpu_file_desc[index], &value, sizeof(value), msr_offset);
        if (rv != sizeof(value)) {
            throw Exception(std::to_string(msr_offset), GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }

        return value;
    }

    void PlatformImp::batch_msr_read(void)
    {
        int rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &m_batch);

        if (rv) {
            throw Exception("read from /dev/cpu/msr_batch failed", GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
    }

    off_t PlatformImp::msr_offset(std::string msr_name)
    {
        auto it = m_msr_map_ptr->find(msr_name);
        if (it == m_msr_map_ptr->end()) {
            throw Exception("MSR string not found in offset map", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return (*it).second.first;
    }

    unsigned long PlatformImp::msr_mask(std::string msr_name)
    {
        auto it = m_msr_map_ptr->find(msr_name);
        if (it == m_msr_map_ptr->end()) {
            throw Exception("MSR string not found in offset map", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return (*it).second.second;
    }

    void PlatformImp::msr_path(int cpu_num)
    {
        struct stat s;
        int err;

        // check for the msr-safe driver
        err = stat("/dev/cpu/0/msr_safe", &s);
        if (err == 0) {
            snprintf(m_msr_path, NAME_MAX, "/dev/cpu/%d/msr_safe", cpu_num);
            //check for batch support
            m_msr_batch_desc = open("/dev/cpu/msr_batch", O_RDWR);
            if (m_msr_batch_desc != -1) {
                m_is_batch_enabled = true;
            }
            return;
        }

        // fallback to the default msr driver
        err = stat("/dev/cpu/0/msr", &s);
        if (err == 0) {
            snprintf(m_msr_path, NAME_MAX, "/dev/cpu/%d/msr", cpu_num);
            return;
        }

        throw Exception("checked /dev/cpu/0/msr and /dev/cpu/0/msr_safe", GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);
    }

    void PlatformImp::msr_open(int cpu)
    {
        int fd;

        msr_path(cpu);
        fd = open(m_msr_path, O_RDWR);
        //report errors
        if (fd < 0) {
            char error_string[NAME_MAX];
            if (errno == ENXIO || errno == ENOENT) {
                snprintf(error_string, NAME_MAX, "device %s does not exist", m_msr_path);
            }
            else if (errno == EPERM || errno == EACCES) {
                snprintf(error_string, NAME_MAX, "permission denied opening device %s", m_msr_path);
            }
            else {
                snprintf(error_string, NAME_MAX, "system error opening cpu device %s", m_msr_path);
            }
            throw Exception(error_string, GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);

            return;
        }
        //all is good, save handle
        m_cpu_file_desc.push_back(fd);
    }

    void PlatformImp::msr_close(int cpu)
    {
        if (m_cpu_file_desc.size() > (size_t)cpu &&
            m_cpu_file_desc[cpu] >= 0) {
            int rv = close(m_cpu_file_desc[cpu]);
            //mark as invalid
            m_cpu_file_desc[cpu] = -1;

            //check for errors
            if (rv < 0) {
                throw Exception("system error closing cpu device", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void PlatformImp::whitelist(FILE *file_desc)
    {
        fprintf(file_desc, "# MSR      Write Mask         # Comment\n");
        for (auto it : *m_msr_map_ptr) {
            fprintf(file_desc, "0x%.8llx 0x%.16lx # %s\n", (long long)it.second.first, it.second.second, it.first.c_str());
        }
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

    double PlatformImp::msr_overflow(int signal_idx, uint32_t msr_size, uint64_t value)
    {
        // Mask off bits beyond msr_size
        value &= ((~0ULL) >> (64 - msr_size));
        // Deal with register overflow
        if (value < m_msr_value_last[signal_idx]) {
            m_msr_overflow_offset[signal_idx] += pow(2, msr_size);
        }
        m_msr_value_last[signal_idx] = value;
        return value + m_msr_overflow_offset[signal_idx];
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
        unsigned long mask = msr_mask(name);
        msr_val &= mask;
        save_file << device_type << ":" << device_index << ":" << msr_offset(name) << ":" << msr_mask(name) << ":" << msr_val << std::endl;
    }

    void PlatformImp::restore_msr_state(const char *path)
    {
        std::ifstream restore_file;
        std::string line;
        std::vector<uint64_t> vals;
        std::string item;

        if (path == NULL) {
            throw Exception("PlatformImp(): file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        restore_file.open(path, std::ios_base::in);

        while (std::getline(restore_file,line)) {
            std::stringstream ss(line);
            while (std::getline(ss, item, ':')) {
                vals.push_back((uint64_t)strtoul(item.c_str(), NULL, 0));
            }
            if (vals.size() == 5) {
                msr_write(vals[0], vals[1], vals[2], vals[3], vals[4]);
            }
            else {
                throw Exception("error detected in restore file. Could not restore msr states", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            vals.clear();
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
        uint64_t curr_value = msr_read(GEOPM_DOMAIN_PACKAGE, 0, m_trigger_offset);
        bool result = (m_trigger_value && curr_value != m_trigger_value);
        m_trigger_value = curr_value;
        return result;
    }


    std::string PlatformImp::msr_save_file_path(void)
    {
        return m_msr_save_file_path;
    }
}

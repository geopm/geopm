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

// c includes for system programming
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

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
        , m_control_latency_ms(10.0)
    {

    }

    PlatformImp::PlatformImp(int num_package_signal, int num_cpu_signal, double control_latency)
        : m_num_logical_cpu(0)
        , m_num_hw_cpu(0)
        , m_num_tile(0)
        , m_num_tile_group(0)
        , m_num_package(0)
        , m_num_package_signal(num_package_signal)
        , m_num_cpu_signal(num_cpu_signal)
        , m_control_latency_ms(control_latency)
    {

    }

    PlatformImp::~PlatformImp() {}

    void PlatformImp::initialize()
    {
        parse_hw_topology();
        msr_initialize();
        int num_signal = m_num_package_signal * m_num_package + m_num_cpu_signal * m_num_hw_cpu;
        m_msr_value_last.resize(num_signal);
        m_msr_overflow_offset.resize(num_signal);
        std::fill(m_msr_value_last.begin(), m_msr_value_last.end(), 0.0);
        std::fill(m_msr_overflow_offset.begin(), m_msr_overflow_offset.end(), 0.0);
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

    int PlatformImp::num_package_signal(void) const
    {
        return m_num_package_signal;
    }

    int PlatformImp::num_cpu_signal(void) const
    {
        return m_num_cpu_signal;
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
        off_t offset = m_msr_offset_map.find(msr_name)->second.first;
        msr_write(device_type, device_index, offset, value);
    }

    void PlatformImp::msr_write(int device_type, int device_index, off_t msr_offset, uint64_t value)
    {
        if (device_type == GEOPM_DOMAIN_PACKAGE)
            device_index = (m_num_logical_cpu / m_num_package) * device_index;
        else if (device_type == GEOPM_DOMAIN_TILE)
            device_index = (m_num_logical_cpu / m_num_tile) * device_index;

        if (m_cpu_file_desc.size() < (uint64_t)device_index) {
            throw Exception("no file descriptor found for cpu device", GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
        int rv = pwrite(m_cpu_file_desc[device_index], &value, sizeof(value), msr_offset);
        if (rv != sizeof(value)) {
            throw Exception(std::to_string(msr_offset), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
    }

    uint64_t PlatformImp::msr_read(int device_type, int device_index, const std::string &msr_name)
    {
        off_t offset = m_msr_offset_map.find(msr_name)->second.first;
        return msr_read(device_type, device_index, offset);
    }

    uint64_t PlatformImp::msr_read(int device_type, int device_index, off_t msr_offset)
    {
        uint64_t value;

        if (device_type == GEOPM_DOMAIN_PACKAGE)
            device_index = (m_num_logical_cpu / m_num_package) * device_index;
        else if (device_type == GEOPM_DOMAIN_TILE)
            device_index = (m_num_logical_cpu / m_num_tile) * device_index;


        if (m_cpu_file_desc.size() < (uint64_t)device_index) {
            throw Exception("no file descriptor found for cpu device", GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
        int rv = pread(m_cpu_file_desc[device_index], &value, sizeof(value), msr_offset);
        if (rv != sizeof(value)) {
            throw Exception(std::to_string(msr_offset), GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }

        return value;
    }

    off_t PlatformImp::msr_offset(std::string msr_name)
    {
        return m_msr_offset_map.find(msr_name)->second.first;
    }

    void PlatformImp::msr_path(int cpu_num)
    {
        struct stat s;
        int err;

        // check for the msr-safe driver
        err = stat("/dev/cpu/0/msr_safe", &s);
        if (err == 0) {
            snprintf(m_msr_path, NAME_MAX, "/dev/cpu/%d/msr_safe", cpu_num);
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

        //all is good, return handle
        m_cpu_file_desc.push_back(fd);
    }

    void PlatformImp::msr_close(int cpu)
    {
        int rv = close(m_cpu_file_desc[cpu]);
        //mark as invalid
        m_cpu_file_desc[cpu] = -1;

        //check for errors
        if (rv < 0) {
            throw Exception("system error closing cpu device", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void PlatformImp::whitelist(FILE *file_desc)
    {
        fprintf(file_desc, "# MSR      Write Mask         # Comment\n");
        for (auto it : m_msr_offset_map) {
            fprintf(file_desc, "0x%.8llx 0x%.16lx # %s\n", (long long)it.second.first, it.second.second, it.first.c_str());
        }
    }

    void PlatformImp::parse_hw_topology(void)
    {
        m_num_logical_cpu = m_topology.num_domain(GEOPM_DOMAIN_CPU);
        m_num_package = m_topology.num_domain(GEOPM_DOMAIN_PACKAGE);
        m_num_hw_cpu = m_topology.num_domain(GEOPM_DOMAIN_PACKAGE_CORE);
        m_num_cpu_per_core = m_num_logical_cpu / m_num_hw_cpu;
    }

    double PlatformImp::msr_overflow(int signal_idx, uint32_t msr_size, double value)
    {
        // Deal with register overflow
        if (value < m_msr_value_last[signal_idx]) {
            m_msr_overflow_offset[signal_idx] += pow(2, msr_size);
        }
        m_msr_value_last[signal_idx] = value;
        value += m_msr_overflow_offset[signal_idx];

        return value;
    }
}

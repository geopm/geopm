/*
 * Copyright (c) 2015, Intel Corporation
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

#include "geopm_error.h"
#include "Exception.hpp"
#include "PlatformImp.hpp"

namespace geopm
{

    PlatformImp::PlatformImp()
        : m_hyperthreads(0)
        , m_num_cpu(0)
        , m_num_tile(0)
        , m_num_package(0)
    {
        parse_hw_topology();
    }

    PlatformImp::~PlatformImp() {}

    uint32_t PlatformImp::get_num_package(void) const
    {
        return m_num_package;
    }

    uint32_t PlatformImp::get_num_tile(void) const
    {
        return m_num_tile;
    }

    uint32_t PlatformImp::get_num_cpu(void) const
    {
        return m_num_cpu;
    }

    uint32_t PlatformImp::get_num_hyperthreads(void) const
    {
        return m_hyperthreads;
    }

    PlatformTopology PlatformImp::topology(void) const
    {
        return m_topology;
    }

    void PlatformImp::write_msr(int device_type, int device_index, const std::string &msr_name, uint64_t value)
    {
        off_t offset = m_msr_offset_map.find(msr_name)->second;
        write_msr(device_type, device_index, offset, value);
    }

    void PlatformImp::write_msr(int device_type, int device_index, off_t msr_offset, uint64_t value)
    {
        if (device_type == GEOPM_DOMAIN_PACKAGE)
            device_index = (m_num_cpu / m_num_package) * device_index;
        else if (device_type == GEOPM_DOMAIN_TILE)
            device_index = (m_num_cpu / m_num_tile) * device_index;

        if (m_cpu_file_descs.size() < (uint64_t)device_index) {
            throw Exception("no file descriptor found for cpu device", ENODEV, __FILE__, __LINE__);
        }
        int rv = pwrite(m_cpu_file_descs[device_index], &value, sizeof(value), msr_offset);
        if (rv != sizeof(value)) {
            throw Exception("error writing to msr", EBADF, __FILE__, __LINE__);
        }
    }

    uint64_t PlatformImp::read_msr(int device_type, int device_index, const std::string &msr_name)
    {
        off_t offset = m_msr_offset_map.find(msr_name)->second;
        return read_msr(device_type, device_index, offset);
    }

    uint64_t PlatformImp::read_msr(int device_type, int device_index, off_t msr_offset)
    {
        uint64_t value;

        if (device_type == GEOPM_DOMAIN_PACKAGE)
            device_index = (m_num_cpu / m_num_package) * device_index;
        else if (device_type == GEOPM_DOMAIN_TILE)
            device_index = (m_num_cpu / m_num_tile) * device_index;


        if (m_cpu_file_descs.size() < (uint64_t)device_index) {
            throw Exception("no file descriptor found for cpu device", ENODEV, __FILE__, __LINE__);
        }
        int rv = pread(m_cpu_file_descs[device_index], &value, sizeof(value), msr_offset);
        if (rv != sizeof(value)) {
            throw Exception("error reading msr", EBADF, __FILE__, __LINE__);
        }

        return value;
    }

    off_t PlatformImp::get_msr_offset(std::string msr_name)
    {
        return m_msr_offset_map.find(msr_name)->second;
    }

    void PlatformImp::set_msr_path(int cpu_num)
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

        throw Exception("could not stat msr directory", errno, __FILE__, __LINE__);
    }

    void PlatformImp::open_msr(int cpu)
    {
        int fd;

        set_msr_path(cpu);
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
            throw Exception(error_string, errno, __FILE__, __LINE__);

            return;
        }

        //all is good, return handle
        m_cpu_file_descs.push_back(fd);
    }

    void PlatformImp::close_msr(int cpu)
    {
        int rv = close(m_cpu_file_descs[cpu]);
        //mark as invalid
        m_cpu_file_descs[cpu] = -1;

        //check for errors
        if (rv < 0) {
            throw Exception("system error closing cpu device", errno, __FILE__, __LINE__);
        }
    }

    void PlatformImp::parse_hw_topology()
    {
        int num_core = 0;

        m_num_cpu = m_topology.num_domain(GEOPM_DOMAIN_CPU);
        m_num_package = m_topology.num_domain(GEOPM_DOMAIN_PACKAGE);
        num_core = m_topology.num_domain(GEOPM_DOMAIN_PACKAGE_CORE);
        m_hyperthreads = num_core ? (m_num_cpu / num_core) : 1;
    }
}

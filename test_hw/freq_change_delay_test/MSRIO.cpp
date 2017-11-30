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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <map>
#include <stdexcept>

#include "MSRIO.hpp"


namespace geopm
{
    MSRIO::MSRIO()
        : m_num_cpu(sysconf(_SC_NPROCESSORS_ONLN))
        , m_file_desc(m_num_cpu + 1, -1) // Last file descriptor is for the batch file
    {

    }

    MSRIO::~MSRIO()
    {
        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            close_msr(cpu_idx);
        }
    }

    uint64_t MSRIO::read_msr(int cpu_idx,
                             uint64_t offset)
    {
        uint64_t result = 0;
        size_t num_read = pread(msr_desc(cpu_idx), &result, sizeof(result), offset);
        if (num_read != sizeof(result)) {
            std::ostringstream err_str;
            err_str << "MSRIO::read_msr(): pread() failed at offset 0x" << std::hex << offset
                    << " system error: " << strerror(errno);
            throw std::runtime_error(err_str.str());
        }
        return result;
    }

    void MSRIO::write_msr(int cpu_idx,
                          uint64_t offset,
                          uint64_t raw_value,
                          uint64_t write_mask)
    {
        if ((raw_value & write_mask) != raw_value) {
            std::ostringstream err_str;
            err_str << "MSRIO::write_msr(): raw_value does not obey write_mask, "
                       "raw_value=0x" << std::hex << raw_value
                    << " write_mask=0x" << write_mask;
            throw std::runtime_error(err_str.str());
        }
        uint64_t write_value = read_msr(cpu_idx, offset);
        write_value &= ~write_mask;
        write_value |= raw_value;
        size_t num_write = pwrite(msr_desc(cpu_idx), &write_value, sizeof(write_value), offset);
        if (num_write != sizeof(write_value)) {
            std::ostringstream err_str;
            err_str << "MSRIO::read_msr(): pwrite() failed at offset 0x" << std::hex << offset
                    << " system error: " << strerror(errno);
            throw std::runtime_error(err_str.str());
        }
    }

    int MSRIO::msr_desc(int cpu_idx)
    {
        if (cpu_idx < 0 || cpu_idx > m_num_cpu) {
            std::ostringstream err_str;
            err_str << "MSRIO: cpu_idx=" << cpu_idx
                    << " out of range, num_cpu=" << m_num_cpu;
            throw std::runtime_error(err_str.str());
        }
        open_msr(cpu_idx);
        return m_file_desc[cpu_idx];
    }

    void MSRIO::msr_path(int cpu_idx,
                         bool is_fallback,
                         std::string &path)
    {
        std::ostringstream msr_path;
        msr_path << "/dev/cpu/" << cpu_idx;
        if (!is_fallback) {
            msr_path << "/msr_safe";
        }
        else {
            msr_path << "/msr";
        }
        path = msr_path.str();
    }

    void MSRIO::open_msr(int cpu_idx)
    {
        if (m_file_desc[cpu_idx] == -1) {
            std::string path;
            msr_path(cpu_idx, false, path);
            m_file_desc[cpu_idx] = open(path.c_str(), O_RDWR);
            if (m_file_desc[cpu_idx] == -1) {
                errno = 0;
                msr_path(cpu_idx, true, path);
                m_file_desc[cpu_idx] = open(path.c_str(), O_RDWR);
                if (m_file_desc[cpu_idx] == -1) {
                    std::ostringstream err_str;
                    err_str << "MSRIO::open_msr(): Failed to open \"" << path << "\": "
                            << "system error: " << strerror(errno);
                    throw std::runtime_error(err_str.str());
                }
            }
        }
        struct stat stat_buffer;
        int err = fstat(m_file_desc[cpu_idx], &stat_buffer);
        if (err) {
            throw std::runtime_error("MSRIO::open_msr(): file descritor invalid");
        }
    }

    void MSRIO::close_msr(int cpu_idx)
    {
        if (m_file_desc[cpu_idx] != -1) {
            (void)close(m_file_desc[cpu_idx]);
            m_file_desc[cpu_idx] = -1;
        }
    }
}

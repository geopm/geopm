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

#include "MSRIOImp.hpp"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <map>

#include "geopm_sched.h"
#include "Exception.hpp"
#include "Helper.hpp"
#include "config.h"

#define GEOPM_IOC_MSR_BATCH _IOWR('c', 0xA2, struct geopm::MSRIOImp::m_msr_batch_array_s)

namespace geopm
{
    std::unique_ptr<MSRIO> MSRIO::make_unique(void)
    {
        return geopm::make_unique<MSRIOImp>();
    }

    std::shared_ptr<MSRIO> MSRIO::make_shared(void)
    {
        return std::make_shared<MSRIOImp>();
    }

    MSRIOImp::MSRIOImp()
        : MSRIOImp(geopm_sched_num_cpu())
    {

    }

    MSRIOImp::MSRIOImp(int num_cpu)
        : m_num_cpu(num_cpu)
        , m_file_desc(m_num_cpu + 1, -1) // Last file descriptor is for the batch file
        , m_is_batch_enabled(true)
        , m_read_batch({0, NULL})
        , m_write_batch({0, NULL})
        , m_read_batch_op(0)
        , m_write_batch_op(0)
    {

    }

    MSRIOImp::~MSRIOImp()
    {
        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            close_msr(cpu_idx);
        }
        close_msr_batch();
    }

    uint64_t MSRIOImp::read_msr(int cpu_idx,
                                uint64_t offset)
    {
        uint64_t result = 0;
        size_t num_read = pread(msr_desc(cpu_idx), &result, sizeof(result), offset);
        if (num_read != sizeof(result)) {
            std::ostringstream err_str;
            err_str << "MSRIOImp::read_msr(): pread() failed at offset 0x" << std::hex << offset
                    << " system error: " << strerror(errno);
            throw Exception(err_str.str(), GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
        return result;
    }

    void MSRIOImp::write_msr(int cpu_idx,
                             uint64_t offset,
                             uint64_t raw_value,
                             uint64_t write_mask)
    {
        if ((raw_value & write_mask) != raw_value) {
            std::ostringstream err_str;
            err_str << "MSRIOImp::write_msr(): raw_value does not obey write_mask, "
                    << "raw_value=0x" << std::hex << raw_value
                    << " write_mask=0x" << write_mask;
            throw Exception(err_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        uint64_t write_value = read_msr(cpu_idx, offset);
        write_value &= ~write_mask;
        write_value |= raw_value;
        size_t num_write = pwrite(msr_desc(cpu_idx), &write_value, sizeof(write_value), offset);
        if (num_write != sizeof(write_value)) {
            std::ostringstream err_str;
            err_str << "MSRIOImp::write_msr(): pwrite() failed at offset 0x" << std::hex << offset
                    << " system error: " << strerror(errno);
            throw Exception(err_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
    }

    void MSRIOImp::config_batch(const std::vector<int> &read_cpu_idx,
                                const std::vector<uint64_t> &read_offset,
                                const std::vector<int> &write_cpu_idx,
                                const std::vector<uint64_t> &write_offset,
                                const std::vector<uint64_t> &write_mask)
    {
        if (read_cpu_idx.size() != read_offset.size() ||
            write_cpu_idx.size() != write_offset.size() ||
            write_offset.size() != write_mask.size()) {
            throw Exception("MSRIOImp::config_batch(): Input vector length mismatch",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_read_batch_op.resize(read_cpu_idx.size());
        {
            auto cpu_it = read_cpu_idx.begin();
            auto offset_it = read_offset.begin();
            for (auto batch_it = m_read_batch_op.begin();
                 batch_it != m_read_batch_op.end();
                 ++batch_it, ++cpu_it, ++offset_it) {
                batch_it->cpu = *cpu_it;
                batch_it->isrdmsr = 1;
                batch_it->err = 0;
                batch_it->msr = *offset_it;
                batch_it->msrdata = 0;
                batch_it->wmask = 0;
            }
        }
        m_read_batch.numops = m_read_batch_op.size();
        m_read_batch.ops = m_read_batch_op.data();

        m_write_batch_op.resize(write_cpu_idx.size());
        {
            auto cpu_it = write_cpu_idx.begin();
            auto offset_it = write_offset.begin();
            auto mask_it = write_mask.begin();
            for (auto batch_it = m_write_batch_op.begin();
                 batch_it != m_write_batch_op.end();
                 ++batch_it, ++cpu_it, ++offset_it, ++mask_it) {
                batch_it->cpu = *cpu_it;
                batch_it->isrdmsr = 0;
                batch_it->err = 0;
                batch_it->msr = *offset_it;
                batch_it->msrdata = 0;
                batch_it->wmask = *mask_it;
            }
        }
        m_write_batch.numops = m_write_batch_op.size();
        m_write_batch.ops = m_write_batch_op.data();
    }

    void MSRIOImp::msr_ioctl(bool is_read)
    {
        struct m_msr_batch_array_s *batch_ptr = is_read ? &m_read_batch : &m_write_batch;
        int err = ioctl(msr_batch_desc(), GEOPM_IOC_MSR_BATCH, batch_ptr);
        if (err) {
            throw Exception("MSRIOImp::msr_ioctl(): call to ioctl() for /dev/cpu/msr_batch failed: " +
                            std::string(" system error: ") + strerror(errno),
                            GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
        for (uint32_t batch_idx = 0; batch_idx != m_write_batch.numops; ++batch_idx) {
            if (m_write_batch.ops[batch_idx].err) {
                std::ostringstream err_str;
                err_str << "MSRIOImp::msr_ioctl(): operation failed at offset 0x"
                        << std::hex << m_write_batch.ops[batch_idx].msr
                        << " system error: " << strerror(m_write_batch.ops[batch_idx].err);
                throw Exception(err_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
            }
        }
    }

    void MSRIOImp::read_batch(std::vector<uint64_t> &raw_value)
    {
        if (raw_value.size() < m_read_batch.numops) {
            raw_value.resize(m_read_batch.numops);
        }
        open_msr_batch();
        if (m_is_batch_enabled) {
            msr_ioctl(true);
            uint32_t batch_idx = 0;
            for (auto raw_it = raw_value.begin();
                 batch_idx != m_read_batch.numops;
                 ++raw_it, ++batch_idx) {
                *raw_it = m_read_batch.ops[batch_idx].msrdata;
            }
        }
        else {
            uint32_t batch_idx = 0;
            for (auto raw_it = raw_value.begin();
                 batch_idx != m_read_batch.numops;
                 ++raw_it, ++batch_idx) {
                *raw_it = read_msr(m_read_batch_op[batch_idx].cpu,
                                   m_read_batch_op[batch_idx].msr);
            }
        }
    }

    void MSRIOImp::write_batch(const std::vector<uint64_t> &raw_value)
    {
        if (raw_value.size() < m_write_batch.numops) {
            throw Exception("MSRIOImp::write_batch(): input vector smaller than configured number of operations",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        open_msr_batch();
#ifdef GEOPM_ENABLE_MSRSAFE_IOCTL_WRITE
        if (m_is_batch_enabled) {
            uint32_t batch_idx = 0;
            for (auto raw_it = raw_value.begin();
                 batch_idx != m_write_batch.numops;
                 ++raw_it, ++batch_idx) {
                m_write_batch.ops[batch_idx].msrdata = *raw_it;
            }
            msr_ioctl(false);
        }
        else
#endif
        {
            uint32_t batch_idx = 0;
            for (auto raw_it = raw_value.begin();
                 batch_idx != m_write_batch.numops;
                 ++raw_it, ++batch_idx) {
                write_msr(m_write_batch_op[batch_idx].cpu,
                          m_write_batch_op[batch_idx].msr,
                          *raw_it,
                          m_write_batch_op[batch_idx].wmask);
            }
        }
    }

    int MSRIOImp::msr_desc(int cpu_idx)
    {
        if (cpu_idx < 0 || cpu_idx > m_num_cpu) {
            throw Exception("MSRIOImp::msr_desc(): cpu_idx=" + std::to_string(cpu_idx) +
                            " out of range, num_cpu=" + std::to_string(m_num_cpu),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        open_msr(cpu_idx);
        return m_file_desc[cpu_idx];
    }

    int MSRIOImp::msr_batch_desc()
    {
        return m_file_desc[m_num_cpu];
    }

    void MSRIOImp::msr_path(int cpu_idx,
                            int fallback_idx,
                            std::string &path)
    {
        std::ostringstream path_ss;
        path_ss << "/dev/cpu/" << cpu_idx;
        switch (fallback_idx) {
            case M_FALLBACK_MSRSAFE:
                path_ss << "/msr_safe";
                break;
            case M_FALLBACK_MSR:
                path_ss << "/msr";
                break;
            default:
                throw Exception("MSRIOImp::msr_path(): Failed to open any of the options for reading msr values",
                                GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);
                break;
        }
        path = path_ss.str();
    }

    void MSRIOImp::msr_batch_path(std::string &path)
    {
        path = "/dev/cpu/msr_batch";
    }

    void MSRIOImp::open_msr(int cpu_idx)
    {
        for (int fallback_idx = 0;
             m_file_desc[cpu_idx] == -1;
             ++fallback_idx) {
            std::string path;
            msr_path(cpu_idx, fallback_idx, path);
            m_file_desc[cpu_idx] = open(path.c_str(), O_RDWR);
        }
        struct stat stat_buffer;
        int err = fstat(m_file_desc[cpu_idx], &stat_buffer);
        if (err) {
            throw Exception("MSRIOImp::open_msr(): file descriptor invalid",
                            GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);
        }
    }

    void MSRIOImp::open_msr_batch(void)
    {
        if (m_is_batch_enabled && m_file_desc[m_num_cpu] == -1) {
            std::string path;
            msr_batch_path(path);
            m_file_desc[m_num_cpu] = open(path.c_str(), O_RDWR);
            if (m_file_desc[m_num_cpu] == -1) {
                m_is_batch_enabled = false;
            }
        }
        if (m_is_batch_enabled) {
            struct stat stat_buffer;
            int err = fstat(m_file_desc[m_num_cpu], &stat_buffer);
            if (err) {
                throw Exception("MSRIOImp::open_msr_batch(): file descriptor invalid",
                                GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);
            }
        }
    }

    void MSRIOImp::close_msr(int cpu_idx)
    {
        if (m_file_desc[cpu_idx] != -1) {
            (void)close(m_file_desc[cpu_idx]);
            m_file_desc[cpu_idx] = -1;
        }
    }

    void MSRIOImp::close_msr_batch(void)
    {
        if (m_file_desc[m_num_cpu] != -1) {
            (void)close(m_file_desc[m_num_cpu]);
            m_file_desc[m_num_cpu] = -1;
        }
    }
}

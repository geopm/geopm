/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
#include "config.h"

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
#include "geopm_debug.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "MSRPath.hpp"

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
        : MSRIOImp(geopm_sched_num_cpu(), std::make_shared<MSRPath>())
    {

    }

    MSRIOImp::MSRIOImp(int num_cpu, std::shared_ptr<MSRPath> path)
        : m_num_cpu(num_cpu)
        , m_file_desc(m_num_cpu + 1, -1) // Last file descriptor is for the batch file
        , m_is_batch_enabled(true)
        , m_read_batch({0, NULL})
        , m_write_batch({0, NULL})
        , m_read_batch_op(0)
        , m_write_batch_op(0)
        , m_is_batch_read(false)
        , m_read_batch_idx_map(m_num_cpu)
        , m_write_batch_idx_map(m_num_cpu)
        , m_is_open(false)
        , m_path(path)
    {
        open_all();
    }

    MSRIOImp::~MSRIOImp()
    {
        close_all();
    }

    void MSRIOImp::open_all(void)
    {
        if (!m_is_open) {
            for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
                open_msr(cpu_idx);
            }
            open_msr_batch();
            m_is_open = true;
        }
    }

    void MSRIOImp::close_all(void)
    {
        if (m_is_open) {
            close_msr_batch();
            for (int cpu_idx = m_num_cpu - 1; cpu_idx != -1; --cpu_idx) {
                close_msr(cpu_idx);
            }
            m_is_open = false;
        }
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

    uint64_t MSRIOImp::system_write_mask(uint64_t offset)
    {
        if (!m_is_batch_enabled) {
            return ~0ULL;
        }
        uint64_t result = 0;
        auto off_it = m_offset_mask_map.find(offset);
        if (off_it != m_offset_mask_map.end()) {
            result = off_it->second;
        }
        else {
            m_msr_batch_op_s wr {
                .cpu = 0,
                .isrdmsr = 1,
                .err = 0,
                .msr = (uint32_t)offset,
                .msrdata = 0,
                .wmask = 0,
            };
            m_msr_batch_array_s arr {
                .numops = 1,
                .ops = &wr,
            };
            int err = ioctl(msr_batch_desc(), GEOPM_IOC_MSR_BATCH, &arr);
            if (err || wr.err) {
                throw Exception("MSRIOImp::system_write_mask(): read of mask failed",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_offset_mask_map[offset] = wr.wmask;
            result = wr.wmask;
        }
        return result;
   }

    int MSRIOImp::add_write(int cpu_idx, uint64_t offset)
    {
        int result = -1;
        auto batch_it = m_write_batch_idx_map.at(cpu_idx).find(offset);
        if (batch_it == m_write_batch_idx_map[cpu_idx].end()) {
            result = m_write_batch_op.size();
            m_msr_batch_op_s wr {
                .cpu = (uint16_t)cpu_idx,
                .isrdmsr = 1,
                .err = 0,
                .msr = (uint32_t)offset,
                .msrdata = 0,
                .wmask = system_write_mask(offset),
            };
            m_write_batch_op.push_back(wr);
            m_write_val.push_back(0);
            m_write_mask.push_back(0);  // will be widened to match writes by adjust()
            m_write_batch_idx_map[cpu_idx][offset] = result;
        }
        else {
            result = batch_it->second;
        }
        return result;
    }

    void MSRIOImp::adjust(int batch_idx, uint64_t raw_value, uint64_t write_mask)
    {
        if (batch_idx < 0 || (size_t)batch_idx >= m_write_batch_op.size()) {
            throw Exception("MSRIOImp::adjust(): batch_idx out of range: " + std::to_string(batch_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_write_batch_op.size() == m_write_val.size() &&
                           m_write_batch_op.size() == m_write_mask.size(),
                           "Size of member vectors does not match");
        uint64_t wmask_sys = m_write_batch_op[batch_idx].wmask;
        if ((~wmask_sys & write_mask) != 0ULL) {
            throw Exception("MSRIOImp::adjust(): write_mask is out of bounds",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if ((raw_value & write_mask) != raw_value) {
            std::ostringstream err_str;
            err_str << "MSRIOImp::adjust(): raw_value does not obey write_mask, "
                    << "raw_value=0x" << std::hex << raw_value
                    << " write_mask=0x" << write_mask;
            throw Exception(err_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_write_val[batch_idx] &= ~write_mask;
        m_write_val[batch_idx] |= raw_value;
        m_write_mask[batch_idx] |= write_mask;
    }

    int MSRIOImp::add_read(int cpu_idx, uint64_t offset)
    {
        /// @todo return same index for repeated calls with same inputs.
        m_msr_batch_op_s rd {
            .cpu = (uint16_t)cpu_idx,
            .isrdmsr = 1,
            .err = 0,
            .msr = (uint32_t)offset,
            .msrdata = 0,
            .wmask = 0
        };
        int idx = m_read_batch_op.size();
        m_read_batch_op.push_back(rd);
        return idx;
    }

    uint64_t MSRIOImp::sample(int batch_idx) const
    {
        if (!m_is_batch_read) {
            throw Exception("MSRIOImp::sample(): cannot call sample() before read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_read_batch.ops[batch_idx].msrdata;
    }


    void MSRIOImp::msr_ioctl(struct m_msr_batch_array_s &batch)
    {
        int err = ioctl(msr_batch_desc(), GEOPM_IOC_MSR_BATCH, &batch);
        if (err) {
            err = errno ? errno : err;
            std::ostringstream err_str;
            err_str << "MSRIOImp::msr_ioctl(): call to ioctl() for /dev/cpu/msr_batch failed: "
                    << " system error: " << strerror(err);
            throw Exception(err_str.str(), GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
        for (uint32_t batch_idx = 0; batch_idx != batch.numops; ++batch_idx) {
            err = batch.ops[batch_idx].err;
            if (err) {
                auto offset = batch.ops[batch_idx].msr;
                std::ostringstream err_str;
                err_str << "MSRIOImp::msr_ioctl(): operation failed at offset 0x"
                        << std::hex << offset
                        << " system error: " << strerror(err);
                throw Exception(err_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
            }
        }
    }

   void MSRIOImp::msr_ioctl_read(void)
    {
        if (m_read_batch.numops == 0) {
            return;
        }
        GEOPM_DEBUG_ASSERT(m_read_batch.numops == m_read_batch_op.size() &&
                           m_read_batch.ops == m_read_batch_op.data(),
                           "Batch operations not updated prior to calling MSRIOImp::msr_ioctl_read()");
        msr_ioctl(m_read_batch);
    }

    void MSRIOImp::msr_ioctl_write(void)
    {
        if (m_write_batch.numops == 0) {
            return;
        }
        GEOPM_DEBUG_ASSERT(m_write_batch.numops == m_write_batch_op.size() &&
                           m_write_batch.ops == m_write_batch_op.data(),
                           "Batch operations not updated prior to calling MSRIOImp::msr_ioctl_write()");
        msr_ioctl(m_write_batch);
        // Modify with write mask
        auto val_it = m_write_val.begin();
        auto mask_it = m_write_mask.begin();
        for (auto &op_it : m_write_batch_op) {
            op_it.isrdmsr = 0;
            op_it.msrdata &= ~*mask_it;
            op_it.msrdata |= *val_it;
            GEOPM_DEBUG_ASSERT((~op_it.wmask & *mask_it) == 0ULL,
                               "Write mask violation at write time");
            ++val_it;
            ++mask_it;
        }
        msr_ioctl(m_write_batch);
        for (auto &op_it : m_write_batch_op) {
            op_it.isrdmsr = 1;
        }
    }

    void MSRIOImp::read_batch(void)
    {
        m_read_batch.numops = m_read_batch_op.size();
        m_read_batch.ops = m_read_batch_op.data();

        if (m_is_batch_enabled) {
            msr_ioctl_read();
        }
        else {
            for (uint32_t batch_idx = 0;
                 batch_idx != m_read_batch.numops;
                 ++batch_idx) {
                m_read_batch.ops[batch_idx].msrdata =
                    read_msr(m_read_batch_op[batch_idx].cpu,
                             m_read_batch_op[batch_idx].msr);
            }
        }
        m_is_batch_read = true;
    }

    void MSRIOImp::write_batch(void)
    {
        m_write_batch.numops = m_write_batch_op.size();
        m_write_batch.ops = m_write_batch_op.data();

        if (m_is_batch_enabled) {
            msr_ioctl_write();
        }
        else {
            for (uint32_t batch_idx = 0;
                 batch_idx != m_write_batch.numops;
                 ++batch_idx) {
                write_msr(m_write_batch_op[batch_idx].cpu,
                          m_write_batch_op[batch_idx].msr,
                          m_write_val.at(batch_idx),
                          m_write_mask.at(batch_idx));
            }
        }
        std::fill(m_write_val.begin(), m_write_val.end(), 0ULL);
        std::fill(m_write_mask.begin(), m_write_mask.end(), 0ULL);
        m_is_batch_read = true;
    }

    int MSRIOImp::msr_desc(int cpu_idx)
    {
        if (cpu_idx < 0 || cpu_idx > m_num_cpu) {
            throw Exception("MSRIOImp::msr_desc(): cpu_idx=" + std::to_string(cpu_idx) +
                            " out of range, num_cpu=" + std::to_string(m_num_cpu),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_file_desc[cpu_idx];
    }

    int MSRIOImp::msr_batch_desc()
    {
        return m_file_desc[m_num_cpu];
    }

    void MSRIOImp::open_msr(int cpu_idx)
    {
        for (int fallback_idx = 0;
             m_file_desc[cpu_idx] == -1;
             ++fallback_idx) {
            std::string path = m_path->msr_path(cpu_idx, fallback_idx);
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
            std::string path = m_path->msr_batch_path();
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

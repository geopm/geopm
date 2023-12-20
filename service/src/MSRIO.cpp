/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

#include "geopm_error.h"
#include "geopm_sched.h"
#include "geopm_debug.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "IOUring.hpp"
#include "MSRPath.hpp"

#define GEOPM_IOC_MSR_BATCH _IOWR('c', 0xA2, struct geopm::MSRIOImp::m_msr_batch_array_s)

namespace geopm
{
    std::unique_ptr<MSRIO> MSRIO::make_unique(int driver_type)
    {
        return std::make_unique<MSRIOImp>(std::make_unique<MSRPath>(driver_type));
    }

    MSRIOImp::MSRIOImp(std::shared_ptr<MSRPath> path)
        : MSRIOImp(platform_topo().num_domain(GEOPM_DOMAIN_CPU), path, nullptr, nullptr)
    {

    }

    MSRIOImp::MSRIOImp(int num_cpu, std::shared_ptr<MSRPath> path,
                       std::shared_ptr<IOUring> batch_reader,
                       std::shared_ptr<IOUring> batch_writer)
        : m_num_cpu(num_cpu)
        , m_file_desc(m_num_cpu + 1, -1) // Last file descriptor is for the batch file
        , m_is_batch_enabled(true)
        , m_is_open(false)
        , m_path(std::move(path))
        , m_batch_reader(std::move(batch_reader))
        , m_batch_writer(std::move(batch_writer))
    {
        create_batch_context();
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

    int MSRIOImp::create_batch_context(void)
    {
        int ctx = static_cast<int>(m_batch_context.size());
        m_batch_context.emplace_back(m_num_cpu);
        return ctx;
    }

    int MSRIOImp::add_write(int cpu_idx, uint64_t offset)
    {
        return add_write(cpu_idx, offset, 0);
    }

    int MSRIOImp::add_write(int cpu_idx, uint64_t offset, int batch_ctx)
    {
        m_batch_context_s &ctx = m_batch_context.at(batch_ctx);
        int result = -1;
        auto &context = ctx.m_write_batch_idx_map.at(cpu_idx);
        auto batch_it = context.find(offset);
        if (batch_it == context.end()) {
            result = ctx.m_write_batch_op.size();
            m_msr_batch_op_s wr {
                .cpu = (uint16_t)cpu_idx,
                .isrdmsr = 1,
                .err = 0,
                .msr = (uint32_t)offset,
                .msrdata = 0,
                .wmask = system_write_mask(offset),
            };
            ctx.m_write_batch_op.push_back(wr);
            ctx.m_write_val.push_back(0);
            ctx.m_write_mask.push_back(0);  // will be widened to match writes by adjust()
            ctx.m_write_batch_idx_map[cpu_idx][offset] = result;
        }
        else {
            result = batch_it->second;
        }
        return result;
    }

    void MSRIOImp::adjust(int batch_idx, uint64_t raw_value, uint64_t write_mask)
    {
        adjust(batch_idx, raw_value, write_mask, 0);
    }

    void MSRIOImp::adjust(int batch_idx, uint64_t raw_value, uint64_t write_mask, int batch_ctx)
    {
        m_batch_context_s &ctx = m_batch_context.at(batch_ctx);
        if (batch_idx < 0 || (size_t)batch_idx >= ctx.m_write_batch_op.size()) {
            throw Exception("MSRIOImp::adjust(): batch_idx out of range: " + std::to_string(batch_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(ctx.m_write_batch_op.size() == ctx.m_write_val.size() &&
                           ctx.m_write_batch_op.size() == ctx.m_write_mask.size(),
                           "Size of member vectors does not match");
        uint64_t wmask_sys = ctx.m_write_batch_op[batch_idx].wmask;
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
        ctx.m_write_val[batch_idx] &= ~write_mask;
        ctx.m_write_val[batch_idx] |= raw_value;
        ctx.m_write_mask[batch_idx] |= write_mask;
    }

    int MSRIOImp::add_read(int cpu_idx, uint64_t offset)
    {
        return add_read(cpu_idx, offset, 0);
    }

    int MSRIOImp::add_read(int cpu_idx, uint64_t offset, int batch_ctx)
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
        m_batch_context_s &ctx = m_batch_context.at(batch_ctx);
        int idx = ctx.m_read_batch_op.size();
        ctx.m_read_batch_op.push_back(rd);
        return idx;
    }

    uint64_t MSRIOImp::sample(int batch_idx) const
    {
        return sample(batch_idx, 0);
    }

    uint64_t MSRIOImp::sample(int batch_idx, int batch_ctx) const
    {
        const m_batch_context_s &ctx = m_batch_context.at(batch_ctx);

        if (!ctx.m_is_batch_read) {
            throw Exception("MSRIOImp::sample(): cannot call sample() before read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return ctx.m_read_batch.ops[batch_idx].msrdata;
    }


    void MSRIOImp::msr_ioctl(struct m_msr_batch_array_s &batch)
    {
        int err = ioctl(msr_batch_desc(), GEOPM_IOC_MSR_BATCH, &batch);
        std::string err_msg = errno ? strerror(errno) : "";
        if (err == -1) {
            std::ostringstream err_str;
            err_str << "MSRIOImp::msr_ioctl(): call to ioctl() for /dev/cpu/msr_batch failed: "
                    << " system error: " << err_msg;
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

   void MSRIOImp::msr_ioctl_read(struct m_batch_context_s &ctx)
    {
        if (ctx.m_read_batch.numops == 0) {
            return;
        }
        GEOPM_DEBUG_ASSERT(ctx.m_read_batch.numops == ctx.m_read_batch_op.size() &&
                           ctx.m_read_batch.ops == ctx.m_read_batch_op.data(),
                           "Batch operations not updated prior to calling MSRIOImp::msr_ioctl_read()");
        msr_ioctl(ctx.m_read_batch);
    }

    void MSRIOImp::msr_ioctl_write(struct m_batch_context_s &ctx)
    {
        if (ctx.m_write_batch.numops == 0) {
            return;
        }
        GEOPM_DEBUG_ASSERT(ctx.m_write_batch.numops == ctx.m_write_batch_op.size() &&
                           ctx.m_write_batch.ops == ctx.m_write_batch_op.data(),
                           "MSRIOImp::msr_ioctl_write(): Batch operations not updated prior to calling");
        if (ctx.m_write_val.size() != ctx.m_write_batch.numops ||
            ctx.m_write_mask.size() != ctx.m_write_batch.numops ||
            ctx.m_write_batch_op.size() != ctx.m_write_batch.numops) {
            throw Exception("MSRIOImp::msr_ioctl_write(): Invalid operations stored in object, incorrectly sized",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        msr_ioctl(ctx.m_write_batch);
        // Modify with write mask
        int op_idx = 0;
        for (auto &op_it : ctx.m_write_batch_op) {
            op_it.isrdmsr = 0;
            op_it.msrdata &= ~ctx.m_write_mask[op_idx];
            op_it.msrdata |= ctx.m_write_val[op_idx];
            GEOPM_DEBUG_ASSERT((~op_it.wmask & ctx.m_write_mask[op_idx]) == 0ULL,
                               "MSRIOImp::msr_ioctl_write(): Write mask violation at write time");
            ++op_idx;
        }
        msr_ioctl(ctx.m_write_batch);
        for (auto &op_it : ctx.m_write_batch_op) {
            op_it.isrdmsr = 1;
        }
    }

    void MSRIOImp::msr_read_files(int batch_ctx)
    {
        auto &read_batch = m_batch_context.at(batch_ctx).m_read_batch;
        if (read_batch.numops == 0) {
            return;
        }
        GEOPM_DEBUG_ASSERT(read_batch.numops == m_batch_context.at(batch_ctx).m_read_batch_op.size() &&
                           read_batch.ops == m_batch_context.at(batch_ctx).m_read_batch_op.data(),
                           "Batch operations not updated prior to calling "
                           "MSRIOImp::msr_read_files()");

        if (!m_batch_reader) {
            m_batch_reader = IOUring::make_unique(read_batch.numops);
        }
        msr_batch_io(*m_batch_reader, read_batch);
    }

    void MSRIOImp::msr_batch_io(IOUring &batcher,
                                struct m_msr_batch_array_s &batch)
    {
        std::vector<std::shared_ptr<int> > return_values;
        return_values.reserve(batch.numops);

        for (uint32_t batch_idx = 0; batch_idx != batch.numops; ++batch_idx) {
            return_values.emplace_back(new int(0));
            auto& batch_op = batch.ops[batch_idx];
            if (batch_op.isrdmsr) {
                batcher.prep_read(return_values.back(), msr_desc(batch_op.cpu),
                                  &batch_op.msrdata, sizeof(batch_op.msrdata),
                                  batch_op.msr);
            }
            else {
                batcher.prep_write(return_values.back(), msr_desc(batch_op.cpu),
                                   &batch_op.msrdata, sizeof(batch_op.msrdata),
                                   batch_op.msr);
            }
        }

        batcher.submit();

        for (uint32_t batch_idx = 0; batch_idx != batch.numops; ++batch_idx) {
            ssize_t successful_bytes = *return_values[batch_idx];
            auto& batch_op = batch.ops[batch_idx];
            if (successful_bytes != sizeof(batch_op.msrdata)) {
                std::ostringstream err_str;
                err_str << "MSRIOImp::msr_batch_io(): failed at offset 0x"
                        << std::hex << batch_op.msr
                        << " system error: "
                        << ((successful_bytes < 0) ? strerror(-successful_bytes) : "none");
                throw Exception(err_str.str(), batch_op.isrdmsr
                                ? GEOPM_ERROR_MSR_READ : GEOPM_ERROR_MSR_WRITE,
                                __FILE__, __LINE__);
            }
        }
    }

    void MSRIOImp::msr_rmw_files(int batch_ctx)
    {
        auto &write_batch = m_batch_context.at(batch_ctx).m_write_batch;
        auto &write_batch_op = m_batch_context.at(batch_ctx).m_write_batch_op;
        auto &write_mask = m_batch_context.at(batch_ctx).m_write_mask;
        auto &write_val = m_batch_context.at(batch_ctx).m_write_val;
        if (write_batch.numops == 0) {
            return;
        }
        GEOPM_DEBUG_ASSERT(write_batch.numops == write_batch_op.size() &&
                           write_batch.ops == write_batch_op.data(),
                           "Batch operations not updated prior to calling "
                           "MSRIOImp::msr_rmw_files()");

        if (!m_batch_writer) {
            m_batch_writer = IOUring::make_unique(write_batch.numops);
        }

        // Read existing MSR values
        msr_batch_io(*m_batch_writer, write_batch);

        // Modify with write mask
        int op_idx = 0;
        for (auto &op_it : write_batch_op) {
            op_it.isrdmsr = 0;
            op_it.msrdata &= ~write_mask[op_idx];
            op_it.msrdata |= write_val[op_idx];
            GEOPM_DEBUG_ASSERT((~op_it.wmask & write_mask[op_idx]) == 0ULL,
                               "MSRIOImp::msr_rmw_files(): Write mask "
                               "violation at write time");
            ++op_idx;
        }

        // Write back the modified MSRs
        msr_batch_io(*m_batch_writer, write_batch);
        for (auto &op_it : write_batch_op) {
            op_it.isrdmsr = 1;
        }
    }

    void MSRIOImp::read_batch(void)
    {
        read_batch(0);
    }

    void MSRIOImp::read_batch(int batch_ctx)
    {
        m_batch_context_s &ctx = m_batch_context.at(batch_ctx);
        ctx.m_read_batch.numops = ctx.m_read_batch_op.size();
        ctx.m_read_batch.ops = ctx.m_read_batch_op.data();

        // Use the batch-oriented MSR-safe ioctl if possible. Otherwise, operate
        // over individual read operations per MSR.
        if (m_is_batch_enabled) {
            msr_ioctl_read(ctx);
        }
        else {
            msr_read_files(batch_ctx);
        }
        ctx.m_is_batch_read = true;
    }

    void MSRIOImp::write_batch(void)
    {
        write_batch(0);
    }

    void MSRIOImp::write_batch(int batch_ctx)
    {
        m_batch_context_s &ctx = m_batch_context.at(batch_ctx);
        ctx.m_write_batch.numops = ctx.m_write_batch_op.size();
        ctx.m_write_batch.ops = ctx.m_write_batch_op.data();

        // Use the batch-oriented MSR-safe ioctl twice (batch-read, modify,
        // batch-write) if possible. Otherwise, operate over individual
        // read-modify-write operations per MSR.
        if (m_is_batch_enabled) {
            msr_ioctl_write(ctx);
        }
        else {
            msr_rmw_files(batch_ctx);
        }
        std::fill(ctx.m_write_val.begin(), ctx.m_write_val.end(), 0ULL);
        std::fill(ctx.m_write_mask.begin(), ctx.m_write_mask.end(), 0ULL);
        ctx.m_is_batch_read = true;
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
        if (m_file_desc[cpu_idx] == -1) {
            std::string path = m_path->msr_path(cpu_idx);
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

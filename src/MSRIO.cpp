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

#include <sstream>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>

#include "Exception.hpp"
#include "MSRIO.hpp"

#define  GEOPM_IOC_MSR_BATCH _IOWR('c', 0xA2, struct MSRIO::m_msr_batch_array)

#if 0
namespace geopm
{
    MSRIO::MSRIO(const std::map<std::string, struct m_msr_signal_entry> *signal_map,
                         const std::map<std::string, std::pair<off_t, uint64_t> > *control_map,
                         const PlatformTopology &topo)
        : m_msr_path("")
        , m_cpu_file_desc(0)
        , m_msr_batch_desc(-1)
        , m_is_batch_enabled(false)
        , m_read_batch({0,NULL})
        , m_write_batch({0,NULL})
        , m_read_batch_op(0)
        , m_write_batch_op(0)
        , m_num_logical_cpu(0)
        , m_num_package(0)
        , m_msr_signal_map_ptr(signal_map)
        , m_msr_control_map_ptr(control_map)
    {
        m_num_logical_cpu = topo.num_domain(GEOPM_DOMAIN_CPU);
        m_num_package = topo.num_domain(GEOPM_DOMAIN_PACKAGE);
        for (int i = 0; i < m_num_logical_cpu; i++) {
            msr_open(i);
        }
    }

    MSRIO::~MSRIO()
    {

    }

    uint64_t MSRIO::offset(const std::string &msr_name)
    {
        uint64_t off = 0;
        auto control_it = m_msr_control_map_ptr->find(msr_name);
        if (control_it != m_msr_control_map_ptr->end()) {
            off =  (*control_it).second.first;
        }
        else {
            auto signal_it = m_msr_signal_map_ptr->find(msr_name);
            if (signal_it != m_msr_signal_map_ptr->end()) {
                off = (*signal_it).second.offset;
            }
            else {
                throw Exception("MSRIO::offset(): Invalid MSR name", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return off;
    }

    uint64_t MSRIO::write_mask(const std::string &msr_name)
    {
        uint64_t mask = 0;
        auto control_it = m_msr_control_map_ptr->find(msr_name);
        if (control_it != m_msr_control_map_ptr->end()) {
            mask =  (*control_it).second.second;
        }
        else {
            auto signal_it = m_msr_signal_map_ptr->find(msr_name);
            if (signal_it != m_msr_signal_map_ptr->end()) {
                mask = (*signal_it).second.write_mask;
            }
            else {
                throw Exception("MSRIO::offset(): Invalid MSR name", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return mask;
    }

    uint64_t MSRIO::read(int cpu_id, uint64_t offset)
    {
        uint64_t raw_value;

        if (m_cpu_file_desc.size() < (uint64_t)cpu_id) {
            throw Exception("MSRIO::read(): No file descriptor found for cpu device", GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }
        int rv = pread(m_cpu_file_desc[cpu_id], &raw_value, sizeof(raw_value), offset);
        if (rv != sizeof(raw_value)) {
            throw Exception(std::to_string(offset), GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
        }

        return raw_value;
    }

    void MSRIO::write(int cpu_id, uint64_t offset, uint64_t write_mask, uint64_t raw_value)
    {
        uint64_t old_value;
        uint64_t curr_value;

        curr_value = read(cpu_id, offset);
        curr_value &= ~write_mask;
        if (m_cpu_file_desc.size() < (uint64_t)cpu_id) {
            throw Exception("MSRIO::write(): No file descriptor found for cpu device", GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
        old_value = raw_value;
        curr_value &= write_mask;
        if (curr_value != old_value) {
            std::ostringstream message;
            message << "MSR value to be written was modified by the mask! Desired = 0x" << std::hex << old_value
                    << " After mask = 0x" << std::hex << curr_value;
            throw Exception(message.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
        raw_value |= curr_value;

        int rv = pwrite(m_cpu_file_desc[cpu_id], &raw_value, sizeof(raw_value), offset);
        if (rv != sizeof(raw_value)) {
            std::ostringstream ex_str;
            ex_str << "offset: " << offset << " value: " << raw_value;
            throw Exception(ex_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
    }

    void MSRIO::config_batch_read(const std::vector<int> &cpu, const std::vector<uint64_t> &read_offset)
    {
        if (cpu.size() != read_offset.size()) {
            throw Exception("MSRIO::config_batch_read(): Number of CPUs != Number of offsets", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_read_batch_op.resize(cpu.size());
        auto cpu_it = cpu.begin();
        auto offset_it = read_offset.begin();
        for (auto batch_it = m_read_batch_op.begin(); batch_it != m_read_batch_op.end(); ++batch_it) {
            batch_it->cpu = (*cpu_it);
            batch_it->isrdmsr = 1;
            batch_it->err = 0;
            batch_it->msr = (*offset_it);
            batch_it->msrdata = 0;
            batch_it->wmask = 0;
        }
        m_read_batch.numops = m_read_batch_op.size();
        m_read_batch.ops = m_read_batch_op.data();
    }

    void MSRIO::config_batch_write(const std::vector<int> &cpu, const std::vector<uint64_t> &write_offset, const std::vector<uint64_t> &write_mask)
    {
        if (cpu.size() != write_offset.size() ||
            cpu.size() != write_mask.size()) {
            throw Exception("MSRIO::config_batch_write(): Number of CPUs, number of offsets, and number of masks do not match", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_write_batch_op.resize(cpu.size());
        auto cpu_it = cpu.begin();
        auto offset_it = write_offset.begin();
        auto mask_it = write_mask.begin();
        for (auto batch_it = m_write_batch_op.begin(); batch_it != m_write_batch_op.end(); ++batch_it) {
            batch_it->cpu = (*cpu_it);
            batch_it->isrdmsr = 0;
            batch_it->err = 0;
            batch_it->msr = (*offset_it);
            batch_it->msrdata = 0;
            batch_it->wmask = (*mask_it);
        }
        m_write_batch.numops = m_write_batch_op.size();
        m_write_batch.ops = m_write_batch_op.data();
    }

    void MSRIO::read_batch(std::vector<uint64_t> &raw_value)
    {
        if (m_is_batch_enabled) {
            int rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &m_read_batch);
            if (rv) {
                throw Exception("MSRIO::read_batch():Read from /dev/cpu/msr_batch failed", GEOPM_ERROR_MSR_READ, __FILE__, __LINE__);
            }
        }
        else {
            for (unsigned i = 0; i < m_read_batch.numops; ++i) {
                m_read_batch.ops[i].msrdata =
                    read(m_read_batch.ops[i].cpu, m_read_batch.ops[i].msr);
            }
        }
        raw_value.clear();
        if (raw_value.size() < m_read_batch.numops) {
            raw_value.resize(m_read_batch.numops);
        }
        int batch_idx = 0;
        for (auto raw_it = raw_value.begin(); raw_it != raw_value.end(); ++raw_it) {
            (*raw_it) = m_read_batch.ops[batch_idx++].msrdata;
        }
    }

    void MSRIO::write_batch(const std::vector<uint64_t> &raw_value)
    {
        if (raw_value.size() != m_write_batch.numops) {
            throw Exception("MSRIO::write_batch(): Number of values does not equal number of controls", GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
        }
        int batch_idx = 0;
        for (auto raw_it = raw_value.begin(); raw_it != raw_value.end(); ++raw_it) {
            m_write_batch.ops[batch_idx++].msrdata = (*raw_it);
        }
        if (m_is_batch_enabled) {
            int rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &m_write_batch);
            if (rv) {
                throw Exception("MSRIO::write_batch(): Write to /dev/cpu/msr_batch failed", GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
            }
        }
        else {
            for (unsigned i = 0; i < m_write_batch.numops; ++i) {
                write(m_write_batch.ops[i].cpu, m_write_batch.ops[i].msr,
                          m_write_batch.ops[i].wmask, m_write_batch.ops[i].msrdata);
            }
        }
    }

    void MSRIO::descriptor_path(int cpu_num)
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

    void MSRIO::msr_open(int cpu)
    {
        int fd;

        descriptor_path(cpu);
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

    void MSRIO::msr_close(int cpu)
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

    size_t MSRIO::num_raw_signal(void)
    {
        return m_read_batch.numops;
    }
}
#endif

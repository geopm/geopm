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

#include "geopm_plugin.h"
#include "Exception.hpp"
#include "SharedMemory.hpp"
#include "ProfileTable.hpp"

#include "TestPlugin.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr)
{
    int err = 0;
    Decider *decider = NULL;
    Platform *platform = NULL;
    PlatformImp *platform_imp = NULL;

    try {
        switch (plugin_type) {
            case GEOPM_PLUGIN_TYPE_DECIDER:
                decider = new DumbDecider;
                geopm_factory_register(factory, decider, dl_ptr);
                break;
            case GEOPM_PLUGIN_TYPE_PLATFORM:
                platform = new DumbPlatform;
                geopm_factory_register(factory, platform, dl_ptr);
                break;
            case GEOPM_PLUGIN_TYPE_PLATFORM_IMP:
                platform_imp = new ShmemFreqPlatformImp;
                geopm_factory_register(factory, platform_imp, dl_ptr);
                break;
        }
    }
    catch(...) {
        err = exception_handler(std::current_exception());
    }
    return err;
}

DumbDecider::DumbDecider()
    : m_name("dumb")
{

}

DumbDecider::~DumbDecider()
{

}

Decider *DumbDecider::clone() const
{
    return (Decider*)(new DumbDecider(*this));
}

bool DumbDecider::decider_supported(const std::string &description)
{
    return (description == m_name);
}

const std::string& DumbDecider::name(void) const
{
    return m_name;
}

bool DumbDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
{
    return false;
}

DumbPlatform::DumbPlatform()
    : m_name("dumb")
{

}

DumbPlatform::~DumbPlatform()
{

}

size_t DumbPlatform::capacity(void)
{
    return 0;
}

void DumbPlatform::sample(std::vector<struct geopm_msr_message_s> &msr_msg)
{

}

bool DumbPlatform::model_supported(int platform_id, const std::string &description) const
{
    return false;
}

void DumbPlatform::enforce_policy(uint64_t region_id, IPolicy &policy) const
{

}

int DumbPlatform::control_domain(void)
{
    return GEOPM_CONTROL_DOMAIN_POWER;
}

void DumbPlatform::bound(double &upper_bound, double &lower_bound)
{
    upper_bound = DBL_MAX;
    lower_bound = DBL_MIN;
}

void DumbPlatform::initialize(void)
{

}

ShmemFreqPlatformImp::ShmemFreqPlatformImp()
    : m_name("shmem_freq")
    , m_cpu_freq_shmem_key("/geopm_test_platform_shmem_freq")
    , m_cpu_freq_table_size(4096)
    , m_num_cpu(8)
    , m_cpu_freq_max(4000.0)
    , m_pkg_power_max(100.0)
    , m_dram_power_max(25.0)
    , m_pp0_power_max(m_pkg_power_max + m_dram_power_max)
    , m_inst_ratio(2.0)
    , m_llc_ratio(0.25)
    , m_cpu_freq_start(2500.0)
    , m_cpu_freq_shmem(m_cpu_freq_shmem_key, m_cpu_freq_table_size)
    , m_cpu_freq_table(m_cpu_freq_table_size, m_cpu_freq_shmem.pointer())
    , m_clock_count(m_num_cpu)
    , m_telemetry(m_num_cpu)
{
    geopm_time(&m_time_zero);
    m_time_last = m_time_zero;
    struct geopm_prof_message_s message;
    message.progress = m_cpu_freq_start;
    for (int i = 0; i < m_num_cpu; ++i) {
        m_cpu_freq_table.insert(i, message);
    }
}

ShmemFreqPlatformImp::~ShmemFreqPlatformImp()
{

}

bool ShmemFreqPlatformImp::model_supported(int platform_id)
{
    return true;
}

std::string ShmemFreqPlatformImp::platform_name(void)
{
    return m_name;
}

void ShmemFreqPlatformImp::msr_reset(void)
{

}

int ShmemFreqPlatformImp::power_control_domain(void) const
{
    return GEOPM_DOMAIN_PACKAGE;
}

int ShmemFreqPlatformImp::frequency_control_domain(void) const
{
    return GEOPM_DOMAIN_CPU;
}

int ShmemFreqPlatformImp::performance_counter_domain(void) const
{
    return GEOPM_DOMAIN_CPU;
}

void ShmemFreqPlatformImp::msr_initialize(void)
{

}

double ShmemFreqPlatformImp::read_signal(int device_type, int device_idx, int signal_type)
{
    struct geopm_time_s time_curr;
    std::vector<double> cpu_freq_curr(m_num_cpu);
    uint64_t clock_tick_delta;
    geopm_time(&time_curr);
    double time_delta = geopm_time_diff(&m_time_last, &time_curr);

    if (device_type != GEOPM_DOMAIN_CPU) {
        throw Exception("ShmemFreqPlatformImp::read_signal() can only be used to read CPU signals",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
        // Grab the clock frequencies from shared memory
        cpu_freq_curr[cpu_idx] = cpu_freq(cpu_idx);
    }
    for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
        clock_tick_delta = time_delta * cpu_freq_curr[cpu_idx];
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] += clock_tick_delta * m_pkg_power_max * cpu_freq_curr[cpu_idx] / m_cpu_freq_max;
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY] += clock_tick_delta * m_dram_power_max * cpu_freq_curr[cpu_idx] / m_cpu_freq_max;
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_FREQUENCY] = cpu_freq_curr[device_idx];
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_INST_RETIRED] = clock_tick_delta * m_inst_ratio * cpu_freq_curr[cpu_idx];
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE] += clock_tick_delta;
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF] += clock_tick_delta;
        m_telemetry[cpu_idx].signal[GEOPM_TELEMETRY_TYPE_READ_BANDWIDTH] += clock_tick_delta * m_llc_ratio;
    }
    m_time_last = time_curr;
    return m_telemetry[device_idx].signal[signal_type];
}

void ShmemFreqPlatformImp::batch_read_signal(std::vector<struct geopm_signal_descriptor> &signal_desc, bool is_changed)
{

}

void ShmemFreqPlatformImp::write_control(int device_type, int device_idx, int signal_type, double value)
{
    if (device_type != GEOPM_DOMAIN_CPU ||
        signal_type != GEOPM_TELEMETRY_TYPE_FREQUENCY) {
        throw Exception("ShmemFreqPlatformImp::write_control() can only be used to control CPU frequency", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    cpu_freq(device_idx, value);
}

void ShmemFreqPlatformImp::bound(int control_type, double &upper_bound, double &lower_bound)
{
    upper_bound = 4000.0 ;
    lower_bound = 2500.0;
}

double  ShmemFreqPlatformImp::throttle_limit_mhz(void) const
{
    return 0.5;
}


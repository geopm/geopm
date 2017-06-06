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

#ifndef XEONPLATFORMIMP_HPP_INCLUDE
#define XEONPLATFORMIMP_HPP_INCLUDE

#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#include "PlatformImp.hpp"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

namespace geopm
{
    /// @brief This class provides a base class for the Xeon processor line.
    class XeonPlatformImp : public PlatformImp
    {
        public:
            /// @brief Default constructor.
            XeonPlatformImp(int platform_id, const std::string &model_name, const std::map<std::string, std::pair<off_t, unsigned long> > *msr_map);
            /// @brief Copy constructor.
            XeonPlatformImp(const XeonPlatformImp &other);
            /// @brief Default destructor.
            virtual ~XeonPlatformImp();
            virtual bool model_supported(int platform_id);
            virtual std::string platform_name(void);
            virtual double read_signal(int device_type, int device_index, int signal_type);
            virtual void batch_read_signal(std::vector<struct geopm_signal_descriptor> &signal_desc, bool is_changed);
            virtual void write_control(int device_type, int device_index, int signal_type, double value);
            virtual void msr_initialize(void);
            virtual void msr_reset(void);
            virtual int power_control_domain(void) const;
            virtual int frequency_control_domain(void) const;
            virtual int performance_counter_domain(void) const;
            virtual double throttle_limit_mhz(void) const;

        protected:
            /// @brief Initialize Running Average Power Limiting (RAPL) controls.
            virtual void rapl_init(void);
            /// @brief Initialize per-CPU counters.
            virtual void cbo_counters_init(void);
            /// @brief Initialize free running counters.
            virtual void fixed_counters_init(void);
            /// @brief Reset per-CPU counters to default state.
            virtual void cbo_counters_reset(void);
            /// @brief Reset free running counters to default state.
            virtual void fixed_counters_reset(void);
            /// @brief Return the upper and lower bounds of the control.
            virtual void bound(int control_type, double &upper_bound, double &lower_bound);

            /// @brief Frequency where anything <= is considered throttling.
            double m_throttle_limit_mhz;
            /// @brief Store the units of energy read from RAPL.
            double m_energy_units;
            /// @brief Store the units of energy read from RAPL for dram.
            double m_dram_energy_units;
            /// @brief Store the inverse units of power read from RAPL.
            double m_power_units_inv;
            /// @brief Minimum value for package (CPU) power read from RAPL.
            double m_min_pkg_watts;
            /// @brief Maximum value for package (CPU) power read from RAPL.
            double m_max_pkg_watts;
            /// @brief Minimum value for DRAM power read from RAPL.
            double m_min_dram_watts;
            /// @brief Maximum value for DRAM power read from RAPL.
            double m_max_dram_watts;
            /// @brief Vector of MSR offsets for reading.
            std::vector<off_t> m_signal_msr_offset;
            ///@brief Vector of MSR data containing pairs of offsets and write masks.
            std::vector<std::pair<off_t, unsigned long> > m_control_msr_pair;
            uint64_t m_pkg_power_limit_static;

            ///Constants
            const unsigned int M_BOX_FRZ_EN;
            const unsigned int M_BOX_FRZ;
            const unsigned int M_CTR_EN;
            const unsigned int M_RST_CTRS;
            const unsigned int M_LLC_FILTER_MASK;
            const unsigned int M_LLC_VICTIMS_EV_SEL;
            const unsigned int M_LLC_VICTIMS_UMASK;
            const unsigned int M_EVENT_SEL_0;
            const unsigned int M_UMASK_0;
            const uint64_t M_DRAM_POWER_LIMIT_MASK;
            const int M_PLATFORM_ID;
            const std::string M_MODEL_NAME;
            const std::string M_TRIGGER_NAME;

            enum {
                M_RAPL_PKG_STATUS,
                M_RAPL_DRAM_STATUS,
                M_IA32_PERF_STATUS,
                M_INST_RETIRED,
                M_CLK_UNHALTED_CORE,
                M_CLK_UNHALTED_REF,
                M_LLC_VICTIMS,
            } m_signal_offset_e;
            enum {
                M_RAPL_PKG_LIMIT,
                M_RAPL_DRAM_LIMIT,
                M_IA32_PERF_CTL,
                M_NUM_CONTROL
            } m_control_offset_e;
            enum {
                M_PKG_STATUS_OVERFLOW,
                M_DRAM_STATUS_OVERFLOW,
                M_NUM_PACKAGE_OVERFLOW_OFFSET
            } m_package_overflow_offset_e;
            enum {
                M_PERF_STATUS_OVERFLOW,
                M_INST_RETIRED_OVERFLOW,
                M_CLK_UNHALTED_CORE_OVERFLOW,
                M_CLK_UNHALTED_REF_OVERFLOW,
                M_LLC_VICTIMS_OVERFLOW,
                M_NUM_SIGNAL_OVERFLOW_OFFSET
            } m_signal_overflow_offset_e;
    };


    /// @brief This class provides a concrete platform implementation of
    /// Sandybridge E processors: cpuid=0x62D.
    class SNBPlatformImp : public XeonPlatformImp
    {
        public:
            /// @brief Default constructor.
            SNBPlatformImp();
            SNBPlatformImp(int platform_id, const std::string &model_name);
            SNBPlatformImp(const SNBPlatformImp &other);
            /// @brief Default destructor.
            virtual ~SNBPlatformImp();
            virtual int frequency_control_domain(void) const;
            static int platform_id(void);
    };

    /// @brief This class provides a concrete platform implementation of
    /// IvyBridge E processors: cpuid=0x63E.
    class IVTPlatformImp : public SNBPlatformImp
    {
        public:
            /// @brief Default constructor.
            IVTPlatformImp();
            IVTPlatformImp(const IVTPlatformImp &other);
            /// @brief Default destructor.
            virtual ~IVTPlatformImp();
            virtual int frequency_control_domain(void) const;
            static int platform_id(void);
    };


    /// @brief This class provides a concrete platform implementation of
    /// Haswell E processors: cpuid=0x63F.
    class HSXPlatformImp : public XeonPlatformImp
    {
        public:
            /// @brief Default constructor.
            HSXPlatformImp();
            HSXPlatformImp(const HSXPlatformImp &other);
            HSXPlatformImp(int platform_id, const std::string &model_name);
            /// @brief Default destructor.
            virtual ~HSXPlatformImp();
            static int platform_id(void);
    };

    class BDXPlatformImp : public HSXPlatformImp
    {
        public:
            /// @brief Default constructor.
            BDXPlatformImp();
            BDXPlatformImp(const BDXPlatformImp &other);
            /// @brief Default destructor.
            virtual ~BDXPlatformImp();
            static int platform_id(void);
    };

}

#endif

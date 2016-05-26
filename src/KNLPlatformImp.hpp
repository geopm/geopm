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

#ifndef KNLPLATFORMIMP_HPP_INCLUDE
#define KNLPLATFORMIMP_HPP_INCLUDE

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#include "PlatformImp.hpp"

namespace geopm
{
    /// @brief This class provides a concrete platform implementation of
    /// Haswell E processors.
    /// (cpuid 0x63F).
    class KNLPlatformImp : public PlatformImp
    {
        public:
            /// @brief Default constructor.
            KNLPlatformImp();
            /// @brief Default destructor.
            virtual ~KNLPlatformImp();

            //////////////////////////////////////////////
            // KNLPlatformImp dependent implementations //
            //////////////////////////////////////////////
            virtual bool model_supported(int platform_id);
            virtual std::string platform_name();
            virtual double read_signal(int device_type, int device_index, int signal_type);
            virtual void write_control(int device_type, int device_index, int signal_type, double value);
            virtual void msr_initialize();
            virtual void msr_reset();
            virtual int power_control_domain(void) const;
            virtual int frequency_control_domain(void) const;
            virtual int performance_counter_domain(void) const;

        protected:
            /// @brief Load KNL specific MSR offsets into MSR offset map.
            void load_msr_offsets();
            /// @brief Initialize Running Average Power Limiting (RAPL) controls.
            void rapl_init();
            /// @brief Initialize per-CPU counters.
            void cbo_counters_init();
            /// @brief Initialize free running counters.
            void fixed_counters_init();
            /// @brief Reset RAPL controls to default state.
            void rapl_reset();
            /// @brief Reset per-CPU counters to default state.
            void cbo_counters_reset();
            /// @brief Reset free running counters to default state.
            void fixed_counters_reset();

            /// @brief Store the units of energy read from RAPL.
            double m_energy_units;
            /// @brief Store the units of power read from RAPL.
            double m_power_units;
            /// @brief Minimum value for package (CPU) power read from RAPL.
            double m_min_pkg_watts;
            /// @brief Maximum value for package (CPU) power read from RAPL.
            double m_max_pkg_watts;
            /// @brief Minimum value for power plane 0 (pkg+dram) read from RAPL.
            double m_min_pp0_watts;
            /// @brief Maximum value for power plane 0 (pkg+dram) read from RAPL.
            double m_max_pp0_watts;
            /// @brief Minimum value for DRAM power read from RAPL.
            double m_min_dram_watts;
            /// @brief Maximum value for DRAM power read from RAPL.
            double m_max_dram_watts;
            /// @brief Stores the platform identifier.
            int m_platform_id;
            /// @brief Vector of MSR offsets for reading.
            std::vector<off_t> m_signal_msr_offset;
            /// @brief Vector of MSR offsets to writing.
            std::vector<off_t> m_control_msr_offset;

            ///Constants
            const std::string M_KNL_MODEL_NAME;
            const unsigned int M_BOX_FRZ_EN;
            const unsigned int M_BOX_FRZ;
            const unsigned int M_CTR_EN;
            const unsigned int M_RST_CTRS;
            const unsigned int M_LLC_FILTER_MASK;
            const unsigned int M_LLC_VICTIMS_EV_SEL;
            const unsigned int M_LLC_VICTIMS_UMASK;
            const unsigned int M_EVENT_SEL_0;
            const unsigned int M_UMASK_0;
            const uint64_t M_PKG_POWER_LIMIT_MASK_MAGIC;
            const uint64_t M_DRAM_POWER_LIMIT_MASK_MAGIC;
            const uint64_t M_PP0_POWER_LIMIT_MASK_MAGIC;
    };
}

#endif

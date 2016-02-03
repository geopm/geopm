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

#ifndef HSXPLATFORMIMP_HPP_INCLUDE
#define HSXPLATFORMIMP_HPP_INCLUDE

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
    class HSXPlatformImp : public PlatformImp
    {
        public:
            /// @brief Default constructor.
            HSXPlatformImp();
            /// @brief Default destructor.
            virtual ~HSXPlatformImp();

            //////////////////////////////////////////////
            // HSXPlatformImp dependent implementations //
            //////////////////////////////////////////////
            virtual bool model_supported(int platform_id);
            virtual std::string platform_name();
            virtual void initialize_msrs();
            virtual void reset_msrs();
            virtual int power_control_domain(void) const;
            virtual int frequency_control_domain(void) const;

        protected:
            /// @brief Load HSX specific MSR offsets into MSR offset map.
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
    };
}

#endif

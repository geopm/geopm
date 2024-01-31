/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CPUID_HPP_INCLUDE
#define CPUID_HPP_INCLUDE

#include <memory>

namespace geopm
{
    class Cpuid
    {
        public:
            struct rdt_info_s
            {
                bool rdt_support;
                uint32_t rmid_bit_width;
                uint32_t mbm_scalar;
            };

            static std::unique_ptr<Cpuid> make_unique(void);
            Cpuid() = default;
            virtual ~Cpuid() = default;
            virtual int cpuid(void) const = 0;
            virtual bool is_hwp_supported(void) const = 0;
            virtual double freq_sticker(void) const = 0;
            virtual rdt_info_s rdt_info(void) const = 0;
            virtual uint32_t pmc_bit_width(void) const = 0;
    };
}

#endif

/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm/Cpuid.hpp"

#ifdef GEOPM_ENABLE_CPUID
#include <cpuid.h>
#include <cmath>

namespace geopm
{
    class CpuidImp : public Cpuid
    {
        public:
            CpuidImp() = default;
            virtual ~CpuidImp() = default;
            int cpuid(void) const override;
            bool is_hwp_supported(void) const override;
            double freq_sticker(void) const override;
            rdt_info_s rdt_info(void) const override;
            uint32_t pmc_bit_width(void) const override;
    };

    std::unique_ptr<Cpuid> Cpuid::make_unique(void)
    {
        return std::make_unique<CpuidImp>();
    }

    int CpuidImp::cpuid(void) const
    {
        uint32_t key = 1; //processor features
        uint32_t proc_info = 0;
        uint32_t model;
        uint32_t family;
        uint32_t ext_model;
        uint32_t ext_family;
        uint32_t ebx, ecx, edx;
        const uint32_t model_mask = 0xF0;
        const uint32_t family_mask = 0xF00;
        const uint32_t extended_model_mask = 0xF0000;
        const uint32_t extended_family_mask = 0xFF00000;

        __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

        model = (proc_info & model_mask) >> 4;
        family = (proc_info & family_mask) >> 8;
        ext_model = (proc_info & extended_model_mask) >> 16;
        ext_family = (proc_info & extended_family_mask) >> 20;

        if (family == 6) {
            model+=(ext_model << 4);
        }
        else if (family == 15) {
            model+=(ext_model << 4);
            family+=ext_family;
        }

        return ((family << 8) + model);
    }

    bool CpuidImp::is_hwp_supported(void) const
    {
        uint32_t leaf = 6; //thermal and power management features
        uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
        const uint32_t hwp_mask = 0x80;
        bool supported = false;
        __get_cpuid(leaf, &eax, &ebx, &ecx, &edx);

        supported = (bool)((eax & hwp_mask) >> 7);
        return supported;
    }

    Cpuid::rdt_info_s CpuidImp::rdt_info(void) const
    {
        uint32_t leaf, subleaf = 0;
        uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
        bool supported = false;
        uint32_t max, scale = 0;

        leaf = 0x0F;
        subleaf = 0;

        __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
        supported = (bool)((edx >> 1) & 1);
        max = ebx;

        if (supported == true) {
            subleaf = 1;
            eax = 0;
            ebx = 0;
            ecx = 0;
            edx = 0;
            __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
            scale = ebx;
        }

        rdt_info_s rdt = {
            .rdt_support = supported,
            .rmid_bit_width = (uint32_t)ceil(log2((double)max + 1)),
            .mbm_scalar = scale
        };

        return rdt;
    }

    uint32_t CpuidImp::pmc_bit_width(void) const
    {
        uint32_t leaf, subleaf = 0;
        uint32_t eax, ebx, ecx, edx = 0;

        leaf = 0x0A;
        subleaf = 0;

        __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);

        // SDM vol 3b, section 18 specifies where to find how many PMC bits are
        // available
        return (eax >> 16) & 0xff;
    }

    double CpuidImp::freq_sticker(void) const
    {
        double result = NAN;
        uint32_t leaf = 0x16; //processor frequency info
        uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
        const uint32_t sticker_mask = 0xFFFF;
        const uint32_t unit_factor = 1e6;

        __get_cpuid(leaf, &eax, &ebx, &ecx, &edx);

        result = (eax & sticker_mask) * unit_factor;

        return result;
    }
}

#else

namespace geopm
{
    class CpuidNull : public Cpuid
    {
        public:
            CpuidNull() = default;
            virtual ~CpuidNull() = default;
            int cpuid(void) const override;
            bool is_hwp_supported(void) const override;
            double freq_sticker(void) const override;
            rdt_info_s rdt_info(void) const override;
            uint32_t pmc_bit_width(void) const override;
    };

    std::unique_ptr<Cpuid> Cpuid::make_unique(void)
    {
        return std::make_unique<CpuidNull>();
    }

    int CpuidNull::cpuid(void) const
    {
        return 0;
    }

    bool CpuidNull::is_hwp_supported(void) const
    {
        return false;
    }

    Cpuid::rdt_info_s CpuidNull::rdt_info(void) const
    {
        rdt_info_s rdt = {
            .rdt_support = false,
            .rmid_bit_width = 0,
            .mbm_scalar = 0
        };

        return rdt;
    }

    uint32_t CpuidNull::pmc_bit_width(void) const
    {
        return 0;
    }

    double CpuidNull::freq_sticker(void) const
    {
        return 0.0;
    }
}

#endif

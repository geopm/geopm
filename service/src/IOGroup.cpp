/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include <unistd.h>

#include <functional>
#include <mutex>


#include "geopm/IOGroup.hpp"

#include "geopm_plugin.hpp"
#include "geopm/MSRIOGroup.hpp"
#include "CpuinfoIOGroup.hpp"
#include "TimeIOGroup.hpp"
#include "SSTIOGroup.hpp"
#include "geopm/Helper.hpp"
#include "CpufreqSysfsDriver.hpp"
#ifdef GEOPM_ENABLE_SYSTEMD
#include "ServiceIOGroup.hpp"
#endif
#ifdef GEOPM_CNL_IOGROUP
#include "CNLIOGroup.hpp"
#endif
#ifdef GEOPM_ENABLE_DCGM
#include "DCGMIOGroup.hpp"
#endif
#ifdef GEOPM_ENABLE_NVML
#include "NVMLIOGroup.hpp"
#endif
#ifdef GEOPM_ENABLE_LEVELZERO
#include "LevelZeroIOGroup.hpp"
#endif
#include "ConstConfigIOGroup.hpp"
#ifdef GEOPM_DEBUG
#include <iostream>
#endif


namespace geopm
{
    const std::string IOGroup::M_PLUGIN_PREFIX = "libgeopmiogroup_";

    const std::string IOGroup::M_UNITS[IOGroup::M_NUM_UNITS] = {
        "none",
        "seconds",
        "hertz",
        "watts",
        "joules",
        "celsius"
    };

    const std::string IOGroup::M_BEHAVIORS[IOGroup::M_NUM_SIGNAL_BEHAVIOR] = {
        "constant",
        "monotone",
        "variable",
        "label"
    };

    const std::map<std::string, IOGroup::m_units_e> IOGroup::M_UNITS_STRING = {
        {IOGroup::M_UNITS[M_UNITS_NONE], M_UNITS_NONE},
        {IOGroup::M_UNITS[M_UNITS_SECONDS], M_UNITS_SECONDS},
        {IOGroup::M_UNITS[M_UNITS_HERTZ], M_UNITS_HERTZ},
        {IOGroup::M_UNITS[M_UNITS_WATTS], M_UNITS_WATTS},
        {IOGroup::M_UNITS[M_UNITS_JOULES], M_UNITS_JOULES},
        {IOGroup::M_UNITS[M_UNITS_CELSIUS], M_UNITS_CELSIUS}
    };

    const std::map<std::string, IOGroup::m_signal_behavior_e> IOGroup::M_BEHAVIOR_STRING = {
        {IOGroup::M_BEHAVIORS[M_SIGNAL_BEHAVIOR_CONSTANT], M_SIGNAL_BEHAVIOR_CONSTANT},
        {IOGroup::M_BEHAVIORS[M_SIGNAL_BEHAVIOR_MONOTONE], M_SIGNAL_BEHAVIOR_MONOTONE},
        {IOGroup::M_BEHAVIORS[M_SIGNAL_BEHAVIOR_VARIABLE], M_SIGNAL_BEHAVIOR_VARIABLE},
        {IOGroup::M_BEHAVIORS[M_SIGNAL_BEHAVIOR_LABEL], M_SIGNAL_BEHAVIOR_LABEL},
    };


    IOGroupFactory::IOGroupFactory()
    {
        // Unless running as root add the ServiceIOGroup which will go
        // through D-Bus to access geopmd.  Note this IOGroup is
        // loaded first and provides all signals and controls
        // available from geopmd.  Any signal or control available
        // without using the service will be used preferentially
        // because this IOGroup is loaded first.  Also note that
        // creation of the ServiceIOGroup will open a session with the
        // service enabling save/restore by geopmd.  If the geopm
        // service is not active then loading the ServiceIOGroup will
        // fail.
        if (geopm::has_cap_sys_admin()) {
            // May want to give this higher priority than the non-safe
            // msr driver once it is considered more stable.
            register_plugin(CpufreqSysfsDriver::plugin_name(),
                            CpufreqSysfsDriver::make_plugin);
#ifdef GEOPM_ENABLE_CPUID
#ifdef GEOPM_ENABLE_RAWMSR
            // Only use /dev/cpu/*/msr if configured with
            // --enable-rawmsr and msr-safe driver is not available.
            bool use_msr_safe = (access("/dev/cpu/msr_batch", R_OK | W_OK) == 0);
            if (!use_msr_safe) {
                register_plugin(MSRIOGroup::plugin_name(),
                                MSRIOGroup::make_plugin);
            }
#endif
#endif
            register_plugin(SSTIOGroup::plugin_name(),
                            SSTIOGroup::make_plugin);
#ifdef GEOPM_ENABLE_LEVELZERO
            register_plugin(LevelZeroIOGroup::plugin_name(),
                            LevelZeroIOGroup::make_plugin);
#endif
#ifdef GEOPM_ENABLE_DCGM
            register_plugin(DCGMIOGroup::plugin_name(),
                            DCGMIOGroup::make_plugin);
#endif
#ifdef GEOPM_ENABLE_NVML
            register_plugin(NVMLIOGroup::plugin_name(),
                            NVMLIOGroup::make_plugin);
#endif
#ifdef GEOPM_ENABLE_CPUID
#ifdef GEOPM_ENABLE_RAWMSR
            // Always try to load msr-safe version of IOGroup unless
            // raw msr access has already been selected.
            if (use_msr_safe)
#endif
            {
                register_plugin(MSRIOGroup::plugin_name(),
                                MSRIOGroup::make_plugin_safe);
            }
#endif
        }
#ifdef GEOPM_ENABLE_SYSTEMD
        else { // not UID 0
            register_plugin(ServiceIOGroup::plugin_name(),
                            ServiceIOGroup::make_plugin);
        }
#endif
        register_plugin(TimeIOGroup::plugin_name(),
                        TimeIOGroup::make_plugin);
        register_plugin(CpuinfoIOGroup::plugin_name(),
                        CpuinfoIOGroup::make_plugin);
#ifdef GEOPM_CNL_IOGROUP
        register_plugin(CNLIOGroup::plugin_name(),
                        CNLIOGroup::make_plugin);
#endif
        register_plugin(ConstConfigIOGroup::plugin_name(),
                        ConstConfigIOGroup::make_plugin);
    }

    IOGroupFactory &iogroup_factory(void)
    {
        static IOGroupFactory instance;
        static bool is_once = true;
        static std::once_flag flag;
        if (is_once) {
            is_once = false;
            std::call_once(flag, plugin_load, IOGroup::M_PLUGIN_PREFIX);
        }
        return instance;
    }


    std::vector<std::string> IOGroup::iogroup_names(void)
    {
        return iogroup_factory().plugin_names();
    }


    std::unique_ptr<IOGroup> IOGroup::make_unique(const std::string &iogroup_name)
    {
        return iogroup_factory().make_plugin(iogroup_name);
    }


    std::function<std::string(double)> IOGroup::format_function(const std::string &signal_name) const
    {
#ifdef GEOPM_DEBUG
        static bool is_once = true;
        if (is_once) {
            is_once = false;
            std::cerr << "Warning: <geopm> Use of geopm::IOGroup::format_function() is deprecated, each IOGroup will be required implement this method in the future.\n";
        }
#endif
        std::function<std::string(double)> result = string_format_double;
        if (string_ends_with(signal_name, "#")) {
            result = string_format_raw64;
        }
        return result;
    }

    IOGroup::m_units_e IOGroup::string_to_units(const std::string &str)
    {
        auto it = M_UNITS_STRING.find(str);
        if (it == M_UNITS_STRING.end()) {
            throw Exception("IOGroup::string_to_units(): invalid units string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    std::string IOGroup::units_to_string(int uni)
    {
        if (uni >= IOGroup::M_NUM_UNITS || uni < 0) {
            throw Exception("IOGroup::units_to_string): invalid units value",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return IOGroup::M_UNITS[uni];
    }

    IOGroup::m_signal_behavior_e IOGroup::string_to_behavior(const std::string &str)
    {
        auto it = M_BEHAVIOR_STRING.find(str);
        if (it == M_BEHAVIOR_STRING.end()) {
            throw Exception("IOGroup::string_to_behavior(): invalid behavior string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }
}

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cmath>

#include "AliceIOGroup.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"

using geopm::Exception;
using geopm::PlatformTopo;

// Registers this IOGroup with the IOGroup factory, making it visible
// to PlatformIO when the plugin is first loaded.
static void __attribute__((constructor)) example_iogroup_load(void)
{
    geopm::iogroup_factory().register_plugin(AliceIOGroup::plugin_name(),
                                             AliceIOGroup::make_plugin);
}

std::string AliceIOGroup::name(void) const
{
    return plugin_name();
}

// Name used for registration with the IOGroup factory
std::string AliceIOGroup::plugin_name(void)
{
    return "alice";
}

// Function used by the factory to create objects of this type
std::unique_ptr<geopm::IOGroup> AliceIOGroup::make_plugin(void)
{
    return std::unique_ptr<geopm::IOGroup>(new AliceIOGroup);
}

// Set up mapping between signal names and corresponding indices
AliceIOGroup::AliceIOGroup()
    : m_signal_idx_map{{"FOO",  M_SIGNAL_FOO},
                       {"BAR",  M_SIGNAL_BAR},
                       {"TIME", M_SIGNAL_TIME}}
{

}

// A user-friendly description of each signal
std::string AliceIOGroup::signal_description(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("AliceIOGroup::signal_description(): signal_name " + signal_name +
                        " not valid for AliceIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    int signal_idx = m_signal_idx_map.at(signal_name);
    std::string result = "";
    switch (signal_idx) {
        case M_SIGNAL_FOO:
            result = "Alice's foo signal";
            break;
        case M_SIGNAL_BAR:
            result = "Alice's bar signal";
            break;
        case M_SIGNAL_TIME:
            result = "Alice's time signal";
        default:
            break;
    }
    return result;
}

// Extract the set of all signal names from the index map
std::set<std::string> AliceIOGroup::signal_names(void) const
{
    std::set<std::string> result;
    for (const auto &sv : m_signal_idx_map) {
        result.insert(sv.first);
    }
    return result;
}

std::set<std::string> AliceIOGroup::control_names(void) const
{
    std::set<std::string> result;
    return result;
}

bool AliceIOGroup::is_valid_signal(const std::string &signal_name) const
{
    return m_signal_idx_map.find(signal_name) != m_signal_idx_map.end();
}

bool AliceIOGroup::is_valid_control(const std::string &control_name) const
{
    return false;
}

int AliceIOGroup::signal_domain_type(const std::string &signal_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = GEOPM_DOMAIN_BOARD;
    }
    return result;
}

int AliceIOGroup::control_domain_type(const std::string &control_name) const
{
    return GEOPM_DOMAIN_INVALID;
}

int AliceIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    return -1;
}

int AliceIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
{
    return -1;
}

void AliceIOGroup::read_batch(void)
{

}

void AliceIOGroup::write_batch(void)
{

}

double AliceIOGroup::sample(int batch_idx)
{
    return NAN;
}

void AliceIOGroup::adjust(int batch_idx, double setting)
{

}

double AliceIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    return NAN;
}

void AliceIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
{

}

void AliceIOGroup::save_control(void)
{

}

void AliceIOGroup::save_control(const std::string &save_path)
{

}

void AliceIOGroup::restore_control(void)
{

}

void AliceIOGroup::restore_control(const std::string &save_path)
{

}

std::function<double(const std::vector<double> &)> AliceIOGroup::agg_function(const std::string &signal_name) const
{
    return geopm::Agg::average;
}

std::function<std::string(double)> AliceIOGroup::format_function(const std::string &signal_name) const
{
    return geopm::string_format_double;
}

std::string AliceIOGroup::control_description(const std::string &control_name) const
{
    return "";
}

int AliceIOGroup::signal_behavior(const std::string &signal_name) const
{
    return M_SIGNAL_BEHAVIOR_MONOTONE;
}

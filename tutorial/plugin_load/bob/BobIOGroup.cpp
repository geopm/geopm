/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cmath>

#include "BobIOGroup.hpp"
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
    geopm::iogroup_factory().register_plugin(BobIOGroup::plugin_name(),
                                             BobIOGroup::make_plugin);
}

std::string BobIOGroup::name(void) const
{
    return plugin_name();
}

// Name used for registration with the IOGroup factory
std::string BobIOGroup::plugin_name(void)
{
    return "bob";
}

// Function used by the factory to create objects of this type
std::unique_ptr<geopm::IOGroup> BobIOGroup::make_plugin(void)
{
    return std::unique_ptr<geopm::IOGroup>(new BobIOGroup);
}

// Set up mapping between signal names and corresponding indices
BobIOGroup::BobIOGroup()
    : m_signal_idx_map{{"BAR",  M_SIGNAL_BAR},
    {"BAZ",  M_SIGNAL_BAZ},
    {"TIME", M_SIGNAL_TIME}}
{

}

// A user-friendly description of each signal
std::string BobIOGroup::signal_description(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("BobIOGroup::signal_description(): signal_name " + signal_name +
                        " not valid for BobIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    int signal_idx = m_signal_idx_map.at(signal_name);
    std::string result = "";
    switch (signal_idx) {
        case M_SIGNAL_BAR:
            result = "Bar signal from Bob";
            break;
        case M_SIGNAL_BAZ:
            result = "Baz signal from Bob";
            break;
        case M_SIGNAL_TIME:
            result = "Time signal from Bob";
        default:
            break;
    }
    return result;
}

// Extract the set of all signal names from the index map
std::set<std::string> BobIOGroup::signal_names(void) const
{
    std::set<std::string> result;
    for (const auto &sv : m_signal_idx_map) {
        result.insert(sv.first);
    }
    return result;
}

std::set<std::string> BobIOGroup::control_names(void) const
{
    std::set<std::string> result;
    return result;
}

bool BobIOGroup::is_valid_signal(const std::string &signal_name) const
{
    return m_signal_idx_map.find(signal_name) != m_signal_idx_map.end();
}

bool BobIOGroup::is_valid_control(const std::string &control_name) const
{
    return false;
}

int BobIOGroup::signal_domain_type(const std::string &signal_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = GEOPM_DOMAIN_BOARD;
    }
    return result;
}

int BobIOGroup::control_domain_type(const std::string &control_name) const
{
    return GEOPM_DOMAIN_INVALID;
}

int BobIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    return -1;
}

int BobIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
{
    return -1;
}

void BobIOGroup::read_batch(void)
{

}

void BobIOGroup::write_batch(void)
{

}

double BobIOGroup::sample(int batch_idx)
{
    return NAN;
}

void BobIOGroup::adjust(int batch_idx, double setting)
{

}

double BobIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    return NAN;
}

void BobIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
{

}

void BobIOGroup::save_control(void)
{

}

void BobIOGroup::save_control(const std::string &save_path)
{

}

void BobIOGroup::restore_control(void)
{

}

void BobIOGroup::restore_control(const std::string &save_path)
{

}

std::function<double(const std::vector<double> &)> BobIOGroup::agg_function(const std::string &signal_name) const
{
    return geopm::Agg::average;
}

std::function<std::string(double)> BobIOGroup::format_function(const std::string &signal_name) const
{
    return geopm::string_format_double;
}

std::string BobIOGroup::control_description(const std::string &control_name) const
{
    return "";
}

int BobIOGroup::signal_behavior(const std::string &signal_name) const
{
    return M_SIGNAL_BEHAVIOR_MONOTONE;
}

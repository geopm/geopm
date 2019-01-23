/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <cmath>

#include "AliceIOGroup.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

using geopm::Exception;
using geopm::IPlatformTopo;

// Registers this IOGroup with the IOGroup factory, making it visible
// to PlatformIO when the plugin is first loaded.
static void __attribute__((constructor)) example_iogroup_load(void)
{
    geopm::iogroup_factory().register_plugin(AliceIOGroup::plugin_name(),
                                             AliceIOGroup::make_plugin);
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
    int result = IPlatformTopo::M_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = IPlatformTopo::M_DOMAIN_BOARD;
    }
    return result;
}

int AliceIOGroup::control_domain_type(const std::string &control_name) const
{
    return IPlatformTopo::M_DOMAIN_INVALID;
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

void AliceIOGroup::restore_control(void)
{

}

std::function<double(const std::vector<double> &)> AliceIOGroup::agg_function(const std::string &signal_name) const
{
    return geopm::Agg::average;
}

std::string AliceIOGroup::control_description(const std::string &control_name) const
{
    return "";
}

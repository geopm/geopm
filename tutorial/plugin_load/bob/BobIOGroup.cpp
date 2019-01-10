 /*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "BobIOGroup.hpp"
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
    geopm::iogroup_factory().register_plugin(BobIOGroup::plugin_name(),
                                             BobIOGroup::make_plugin);
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
    int result = IPlatformTopo::M_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = IPlatformTopo::M_DOMAIN_BOARD;
    }
    return result;
}

int BobIOGroup::control_domain_type(const std::string &control_name) const
{
    return IPlatformTopo::M_DOMAIN_INVALID;
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

void BobIOGroup::restore_control(void)
{

}

std::function<double(const std::vector<double> &)> BobIOGroup::agg_function(const std::string &signal_name) const
{
    return geopm::Agg::average;
}

std::string BobIOGroup::control_description(const std::string &control_name) const
{
    return "";
}

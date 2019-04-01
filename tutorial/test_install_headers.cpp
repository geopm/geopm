/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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


/// Tests that a file using only the installed headers can compile.
/// e.g.:
/// g++ -std=c++11 examples/test_install_headers.cpp \
///    -I/home/drguttma/build/geopm/include \
///    -lgeopm -L/home/drguttma/build/geopm/lib


// C headers
#include "geopm.h"
#include "geopm_agent.h"
//#include "geopm_ctl.h"
#include "geopm_error.h"
#include "geopm_hash.h"
#include "geopm_imbalancer.h"
#include "geopm_pio.h"
#include "geopm_sched.h"
#include "geopm_time.h"
#include "geopm_topo.h"
#include "geopm_version.h"

/// Interface classes
#include "geopm/Agent.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"

/// Helper classes
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "geopm/CircularBuffer.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/RegionAggregator.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/SharedMemoryUser.hpp"
#include "geopm/PowerBalancer.hpp"
#include "geopm/PowerGovernor.hpp"
#include "geopm/EnergyEfficientRegion.hpp"

/// Example IOGroup implementations
#include "geopm/CpuinfoIOGroup.hpp"
#include "geopm/MSRIOGroup.hpp"
#include "geopm/TimeIOGroup.hpp"

#include "geopm/MSR.hpp"
#include "geopm/MSRControl.hpp"
#include "geopm/MSRIO.hpp"

#include "geopm/MSRSignal.hpp"

// Example Agent implementations
#include "geopm/EnergyEfficientAgent.hpp"
#include "geopm/FrequencyMapAgent.hpp"
#include "geopm/MonitorAgent.hpp"
#include "geopm/PowerBalancerAgent.hpp"
#include "geopm/PowerGovernorAgent.hpp"

#include <memory>
#include <exception>
#include <iostream>

using geopm::PlatformIO;
using geopm::PlatformTopo;
using geopm::IOGroup;
using geopm::CpuinfoIOGroup;
using geopm::MSRIOGroup;
using geopm::TimeIOGroup;

using geopm::PowerBalancer;
using geopm::PowerGovernor;
using geopm::RegionAggregator;
using geopm::SharedMemory;
using geopm::SharedMemoryUser;

using geopm::Agent;
using geopm::MonitorAgent;
using geopm::PowerBalancerAgent;
using geopm::PowerGovernorAgent;
using geopm::EnergyEfficientAgent;
using geopm::FrequencyMapAgent;

int main()
{
    PlatformIO &platio = geopm::platform_io();
    PlatformTopo &topo = geopm::platform_topo();

    std::unique_ptr<IOGroup> m_cpuinfo = CpuinfoIOGroup::make_plugin();
    try {
        std::unique_ptr<IOGroup> m_msriogroup = MSRIOGroup::make_plugin();
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    std::unique_ptr<IOGroup> m_timeio = TimeIOGroup::make_plugin();

    std::unique_ptr<RegionAggregator> reg_agg = RegionAggregator::make_unique();
    try {
        std::unique_ptr<PowerGovernor> gov = PowerGovernor::make_unique(platio, topo);
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<PowerBalancer> bal = PowerBalancer::make_unique(0.5);
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }

    try {
        std::unique_ptr<SharedMemory> shmem = SharedMemory::make_unique("/dev/shm/testkey", 10);
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<SharedMemoryUser> shmem_user = SharedMemoryUser::make_unique("/dev/shm/testkey", 1);
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<Agent> m_monitor = MonitorAgent::make_plugin();
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<Agent> m_balancer = PowerBalancerAgent::make_plugin();
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<Agent> m_governor = PowerGovernorAgent::make_plugin();
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<Agent> m_freq_map = FrequencyMapAgent::make_plugin();
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
    try {
        std::unique_ptr<Agent> m_eng_eff = EnergyEfficientAgent::make_plugin();
    }
    catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }

}

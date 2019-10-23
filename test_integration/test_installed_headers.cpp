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

#include "geopm/Agent.hpp"
#include "geopm/Agg.hpp"
#include "geopm/CircularBuffer.hpp"
#include "geopm/CpuinfoIOGroup.hpp"
#include "geopm/EnergyEfficientRegion.hpp"
#include "geopm/FrequencyGovernor.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/MSR.hpp"
#include "geopm/MSRControl.hpp"
#include "geopm/MSRIO.hpp"
#include "geopm/MSRIOGroup.hpp"
#include "geopm/MSRSignal.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/PowerBalancer.hpp"
#include "geopm/PowerGovernor.hpp"
#include "geopm/RegionAggregator.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/SharedMemoryScopedLock.hpp"
#include "geopm/SharedMemoryUser.hpp"
#include "geopm/TimeIOGroup.hpp"

// Additional non-geopm headers for example inputs
#include <vector>
#include <pthread.h>

int main(int argc, char *argv[])
{
    // Agent: see agent tutorial
    auto agent = geopm::agent_factory().make_plugin("monitor");

    std::vector<double> vec;
    auto agg = geopm::Agg::sum(vec);

    geopm::CircularBuffer<int> circular_buffer;

    geopm::CpuinfoIOGroup cpuinfo_iogroup;

    try {
        auto energy_efficient_region = geopm::EnergyEfficientRegion::make_unique(1, 2, 3, 0.4);
    }
    catch (geopm::Exception) {
        // pass
    }

    try {
        // todo: remove from installed?
        geopm::EnergyEfficientRegionImp energy_efficient_region_imp{1, 2, 3, 0.4};
    }
    catch (geopm::Exception) {
        // pass
    }

    try {
        auto frequency_governor = geopm::FrequencyGovernor::make_unique();
    }
    catch (geopm::Exception) {
        // pass
    }

    geopm::Exception exception;

    geopm::SignalException signal_exception;

    // IOGroup: see iogroup tutorial
    auto io_group = geopm::iogroup_factory().make_plugin("TIME");

    auto msr = geopm::MSR::make_unique("name", 0x19,
                                       { {"one", {} } },
                                       { {"two", {} } });

    auto msr_control = geopm::MSRControl::make_unique(*msr, 0, 1, 0);

    try {
        auto msrio = geopm::MSRIO::make_unique();
    }
    catch (geopm::Exception) {
        // pass
    }

    try {
        geopm::MSRIOGroup msr_iogroup;
    }
    catch (geopm::Exception) {
        // pass
    }

    auto msr_signal = geopm::MSRSignal::make_unique(*msr, 0, 1, 0);

    geopm::PlatformIO &platformio = geopm::platform_io();

    const geopm::PlatformTopo &platform_topo = geopm::platform_topo();

    try {
        auto power_balancer = geopm::PowerBalancer::make_unique(42);
    }
    catch (geopm::Exception) {
        // pass
    }

    try {
        auto power_governor = geopm::PowerGovernor::make_unique();
    }
    catch (geopm::Exception) {
        // pass
    }

    auto region_aggregator = geopm::RegionAggregator::make_unique();

    try {
        auto shared_memory = geopm::SharedMemory::make_unique("/geopm_test_key", 16);
        shared_memory->unlink();
    }
    catch (geopm::Exception) {
        // pass
    }

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    geopm::SharedMemoryScopedLock shared_memory_scoped_lock{&mutex};

    try {
        auto shared_memory_user = geopm::SharedMemoryUser::make_unique("/bad", 1);
    }
    catch (geopm::Exception) {
        // pass
    }

    geopm::TimeIOGroup time_iogroup;

    return 0;
}

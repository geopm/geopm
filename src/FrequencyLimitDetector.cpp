/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "FrequencyLimitDetector.hpp"
#include "SSTFrequencyLimitDetector.hpp"
#include "TRLFrequencyLimitDetector.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"

namespace geopm
{
    static bool use_sst_tf_signals(PlatformIO &platform_io)
    {
        bool do_use_signals = false;
        try {
            do_use_signals = platform_io.read_signal(
                "SST::TURBOFREQ_SUPPORT:SUPPORTED", GEOPM_DOMAIN_BOARD, 0);
        }
        catch (const geopm::Exception &) {
            // Either we don't know if SST-TF is supported, or it definitely is
            // not supported. So do not use SST-TF signals.
        }
        return do_use_signals;
    }

    std::unique_ptr<FrequencyLimitDetector> FrequencyLimitDetector::make_unique(
        PlatformIO &platform_io, const PlatformTopo &platform_topo)
    {

        if (use_sst_tf_signals(platform_io)) {
            return geopm::make_unique<SSTFrequencyLimitDetector>(platform_io, platform_topo);
        }
        else {
            return geopm::make_unique<TRLFrequencyLimitDetector>(platform_io, platform_topo);
        }
    }

    std::shared_ptr<FrequencyLimitDetector> FrequencyLimitDetector::make_shared(
        PlatformIO &platform_io, const PlatformTopo &platform_topo)
    {

        if (use_sst_tf_signals(platform_io)) {
            return std::make_shared<SSTFrequencyLimitDetector>(platform_io, platform_topo);
        }
        else {
            return std::make_shared<TRLFrequencyLimitDetector>(platform_io, platform_topo);
        }
    }
}

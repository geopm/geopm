/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TIMESIGNAL_HPP_INCLUDE
#define TIMESIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"
#include "geopm_time.h"

namespace geopm
{
    /// A signal used by an IOGroup to produce a signal from
    /// geopm_time().
    class TimeSignal : public Signal
    {
        public:
            TimeSignal(std::shared_ptr<geopm_time_s> time_zero,
                       std::shared_ptr<double> time_batch);
            TimeSignal(const TimeSignal &other) = delete;
            TimeSignal &operator=(const TimeSignal &other) = delete;
            virtual ~TimeSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            std::shared_ptr<geopm_time_s> m_time_zero;
            std::shared_ptr<double> m_time_batch;
            bool m_is_batch_ready;
    };
}

#endif

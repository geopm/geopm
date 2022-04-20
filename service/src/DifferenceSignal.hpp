/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DIFFERENCESIGNAL_HPP_INCLUDE
#define DIFFERENCESIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"

namespace geopm
{
    /// A composite signal used by an IOGroup to produce a signal as
    /// the difference of two signals.
    class DifferenceSignal : public Signal
    {
        public:
            DifferenceSignal(std::shared_ptr<Signal> minuend,
                             std::shared_ptr<Signal> subtrahend);
            DifferenceSignal(const DifferenceSignal &other) = delete;
            DifferenceSignal &operator=(const DifferenceSignal &other) = delete;
            virtual ~DifferenceSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            std::shared_ptr<Signal> m_minuend;
            std::shared_ptr<Signal> m_subtrahend;
            bool m_is_batch_ready;
    };
}

#endif

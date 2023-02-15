/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RATIOSIGNAL_HPP_INCLUDE
#define RATIOSIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"

namespace geopm
{
    /// @brief A composite signal used by an IOGroup to produce a signal as
    /// the Ratio of two signals.
    class RatioSignal : public Signal
    {
        public:
            RatioSignal(std::shared_ptr<Signal> numerator,
                        std::shared_ptr<Signal> denominator);
            RatioSignal(const RatioSignal &other) = delete;
            RatioSignal &operator=(const RatioSignal &other) = delete;
            virtual ~RatioSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            std::shared_ptr<Signal> m_numerator;
            std::shared_ptr<Signal> m_denominator;
            bool m_is_batch_ready;
    };
}

#endif

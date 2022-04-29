/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DIVISIONSIGNAL_HPP_INCLUDE
#define DIVISIONSIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"

namespace geopm
{
    /// @brief A composite signal used by an IOGroup to produce a signal as
    /// the Division of two signals.
    class DivisionSignal : public Signal
    {
        public:
            DivisionSignal(std::shared_ptr<Signal> numerator,
                           std::shared_ptr<Signal> denominator);
            DivisionSignal(const DivisionSignal &other) = delete;
            virtual ~DivisionSignal() = default;
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

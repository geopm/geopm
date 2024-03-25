/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MULTIPLICATIONSIGNAL_HPP_INCLUDE
#define MULTIPLICATIONSIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"

namespace geopm
{
    /// @brief A composite signal used by an IOGroup to produce a signal as
    /// the Multiplication of one signal and a double.
    class MultiplicationSignal : public Signal
    {
        public:
            MultiplicationSignal(std::shared_ptr<Signal> multiplier,
                           double multiplicand);
            MultiplicationSignal(const MultiplicationSignal &other) = delete;
            MultiplicationSignal &operator=(const MultiplicationSignal &other) = delete;
            virtual ~MultiplicationSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            std::shared_ptr<Signal> m_multiplier;
            double m_multiplicand;
            bool m_is_batch_ready;
    };
}

#endif

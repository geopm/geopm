/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZEROSIGNAL_HPP_INCLUDE
#define LEVELZEROSIGNAL_HPP_INCLUDE

#include <functional>

#include "Signal.hpp"
#include "LevelZeroDevicePool.hpp"


namespace geopm
{
    class LevelZeroDevicePool;

    class LevelZeroSignal : public Signal
    {
        public:
            virtual ~LevelZeroSignal() = default;
            LevelZeroSignal(std::function<double (unsigned int)> devpool_func,
                         unsigned int gpu, double scalar);
            LevelZeroSignal(const LevelZeroSignal &other) = delete;
            LevelZeroSignal &operator=(const LevelZeroSignal &other) = delete;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
            void set_sample(double value) override;
        private:
            std::function<double (unsigned int)> m_devpool_func;
            unsigned int m_domain_idx;
            double m_scalar;
            bool m_is_batch_ready;
            double m_value;
    };
}

#endif

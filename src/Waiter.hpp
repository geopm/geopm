/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WAITER_HPP_INCLUDE
#define WAITER_HPP_INCLUDE

#include <memory>
#include "geopm_time.h"

namespace geopm
{
    /// @brief Class to support a periodic wait loop
    class Waiter
    {
        public:
            /// @brief Create a Waiter with "sleep" strategy
            /// @param [in] period Duration in seconds to wait
            static std::unique_ptr<Waiter> make_unique(double period);
            /// @brief Create a Waiter
            /// @param [in] period Duration in seconds to wait
            /// @param [in] strategy Wait algorithm ("sleep")
            static std::unique_ptr<Waiter> make_unique(double period,
                                                       std::string strategy);
            Waiter() = default;
            virtual ~Waiter() = default;
            /// @brief Reset the timer for next wait
            virtual void reset(void) = 0;
            /// @brief Reset the timer for next wait and set period
            /// @param [in] period Duration in seconds to wait
            virtual void reset(double period) = 0;
            /// @brief Wait until the period has elapsed since last call to
            ///        reset() or wait()
            virtual void wait(void) = 0;
            /// @brief Get the period for the waiter
            /// @return The duration of the wait
            virtual double period(void) const = 0;
    };

    /// @brief Class to support a periodic wait loop based on
    ///        clock_nanosleep() using CLOCK_REALTIME.
    class SleepWaiter : public Waiter
    {
        public:
            SleepWaiter(double period);
            virtual ~SleepWaiter() = default;
            void reset(void) override;
            void reset(double period) override;
            void wait(void) override;
            double period(void) const override;
        private:
            double m_period;
            geopm_time_s m_time_target;
            bool m_is_first_time;
    };
}

#endif

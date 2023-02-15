/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SIGNAL_HPP_INCLUDE
#define SIGNAL_HPP_INCLUDE

namespace geopm
{
    /// An abstract interface for all types of signals supported by an
    /// IOGroup.  Any implementation specific data should be injected
    /// in the derived class constructor and used in setup_batch() if
    /// necessary.
    class Signal
    {
        public:
            virtual ~Signal() = default;
            /// @brief Prepare the signal for being updated through
            ///        side effects by the owner's read_batch step.
            ///        This method should not fail if called multiple
            ///        times, and ideally only apply the side effects
            ///        on the first call.
            virtual void setup_batch(void) = 0;
            /// @brief Apply any conversions necessary to interpret
            ///        the latest stored value as a double.
            virtual double sample(void) = 0;
            /// @brief Read directly the value of the signal without
            ///        affecting any pushed batch signals.
            virtual double read(void) const = 0;
            /// @brief Set the value to be returned by sample()
            virtual void set_sample(double) {};
    };
}

#endif

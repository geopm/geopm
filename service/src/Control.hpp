/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTROL_HPP_INCLUDE
#define CONTROL_HPP_INCLUDE

namespace geopm
{
    /// An abstract interface for all types of controls supported by
    /// an IOGroup.  Any implementation specific data should be
    /// injected in the derived class constructor and used in
    /// setup_batch() if necessary.
    class Control
    {
        public:
            virtual ~Control() = default;
            /// @brief Prepare the control for being written through
            ///        side effects by the owner's write_batch step.
            ///        This method should not fail if called multiple
            ///        times, and ideally only apply the side effects
            ///        on the first call.
            virtual void setup_batch(void) = 0;
            /// @brief Store values to be written by the owner's
            ///        write_batch step.
            virtual void adjust(double value) = 0;
            /// @brief Write the value of the control without
            ///        affecting any pushed batch controls.
            virtual void write(double value) = 0;
            /// @brief Store the current setting of the control for use
            ///        by a future call to restore().
            virtual void save(void) = 0;
            /// @brief Restore the setting stored by save().
            virtual void restore(void) = 0;
    };
}

#endif

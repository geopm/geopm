/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef COMBINEDCONTROL_HPP_INCLUDE
#define COMBINEDCONTROL_HPP_INCLUDE


namespace geopm
{
    /// @brief Used by PlatformIO to define a control as a function of
    ///        other controls.
    class CombinedControl
    {
        public:
            CombinedControl();
            CombinedControl(double factor);
            virtual ~CombinedControl() = default;
            /// @brief Multiply setting by factor and return
            virtual double adjust(double setting);
        private:
            double m_factor;
    };
}

#endif

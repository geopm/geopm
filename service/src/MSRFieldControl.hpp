/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MSRFIELDCONTROL_HPP_INCLUDE
#define MSRFIELDCONTROL_HPP_INCLUDE

#include <cstdint>
#include <cmath>

#include <string>
#include <memory>

#include "Control.hpp"

namespace geopm
{
    class MSRIO;

    /// Encapsulates conversion of control values in SI units to MSR
    /// bitfields.
    class MSRFieldControl : public Control
    {
        public:
            MSRFieldControl(std::shared_ptr<MSRIO> msrio,
                            int cpu,
                            uint64_t offset,
                            int begin_bit,
                            int end_bit,
                            int function,
                            double scalar);
            MSRFieldControl(const MSRFieldControl &other) = delete;
            MSRFieldControl &operator=(const MSRFieldControl &other) = delete;
            virtual ~MSRFieldControl() = default;
            void setup_batch(void) override;
            void adjust(double value) override;
            void write(double value) override;
            void save(void) override;
            void restore(void) override;
        private:
            uint64_t encode(double value) const;

            std::shared_ptr<MSRIO> m_msrio;
            const int m_cpu;
            const uint64_t m_offset;
            const int m_shift;
            const int m_num_bit;
            const uint64_t m_mask;
            const int m_function;
            const double m_inverse;
            bool m_is_batch_ready;
            int m_adjust_idx;
            uint64_t m_saved_msr_value;
    };
}

#endif

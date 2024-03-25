/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RAWMSRSIGNAL_HPP_INCLUDE
#define RAWMSRSIGNAL_HPP_INCLUDE

#include <cstdint>
#include <cmath>

#include <string>
#include <memory>

#include "Signal.hpp"


namespace geopm
{
    class MSRIO;

    class RawMSRSignal : public Signal
    {
        public:
            RawMSRSignal(std::shared_ptr<MSRIO> msrio,
                         int cpu,
                         uint64_t offset);
            RawMSRSignal(const RawMSRSignal &other) = delete;
            RawMSRSignal &operator=(const RawMSRSignal &other) = delete;
            virtual ~RawMSRSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            /// MSRIO object shared by all MSR signals in the same
            /// batch.  This object should outlive all other data in
            /// the Signal.
            std::shared_ptr<MSRIO> m_msrio;
            int m_cpu;
            uint64_t m_offset;
            /// Index to the data that will be updated by the
            /// MSRIO's read_batch() calls.
            int m_data_idx;
            bool m_is_batch_ready;
    };
}

#endif

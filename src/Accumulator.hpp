/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ACCUMULATOR_HPP_INCLUDE
#define ACCUMULATOR_HPP_INCLUDE

#include <memory>

namespace geopm
{
    class SumAccumulator
    {
        public:
            static std::unique_ptr<SumAccumulator> make_unique(void);
            virtual ~SumAccumulator() = default;
            virtual void update(double delta_signal) = 0;
            virtual void enter(void) = 0;
            virtual void exit(void) = 0;
            virtual double sample_all(void) const = 0;
            virtual double sample_last(void) const = 0;
        protected:
            SumAccumulator() = default;
    };

    class AvgAccumulator
    {
        public:
            static std::unique_ptr<AvgAccumulator> make_unique(void);
            virtual ~AvgAccumulator() = default;
            virtual void update(double delta_time, double signal) = 0;
            virtual void enter(void) = 0;
            virtual void exit(void) = 0;
            virtual double sample_all(void) const = 0;
            virtual double sample_last(void) const = 0;
        protected:
            AvgAccumulator() = default;
    };

    class SumAccumulatorImp : public SumAccumulator
    {
        public:
            SumAccumulatorImp();
            virtual ~SumAccumulatorImp() = default;
            void update(double delta_signal) override;
            void enter(void) override;
            void exit(void) override;
            double sample_all(void) const override;
            double sample_last(void) const override;
        private:
            double m_total; // Sum of all delta values
            double m_current; // Current sum of delta values since entry (increasing while in region and zeroed on entry)
            double m_last; // Last sum of delta values over region (updated on exit with "current" value)
    };

    class AvgAccumulatorImp : public AvgAccumulator
    {
        public:
            AvgAccumulatorImp();
            virtual ~AvgAccumulatorImp() = default;
            void update(double delta_time, double signal) override;
            void enter(void) override;
            void exit(void) override;
            double sample_all(void) const override;
            double sample_last(void) const override;
        private:
            double m_total; // Sum of all delta values times delta time
            double m_weight; // Sum of all delta time
            double m_curr_total; // Sum of all delta values time delta time since last entry (increasing while in a region and zeroed on entry)
            double m_curr_weight; // Sum of all time delta time since last entry (increasing while in a region and zeroed on entry)
            double m_last; // When region is exited update with curr_total / curr_weight
    };
}

#endif

/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "config.h"

#include "Accumulator.hpp"
#include "Helper.hpp"

namespace geopm
{

    std::unique_ptr<SumAccumulator> SumAccumulator::make_unique()
    {
        return geopm::make_unique<SumAccumulatorImp>();
    }

    std::unique_ptr<AvgAccumulator> AvgAccumulator::make_unique()
    {
        return geopm::make_unique<AvgAccumulatorImp>();
    }

    SumAccumulatorImp::SumAccumulatorImp()
        : m_total(0.0)
        , m_current(0.0)
        , m_last(0.0)
    {

    }

    void SumAccumulatorImp::update(double delta_signal)
    {
        m_total += delta_signal;
        m_current += delta_signal;
    }

    void SumAccumulatorImp::enter(void)
    {
        m_current = 0.0;
    }

    void SumAccumulatorImp::exit(void)
    {
        m_last = m_current;
    }

    double SumAccumulatorImp::total(void) const
    {
        return m_total;
    }

    double SumAccumulatorImp::interval_total(void) const
    {
        return m_last;
    }

    AvgAccumulatorImp::AvgAccumulatorImp()
        : m_total(0.0)
        , m_weight(0.0)
        , m_curr_total(0.0)
        , m_curr_weight(0.0)
        , m_last(0.0)
    {

    }

    void AvgAccumulatorImp::update(double delta_time, double signal)
    {
        m_total += delta_time * signal;
        m_weight += delta_time;
        m_curr_total += delta_time * signal;
        m_curr_weight += delta_time;
    }

    void AvgAccumulatorImp::enter(void)
    {
        m_curr_total = 0.0;
        m_curr_weight = 0.0;
    }

    void AvgAccumulatorImp::exit(void)
    {
        m_last = m_curr_weight == 0.0 ? 0.0 :
                 m_curr_total / m_curr_weight;
    }

    double AvgAccumulatorImp::average(void) const
    {
        return m_weight == 0.0 ? 0.0 :
               m_total / m_weight;
    }

    double AvgAccumulatorImp::interval_average(void) const
    {
        return m_last;
    }
}

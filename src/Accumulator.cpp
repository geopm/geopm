/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Accumulator.hpp"
#include "geopm/Helper.hpp"

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

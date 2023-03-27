/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REGIONHINTRECOMMENDERIMP_HPP_INCLUDE
#define REGIONHINTRECOMMENDERIMP_HPP_INCLUDE

#include "geopm/json11.hpp"

namespace geopm
{
    class RegionHintRecommenderImp : public RegionHintRecommender
    {
        public:
            RegionHintRecommenderImp(const std::string fmap_path, int min_freq, int max_freq);
            double recommend_frequency(std::map<std::string, float> region_class, double phi) const override;

        private:
            int m_min_freq;
            int m_max_freq;
            std::map<std::string, std::vector<double> > m_freq_map;
    };
}


#endif  /* REGIONHINTRECOMMENDERIMP_HPP_INCLUDE */

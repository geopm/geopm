/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REGIONHINTRECOMMENDERIMP_HPP_INCLUDE
#define REGIONHINTRECOMMENDERIMP_HPP_INCLUDE

#include "geopm/json11.hpp"

namespace geopm
{
    /// @brief Class ingesting region classification logits and
    ///        a frequency map json file and determining a recommended 
    ///        frequency decision.
    class RegionHintRecommenderImp : public RegionHintRecommender
    {
        public:
            RegionHintRecommenderImp(const std::string &fmap_path, int min_freq, int max_freq);
            /// @brief Recommends frequency based on region classification logits 
            ///
            /// @param [in] nn_output List of region classification names and
            ///             logits output from region class neural net
            /// @param [in] phi User-input perf-energy bias
            double recommend_frequency(const std::map<std::string, double> &nn_output, double phi)
                    const override;

        private:
            int m_min_freq;
            int m_max_freq;
            std::map<std::string, std::vector<double> > m_freq_map;
    };
}


#endif  /* REGIONHINTRECOMMENDERIMP_HPP_INCLUDE */

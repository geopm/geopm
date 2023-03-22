/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REGIONHINTRECOMMENDER_HPP_INCLUDE
#define REGIONHINTRECOMMENDER_HPP_INCLUDE

#include "geopm/json11.hpp"

namespace geopm
{
    /// @brief Class ingesting region classification probabilities and
    ///        a frequency map json11 file and determining a recommended 
    ///        frequency decisions.
    class RegionHintRecommender
    {
        public:
            /// @brief Loads frequency map file from json11 into a std::map
            ///        of region class string to double
            ///
            /// @param [in] fmap_path string containing path to freq map json
            void load_freqmap(char* fmap_path);
            /// @brief Set maximum frequency to recommend
            ///
            /// @param [in] max_freq integer containing maximum frequency
            void set_max_freq(int max_freq);
            /// @brief Set minimum frequency to recommend
            /// @param [in] min_freq integer containing minimum frequency
            void set_min_freq(int min_freq);
            /// @brief Recommends frequency based on region classification probabilities
            ///
            /// @param [in] region_class List of region classification names and
            ///             probabilities output from region class neural net
            /// @param [in] phi User-input energy-perf bias
            double recommend_frequency(std::map<std::string, float> region_class, int phi);

        private:
            int m_max_freq;
            int m_min_freq;
            std::map<std::string, std::vector<double> > m_freq_map;
            std::vector<int> m_freq_control;
    };
}

#endif  /* REGIONHINTRECOMMENDER_HPP_INCLUDE */

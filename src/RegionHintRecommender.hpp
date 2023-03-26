/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REGIONHINTRECOMMENDER_HPP_INCLUDE
#define REGIONHINTRECOMMENDER_HPP_INCLUDE

#include <memory>
#include <map>

namespace geopm
{
    /// @brief Class ingesting region classification probabilities and
    ///        a frequency map json file and determining a recommended 
    ///        frequency decisions.
    class RegionHintRecommender
    {
        public:
            /// @brief Returns a unique_ptr to a concrete object constructed
            ///        using the underlying implementation, which loads a
            ///        frequency map file into a std::map of region class
            ///        string to double, and sets both min and max frequency
            ///        recommendations.
            ///
            /// @param [in] fmap_path string containing path to freq map json
            /// @param [in] min_freq integer containing minimum frequency
            /// @param [in] max_freq integer containing maximum frequency
            static std::unique_ptr<RegionHintRecommender> make_unique(const std::string fmap_path, int min_freq, int max_freq);

            /// @brief Recommends frequency based on region classification probabilities
            ///
            /// @param [in] region_class List of region classification names and
            ///             probabilities output from region class neural net
            /// @param [in] phi User-input energy-perf bias
            virtual double recommend_frequency(std::map<std::string, float> region_class, double phi) const = 0;
    };
}

#endif  /* REGIONHINTRECOMMENDER_HPP_INCLUDE */

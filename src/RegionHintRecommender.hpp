/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REGIONHINTRECOMMENDER_HPP_INCLUDE
#define REGIONHINTRECOMMENDER_HPP_INCLUDE

#include <memory>
#include <map>
#include <string>
#include <vector>

namespace geopm
{
    /// @brief Class ingesting the output from a DomainNetMap and
    ///        a frequency map json file and determining a recommended 
    ///        frequency decision.
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
            static std::unique_ptr<RegionHintRecommender> make_unique(const std::string &fmap_path,
                                                                      int min_freq, int max_freq);
            /// @brief Returns a shared_ptr to a concrete object constructed
            ///        using the underlying implementation, which loads a
            ///        frequency map file into a std::map of region class
            ///        string to double, and sets both min and max frequency
            ///        recommendations.
            ///
            /// @param [in] fmap_path string containing path to freq map json
            /// @param [in] min_freq integer containing minimum frequency
            /// @param [in] max_freq integer containing maximum frequency
            static std::shared_ptr<RegionHintRecommender> make_shared(const std::string &fmap_path,
                                                                      int min_freq, int max_freq);

            virtual ~RegionHintRecommender() = default;
            /// @brief Recommends frequency based on the output from a DomainNetMap neural net
            ///        evaluation
            ///
            /// @param [in] nn_output Output from the DomainNetMap neural net evaluation, as a
            ///             map from trace names to doubles.
            /// @param [in] phi User-input perf-energy bias
            ///
            /// @return Returns frequency, double in Hertz
            virtual double recommend_frequency(const std::map<std::string, double> &nn_output,
                                               double phi) const = 0;
    };
}

#endif  /* REGIONHINTRECOMMENDER_HPP_INCLUDE */

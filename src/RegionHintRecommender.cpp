/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RegionHintRecommender.hpp"
#include "RegionHintRecommenderImp.hpp"

#include <cmath>
#include <fstream>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    std::unique_ptr<RegionHintRecommender> RegionHintRecommender::make_unique(
            const std::string &fmap_path,
            int min_freq,
            int max_freq)
    {
        return geopm::make_unique<RegionHintRecommenderImp>(fmap_path, min_freq, max_freq);
    }

    std::shared_ptr<RegionHintRecommender> RegionHintRecommender::make_shared(
            const std::string &fmap_path,
            int min_freq,
            int max_freq)
    {
        return std::make_shared<RegionHintRecommenderImp>(fmap_path, min_freq, max_freq);
    }

    RegionHintRecommenderImp::RegionHintRecommenderImp(const std::string &fmap_path, int min_freq,
                                                       int max_freq)
        : m_min_freq(min_freq)
          , m_max_freq(max_freq)
    {
        std::string buf, err;

        std::ifstream ffile(fmap_path);
        if (!ffile) {
            throw Exception("RegionHintRecommenderImp::" + std::string(__func__) +
                            ": Unable to open frequency map file.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        ffile.seekg(0, std::ios::end);
        std::streampos length = ffile.tellg();
        ffile.seekg(0, std::ios::beg);

        std::string fbuf;
        fbuf.reserve(length);

        std::getline(ffile, fbuf, '\0');

        json11::Json fmap_json = json11::Json::parse(fbuf, err);
        
        if (!err.empty()) {
            throw Exception("RegionHintRecommenderImp::" + std::string(__func__) +
                            ": Frequency map file format is incorrect: " + err + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (fmap_json.is_null() || !fmap_json.is_object()) {
            throw Exception("RegionHintRecommenderImp::" + std::string(__func__) +
                            ": Frequency map file format is incorrect: object expected.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (const auto &row : fmap_json.object_items()) {
            if (!row.second.is_array() || row.second.array_items().size() == 0) {
                throw Exception("RegionHintRecommenderImp::" + std::string(__func__) +
                                ": Frequency map file format is incorrect: region keys "
                                "must contain an array of numbers.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_freq_map[row.first].resize(row.second.array_items().size());
            for (size_t idx = 0; idx < m_freq_map[row.first].size(); ++idx) {
                if (!row.second[idx].is_number()) {
                    throw Exception("RegionHintRecommenderImp::" + std::string(__func__) +
                                    ": Non-numeric value found in frequencies for "
                                    "region: \"" + row.first + "\".",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                m_freq_map[row.first][idx] = row.second[idx].number_value();
            }
        }

        if (m_freq_map.empty()) {
            throw Exception("RegionHintRecommenderImp::" + std::string(__func__) +
                            ": Frequency map file must contain a frequency map.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    double RegionHintRecommenderImp::recommend_frequency(
            const std::map<std::string, double> &nn_output,
            double phi) const {
        for (const auto &kv : nn_output ) {
            if (std::isnan(kv.second)) {
                return NAN;
            }
        }

        double zz = 0;
        double freq = 0;
        for (const auto &[region_name, probability] : nn_output) {
            size_t phi_idx = static_cast<size_t>(
                                std::floor(phi * (m_freq_map.at(region_name).size() - 1)));
            freq += exp(probability) * m_freq_map.at(region_name).at(phi_idx);
            zz += exp(probability);
        }
        freq = freq / zz;

        if (std::isnan(freq)) {
            freq = m_max_freq;
        }

        if (freq > m_max_freq) {
            freq = m_max_freq;
        }
        if (freq < m_min_freq) {
            freq = m_min_freq;
        }

        return freq * 1e8;
    }
}

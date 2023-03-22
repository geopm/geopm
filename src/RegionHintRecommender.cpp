/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RegionHintRecommender.hpp"

#include <cmath>
#include <fstream>

#include "geopm/Exception.hpp"

namespace geopm
{
    void RegionHintRecommender::load_freqmap(char* fmap_path)
    {
        std::string buf, err;

        std::ifstream ffile(fmap_path);
        if (!ffile) {
            throw geopm::Exception("Unable to open frequency map file.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        ffile.seekg(0, std::ios::end);
        std::streampos length = ffile.tellg();
        ffile.seekg(0, std::ios::beg);

        std::string fbuf;
        fbuf.reserve(length);

        std::getline(ffile, fbuf, '\0');

        json11::Json fmap_json = json11::Json::parse(fbuf, err);

        for (const auto &row : fmap_json.object_items()) {
            m_freq_map[row.first].resize(row.second.array_items().size());
            for (int idx=0; idx < (int)m_freq_map[row.first].size(); idx++) {
                m_freq_map[row.first][idx] = row.second[idx].number_value();
            }
        }
    }

    void RegionHintRecommender::set_max_freq(int max_freq)
    {
        m_max_freq = max_freq;
    }

    void RegionHintRecommender::set_min_freq(int min_freq)
    {
        m_min_freq = min_freq;
    }

    double RegionHintRecommender::recommend_frequency(std::map<std::string, float> region_class, int phi)
    {
        for (const auto & [region_name, probability] : region_class) {
            if (std::isnan(probability)) {
                return NAN;
            }
        }

        double ZZ = 0;
        double freq = 0;
        for (const auto & [region_name, probability] : region_class) {
            freq += exp(probability) * m_freq_map[region_name][phi];
            ZZ += exp(probability);
        }
        freq = freq / ZZ;

        if (std::isnan(freq)) {
            freq = m_max_freq;
        }

        if (freq > m_max_freq) {
            freq = m_max_freq;
        }
        if (freq < m_min_freq) {
            freq = m_min_freq;
        }

        return freq*1e8;
    }
}

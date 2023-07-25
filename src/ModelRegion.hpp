/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MODELREGION_HPP_INCLUDE
#define MODELREGION_HPP_INCLUDE

#include <string>
#include <cstdint>
#include <memory>

namespace geopm
{
    class ModelRegion
    {
        public:
            static std::unique_ptr<ModelRegion> model_region(const std::string &name,
                                                             double big_o,
                                                             int verbosity);
            ModelRegion(int verbosity);
            virtual ~ModelRegion();
            std::string name(void);
            double big_o(void);
            virtual void region(void);
            virtual void region(uint64_t hint);
            virtual void region_enter(void);
            virtual void region_exit(void);
            virtual void loop_enter(uint64_t iteration);
            virtual void loop_exit(void);
            virtual void big_o(double big_o_in) = 0;
            virtual void run(void) = 0;
        protected:
            virtual void num_progress_updates(double big_o_in);
            static bool name_check(const std::string &name, const std::string &key);
            std::string m_name;
            double m_big_o;
            int m_verbosity;
            uint64_t m_region_id;
            bool m_do_imbalance;
            bool m_do_progress;
            bool m_do_unmarked;
            uint64_t m_num_progress_updates;
            double m_norm;
    };
}

#endif

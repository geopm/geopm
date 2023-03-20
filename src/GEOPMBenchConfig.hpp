/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPMBENCHCONFIG_HPP_INCLUDE
#define GEOPMBENCHCONFIG_HPP_INCLUDE

namespace geopm
{
    class GEOPMBenchConfig
    {
        public:
            GEOPMBenchConfig() = default;
            virtual ~GEOPMBenchConfig() = default;

            virtual bool is_mpi_enabled() const = 0;
    };

    class GEOPMBenchConfigImp : public GEOPMBenchConfig
    {
        public:
            GEOPMBenchConfigImp();
            GEOPMBenchConfigImp(bool is_mpi_enabled);
            virtual ~GEOPMBenchConfigImp() = default;

            bool is_mpi_enabled() const override;

        private:
            bool m_is_mpi_enabled;
    };

    const GEOPMBenchConfig &geopmbench_config(void);
}

#endif

/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSTCLOSGOVERNOR_HPP_INCLUDE
#define SSTCLOSGOVERNOR_HPP_INCLUDE

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    /// Govern class of service (CLOS) controls.
    class SSTClosGovernor
    {
        public:
            enum ClosLevel_e {
                HIGH_PRIORITY = 0,
                MEDIUM_HIGH_PRIORITY = 1,
                MEDIUM_LOW_PRIORITY = 2,
                LOW_PRIORITY = 3,
            };

            SSTClosGovernor() = default;
            virtual ~SSTClosGovernor() = default;

            /// @brief Registers signals and controls with PlatformIO using the
            ///        default control domain.
            virtual void init_platform_io(void) = 0;

            /// @brief Get the domain type of CLOS control on the
            ///        platform.  Users of the SSTClosGovernor can
            ///        use this information to determine the size of
            ///        the vector to pass to adjust_platform().
            /// @return The domain with which CLOS will be governed.
            virtual int clos_domain_type(void) const = 0;

            /// @brief Write CLOS controls
            /// @param [in] clos_by_core Desired per-core CLOS.
            virtual void adjust_platform(const std::vector<double> &clos_by_core) = 0;

            /// @brief Returns true if the last call to adjust_platform
            ///        requires writing controls.
            /// @return True if platform adjustments have been made, false otherwise.
            virtual bool do_write_batch(void) const = 0;

            /// Enable prioritized turbo frequency and core priority
            /// features. This is a no-op if those features are not supported
            /// on the platform.
            virtual void enable_sst_turbo_prioritization() = 0;

            /// Disable prioritized turbo frequency and core priority
            /// features. This is a no-op if those features are not supported
            /// on the platform.
            virtual void disable_sst_turbo_prioritization() = 0;

            /// Indicate whether this platform supports core priority and
            /// prioritized turbo frequency limits.
            static bool is_supported(PlatformIO &platform_io);

            static std::unique_ptr<SSTClosGovernor> make_unique(void);
            static std::shared_ptr<SSTClosGovernor> make_shared(void);
    };
}

#endif // SSTCLOSGOVERNOR_HPP_INCLUDE

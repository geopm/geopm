/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PLATFORMFACTORY_HPP_INCLUDE
#define PLATFORMFACTORY_HPP_INCLUDE

#include <vector>
#include <memory>

int geopm_read_cpuid(void);

namespace geopm
{
    class Platform;
    class PlatformImp;
    /// @brief Provides a factory abstraction for creating Platform/PlatformImp pairs
    /// suitable for the specific hardware the runtime is operating on. The
    /// factory also loads plugins at creation to provide extensibility to
    /// other platforms.
    class PlatformFactory
    {
        public:
            /// @brief PlatformFactory Default constructor
            PlatformFactory();
            /// @brief PlatformFactory Testing constructor.
            ///
            /// This constructor takes in
            /// a specific Platform/PlatformImp pair and does not load plugins.
            /// It is intended to be used for testing.
            /// @param [in] platform The unique_ptr to a Platform object
            ///             assures that the object cannot be destroyed before
            ///             it is copied.
            /// @param [in] platform_imp The unique_ptr to a PlatformImp object
            ///             assures that the object cannot be destroyed before
            ///             it is copied.
            PlatformFactory(std::unique_ptr<Platform> platform,
                            std::unique_ptr<PlatformImp> platform_imp);
            /// @brief PlatformFactory Default destructor.
            virtual ~PlatformFactory();

            /// @brief Returns an abstract Platform pointer to a concrete platform.
            ///
            /// The PlatformImp pointer held by the Platform is initialized to
            /// the appropriate object dependent on the underlying hardware.
            /// The concrete Platform is specific to the underlying class of
            /// hardware it is being run on.
            /// throws a std::invalid_argument if no acceptable
            /// Platform/PlatformImp pair is found.
            ///
            /// @param [in] description The descrition string corresponding
            ///        to the desired Platform.
            ///
            /// @param [in] do_initialize Choose whether or not to initialize
            ///        the returned Platform.
            Platform *platform(const std::string &description, bool do_initialize);
            /// @brief Concrete Platforms register with the factory through this API.
            ///
            /// @param [in] platform The unique_ptr to a Platform object
            ///        assures that the object cannot be destroyed
            ///        before it is copied.
            void register_platform(std::unique_ptr<Platform> platform);
            /// @brief Concrete PlatformImps register with the factory through this API.
            ///
            /// @param [in] platform_imp The unique_ptr to a PlatformImp object
            ///        assures that the object cannot be destroyed
            ///        before it is copied.
            void register_platform(std::unique_ptr<PlatformImp> platform_imp);
        private:
            /// @brief Uses the cpuid asm instruction to identify the hardware
            /// it is being run on.
            virtual int read_cpuid(void);

            // @brief Holds all registered concrete Platform instances.
            std::vector<Platform *> platforms;
            // @brief Holds all registered concrete PlatformImp instances.
            std::vector<PlatformImp *> platform_imps;
    };

}

#endif

/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef APPLICATIONIO_HPP_INCLUDE
#define APPLICATIONIO_HPP_INCLUDE

#include <cstdint>
#include <set>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <list>

#include "geopm_message.h"
#include "geopm_time.h"

namespace geopm
{
    class IComm;

    class IApplicationIO
    {
        public:
            IApplicationIO() = default;
            virtual ~IApplicationIO() = default;
            /// @brief Returns true if the application has indicated
            ///        it is shutting down.
            virtual bool do_shutdown(void) const = 0;
            /// @brief Returns the path to the report file.
            virtual std::string report_name(void) const = 0;
            /// @brief Returns the profile name to be used in the
            ///        report.
            virtual std::string profile_name(void) const = 0;
            /// @brief Returns the set of region names recorded by the
            ///        application.
            virtual std::set<std::string> region_name_set(void) const = 0;
            /// @brief Returns the total runtime for a region.
            /// @param [in] region_id The region ID.
            virtual double total_region_runtime(uint64_t region_id) const = 0;
            /// @brief Returns the total time spent in MPI for a
            ///        region.
            /// @param [in] region_id The region ID.
            virtual double total_region_mpi_runtime(uint64_t region_id) const = 0;
            /// @brief Returns the total application runtime.
            virtual double total_app_runtime(void) const = 0;
            /// @brief Returns the total application energy.
            virtual double total_app_energy(void) const = 0;
            /// @brief Returns the total time spent in MPI for the
            ///        application.
            virtual double total_app_mpi_runtime(void) const = 0;
            /// @brief Returns the total time spent in ignored regionf
            ///        for the application.
            virtual double total_app_ignore_runtime(void) const = 0;
            /// @brief Returns the total runtime after the first epoch
            ///        call.
            virtual double total_epoch_runtime(void) const = 0;
            /// @brief Returns the total number of times a region was
            ///        entered.
            /// @param [in] region_id The region ID.
            virtual int total_count(uint64_t region_id) const = 0;
            /// @brief Check for updates from the application and
            ///        adjust totals accordingly.
            /// @param [in] comm Shared pointer to the comm used by
            ///        the Controller.
            virtual void update(std::shared_ptr<IComm> comm) = 0;
            /// @brief Returns the list of all regions entered or
            ///        exited since the last call to
            ///        clear_region_entry_exit().
            virtual std::list<std::pair<uint64_t, double> > region_entry_exit(void) const = 0;
            /// @brief Resets the internal list of region entries and
            ///        exits.
            virtual void clear_region_entry_exit(void) = 0;
    };

    class IProfileSampler;
    class IEpochRuntimeRegulator;
    class IKprofileIOSample;
    class IPlatformIO;
    class IPlatformTopo;

    class ApplicationIO : public IApplicationIO
    {
        public:
            ApplicationIO(const std::string &shm_key);
            ApplicationIO(const std::string &shm_key,
                          std::unique_ptr<IProfileSampler> sampler,
                          std::shared_ptr<IKprofileIOSample> pio_sample,
                          IPlatformIO &platform_io,
                          IPlatformTopo &platform_topo);
            virtual ~ApplicationIO();
            bool do_shutdown(void) const override;
            std::string report_name(void) const override;
            std::string profile_name(void) const override;
            std::set<std::string> region_name_set(void) const override;
            double total_region_runtime(uint64_t region_id) const override;
            double total_region_mpi_runtime(uint64_t region_id) const override;
            double total_app_runtime(void) const override;
            double total_app_energy(void) const override;
            double total_app_mpi_runtime(void) const override;
            double total_app_ignore_runtime(void) const override;
            double total_epoch_runtime(void) const override;
            int total_count(uint64_t region_id) const override;
            void update(std::shared_ptr<IComm> comm) override;
            std::list<std::pair<uint64_t, double> > region_entry_exit(void) const override;
            void clear_region_entry_exit(void) override;
        private:
            static constexpr size_t M_SHMEM_REGION_SIZE = 12288;

            void connect(void);
            double current_energy(void) const;

            std::unique_ptr<IProfileSampler> m_sampler;
            std::shared_ptr<IKprofileIOSample> m_profile_io_sample;
            std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > m_prof_sample;
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            std::vector<uint64_t> m_region_id;
            // Per rank vector counting number of entries into MPI.
            std::vector<uint64_t> m_num_mpi_enter;
            std::vector<bool> m_is_epoch_changed;
            bool m_do_shutdown;
            bool m_is_connected;
            int m_rank_per_node;
            std::unique_ptr<IEpochRuntimeRegulator> m_epoch_regulator;
            double m_start_energy;
    };
}

#endif

/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#ifndef MANAGERIO_HPP_INCLUDE
#define MANAGERIO_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <pthread.h>

namespace geopm
{
    class ISharedMemory;
    class ISharedMemoryUser;

    struct geopm_manager_shmem_header {
        pthread_mutex_t lock; // 40 bytes
        uint8_t is_updated;   // 1 byte + 7 bytes of padding
        size_t count;         // 8 bytes
        double values;        // 8 bytes
    };

    struct geopm_manager_shmem_s {
        /// @brief Lock to ensure r/w consistency between GEOPM and the resource manager.
        pthread_mutex_t lock;
        /// @brief Enables notification of updates to GEOPM.
        uint8_t is_updated;
        /// @brief Specifies the size of the following array.
        size_t count;
        /// @brief Holds resource manager data.
        double values[(4096 - offsetof(struct geopm_manager_shmem_header, values)) / sizeof(double)];
    };

    static_assert(sizeof(struct geopm_manager_shmem_s) == 4096, "Alignment issue with geopm_manager_shmem_s.");

    class IManagerIO
    {
        public:
            IManagerIO() = default;
            virtual ~IManagerIO() = default;
            /// @brief Set the value for a specific signal or policy
            ///        to be written.
            /// @param [in] signal_name Name of the signal or policy.
            /// @param [in] setting Value to set.
            virtual void adjust(const std::string &signal_name, double setting) = 0;
            /// @brief Set values for all signals or policies to be
            ///        written.
            /// @param [in] settings Vector of values for each signal
            ///        or policy, in the expected order.
            virtual void adjust(const std::vector<double> &settings) = 0;
            /// @brief Write updated values.
            virtual void write_batch(void) = 0;
            /// @brief Returns the expected signal or policy names.
            /// @return Vector of signal or policy names.
            virtual std::vector<std::string> signal_names(void) const = 0;
    };

    class ManagerIO : public IManagerIO
    {
        public:
            ManagerIO() = delete;
            ManagerIO(const ManagerIO &other) = delete;

            ManagerIO(const std::string &data_path, bool is_policy);
            ManagerIO(const std::string &data_path, bool is_policy, const std::string &agent_name);
            ManagerIO(const std::string &data_path,
                      std::unique_ptr<ISharedMemory> shmem,
                      const std::vector<std::string> &signal_names);

            ~ManagerIO() = default;
            void adjust(const std::string &signal_name, double setting) override;
            void adjust(const std::vector<double> &settings) override;
            void write_batch(void) override;
            std::vector<std::string> signal_names(void) const override;
            static void setup_mutex(pthread_mutex_t &lock);

        private:
            void write_file();
            void write_shmem();

            std::string m_path;
            std::vector<std::string> m_signal_names;
            std::unique_ptr<ISharedMemory> m_shmem;
            struct geopm_manager_shmem_s *m_data;
            std::vector<double> m_samples_up;
            bool m_is_shm_data;
    };

    class IManagerIOSampler
    {
        public:
            IManagerIOSampler() = default;
            virtual ~IManagerIOSampler() = default;
            /// @brief Read values from the resource manager.
            virtual void read_batch(void) = 0;
            /// @brief Returns the most recent value for the given
            ///        signal or policy.
            /// @param [in] signal_name Name of the signal or policy.
            /// @return Value of the signal or policy.
            virtual double sample(const std::string &signal_name) const = 0;
            /// @brief Returns all the latest values.
            /// @return Vector of signal or policy values.
            virtual std::vector<double> sample(void) const = 0;
            /// @brief Indicates whether or not the values have been
            ///        updated.
            virtual bool is_update_available(void) = 0;
            /// @brief Returns the signal or policy names expected by
            ///        the resource manager.
            /// @return Vector of signal or policy names.
            virtual std::vector<std::string> signal_names(void) const = 0;
    };

    class ManagerIOSampler : public IManagerIOSampler
    {
        public:
            ManagerIOSampler() = delete;
            ManagerIOSampler(const ManagerIOSampler &other) = delete;

            ManagerIOSampler(const std::string &data_path, bool is_policy);
            ManagerIOSampler(const std::string &data_path, bool is_policy, const std::string &agent_name);
            ManagerIOSampler(const std::string &data_path,
                             std::unique_ptr<ISharedMemoryUser> shmem,
                             const std::vector<std::string> &signal_names);

            ~ManagerIOSampler() = default;
            void read_batch(void) override;
            double sample(const std::string &signal_name) const override;
            std::vector<double> sample(void) const override;
            bool is_update_available(void) override;
            std::vector<std::string> signal_names(void) const override;

        private:
            bool is_valid_signal(const std::string &signal_name) const;
            std::map<std::string, double> parse_json(void);
            void read_shmem(void);

            std::string m_path;
            std::vector<std::string> m_signal_names;
            std::unique_ptr<ISharedMemoryUser> m_shmem;
            struct geopm_manager_shmem_s *m_data;
            std::vector<double> m_signals_down;
            const bool m_is_shm_data;
    };
}

#endif

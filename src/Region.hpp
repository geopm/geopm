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

#ifndef REGION_HPP_INCLUDE
#define REGION_HPP_INCLUDE

#include <stdint.h>
#include <string>
#include <stack>

#include "Policy.hpp"
#include "CircularBuffer.hpp"

namespace geopm
{
    /// @brief This class encapsulates all recorded data for a
    ///        specific application execution region.
    class Region
    {
        public:
            enum m_const_e {
                // If number of samples stored is large, we need to
                // modify the derivative method to just use the last
                // few samples.
                M_NUM_SAMPLE_HISTORY = 8,
            };
            /// @brief Default constructor.
            /// @param [in] identifier Unique 64 bit region identifier.
            /// @param [in] num_domain Number of control domains.
            Region(uint64_t identifier, int num_domain, int level);
            /// @brief Default destructor.
            virtual ~Region();
            /// @brief Record an entry into the region.
            void entry(void);
            /// @brief Return the number of entries into the region.
            int num_entry(void);
            /// @brief Insert signal data into internal buffers
            ///
            /// Inserts hw telemetry and per-domain application data into the
            /// internal buffers for the region. This API is used by leaf
            /// level objects.
            ///
            /// @param [in] A vector of telemetry samples to be inserted.
            void insert(std::vector<struct geopm_telemetry_message_s> &telemetry);
            /// @brief Insert signal data into internal buffers
            ///
            /// Inserts aggregated sample message and data into the internal buffers
            /// for the region. This API is used by tree level objects.
            ///
            /// @param [in] A vector of sample messages to be inserted.
            void insert(const std::vector<struct geopm_sample_message_s> &sample);
            /// @brief Clear data from internal buffers
            ///
            /// Clears aggregated data from the internal buffers.
            void clear(void);
            /// @brief Retrieve the unique region identifier.
            /// @return 64 bit region identifier.
            uint64_t identifier(void) const;
            /// @brief Add increment amount to the total time spent in MPI calls
            //  during this region.
            //  @param [in] mpi_increment_amout Value to add to the MPI time total.
            void increment_mpi_time(double mpi_increment_amount);
            /// @brief Return an aggregated sample to send up the tree.
            /// Called once this region has converged to send a sample
            /// up to the next level of the tree.
            /// @param [out] Sample message structure to fill in.
            void sample_message(struct geopm_sample_message_s &sample);
            /// Returns the latest value
            double signal(int domain_idx, int signal_type);
            /// @brief Retrieve the number of valid samples for a domain of control.
            ///
            /// Get the number of valid samples  for a given domain of control and
            /// a signal type for the buffered data associated with the application
            /// region. This is the number of samples used to calculate the other
            /// statistics.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return The number of valid samples.
            int num_sample(int domain_idx, int signal_type) const;
            /// @brief Retrieve the mean signal value for a domain of control.
            ///
            /// Get the mean signal value for a given domain of control and
            /// a signal type for the buffered data associated with the application
            /// region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return The mean signal value.
            double mean(int domain_idx, int signal_type) const;
            /// @brief Retrieve the median signal value for a domain of control.
            ///
            /// Get the median signal value for a given domain of control and
            /// a signal type for the buffered data associated with the application
            /// region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return The median signal value.
            double median(int domain_idx, int signal_type) const;
            /// @brief Retrieve the standard deviation of the signal values for a domain of control.
            ///
            /// Get the standard deviation of the signal values for a given domain of
            /// control and a signal type for the buffered data associated with the
            /// application region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return The standard deviation of the signal values.
            double std_deviation(int domain_idx, int signal_type) const;
            /// @brief Retrieve the min signal value for a domain of control.
            ///
            /// Get the min signal value for a given domain of control and
            /// a signal type for the buffered data associated with the application
            /// region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return The min signal value.
            double min(int domain_idx, int signal_type) const;
            /// @brief Retrieve the max signal value for a domain of control.
            ///
            /// Get the max signal value for a given domain of control and
            /// a signal type for the buffered data associated with the application
            /// region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return The max signal value.
            double max(int domain_idx, int signal_type) const;
            /// @brief Retrieve the derivative of the signal values for a domain of control.
            ///
            /// Get the derivative of the signal values for a given domain of control and
            /// a signal type for the buffered data associated with the application
            /// region. There must be at least 2 valid samples recorded.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @return If there are 2 valid samples then return he derivative of the
            /// signal values, else return NAN.
            double derivative(int domain_idx, int signal_type);
            /// @brief Integrate a signal over time.
            ///
            /// Computes the integral of the signal over the interval
            /// of time spanned by the samples stored in the region
            /// which where gathered since the applications most
            /// recent entry into the region.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            double integral(int domain_idx, int signal_type, double &delta_time, double &integral) const;
            void report(std::ostringstream &string_stream, const std::string &name, int rank_per_node) const;
        protected:
            /// @brief Bound testing of input parameters.
            ///
            /// Checks the requested domain index and signal type to make sure thay are within
            /// bounds of our internal data structures. If they are not an exception is thrown.
            ///
            /// @param [in] domain_idx The index to the domain of
            ///        control as ordered in the Platform and the
            ///        Policy.
            ///
            /// @param [in] signal_type The signal type requested as
            ///        enumerated in geopm_signal_type_e in
            ///        geopm_message.h.
            ///
            /// @param [in] file Name of source file where exception
            ///        was thrown, e.g. preprocessor __FILE__.
            ///
            /// @param [in] line Line number in source file where
            ///        exception was thrown, e.g. preprocessor
            ///        __LINE__.
            ///
            void check_bounds(int domain_idx, int signal_type, const char *file, int line) const;
            double domain_buffer_value(int buffer_idx, int domain_idx, int signal_type);
            bool is_telemetry_entry(const struct geopm_telemetry_message_s &telemetry, int domain_idx);
            bool is_telemetry_exit(const struct geopm_telemetry_message_s &telemetry, int domain_idx);
            void update_domain_sample(const struct geopm_telemetry_message_s &telemetry, int domain_idx);
            void update_signal_matrix(const double *signal, int domain_idx);
            void update_valid_entries(const struct geopm_telemetry_message_s &telemetry, int domain_idx);
            void update_stats(const double *signal, int domain_idx);
            void update_curr_sample(void);
            /// @brief Holds a unique 64 bit region identifier.
            const uint64_t m_identifier;
            /// @brief Numnber of domains reporting to the region.
            const unsigned m_num_domain;
            /// @brief The level of the tree where the region resides
            const unsigned m_level;
            /// @brief The number of distinct signal in a single domain.
            int m_num_signal;
            /// @brief Temporary signal matrix storeage used before insertion.
            std::vector<double> m_signal_matrix;
            /// @brief Holder for telemerty state on region entry.
            std::vector<struct geopm_telemetry_message_s> m_entry_telemetry;
            /// @brief the current sample message to be sent up the tree.
            struct geopm_sample_message_s m_curr_sample;
            /// @brief Holder for sample data calculated after a domain exits a region.
            std::vector<struct geopm_sample_message_s> m_domain_sample;
            /// @brief Circular buffer is over time, vector is indexed over both domains and signals.
            CircularBuffer<std::vector<double> > m_domain_buffer;
            /// @brief time stamp for each entry in the m_domain_buffer.
            CircularBuffer<struct geopm_time_s> m_time_buffer;
            /// @brief the number of valid samples per domain and signal type.
            std::vector<int> m_valid_entries;
            /// @brief the current minimum signal value per domain and signal type.
            std::vector<double> m_min;
            /// @brief the current maximum signal value per domain and signal type.
            std::vector<double> m_max;
            /// @brief the current sum of signal values per domain and signal type.
            std::vector<double> m_sum;
            /// @brief the current sum of squares of signal values per domain and signal type.
            std::vector<double> m_sum_squares;
            std::vector<double> m_derivative_last;
            struct geopm_sample_message_s m_agg_stats;
            uint64_t m_num_entry;
            std::vector<bool> m_is_entered;
            int m_derivative_num_fit;
            double m_mpi_time;
            double m_last_runtime;
    };
}

#endif

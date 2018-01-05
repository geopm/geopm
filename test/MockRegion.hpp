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

class MockRegion : public geopm::IRegion {
    public:
        MOCK_METHOD0(entry,
                void(void));
        MOCK_METHOD0(num_entry,
                int(void));
        MOCK_METHOD1(insert,
                void(std::vector<struct geopm_telemetry_message_s> &telemetry));
        MOCK_METHOD1(insert,
                void(const std::vector<struct geopm_sample_message_s> &sample));
        MOCK_METHOD0(clear,
                void(void));
        MOCK_CONST_METHOD0(identifier,
                uint64_t(void));
        MOCK_CONST_METHOD0(hint,
                uint64_t(void));
        MOCK_METHOD1(increment_mpi_time,
                void(double mpi_increment_amount));
        MOCK_METHOD1(sample_message,
                void(struct geopm_sample_message_s &sample));
        MOCK_METHOD2(signal,
                double(int domain_idx, int signal_type));
        MOCK_CONST_METHOD2(num_sample,
                int(int domain_idx, int signal_type));
        MOCK_CONST_METHOD2(mean,
                double(int domain_idx, int signal_type));
        MOCK_CONST_METHOD2(median,
                double(int domain_idx, int signal_type));
        MOCK_CONST_METHOD2(std_deviation,
                double(int domain_idx, int signal_type));
        MOCK_CONST_METHOD2(min,
                double(int domain_idx, int signal_type));
        MOCK_CONST_METHOD2(max,
                double(int domain_idx, int signal_type));
        MOCK_METHOD2(derivative,
                double(int domain_idx, int signal_type));
        MOCK_CONST_METHOD4(integral,
                double(int domain_idx, int signal_type, double &delta_time, double &integral));
        MOCK_CONST_METHOD3(report,
                void(std::ostringstream &string_stream, const std::string &name, int rank_per_node));
        MOCK_METHOD1(thread_progress,
                void(std::vector<double> &progress));
        MOCK_METHOD1(telemetry_timestamp,
                struct geopm_time_s(size_t sample_idx));
};

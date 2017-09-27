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

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sstream>
#include <utility>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MSR.hpp"
#include "MockMSRIO.hpp"
#include "PlatformTopology.hpp"
#include "Exception.hpp"

using testing::_;
using testing::Invoke;
using testing::Sequence;

#if 0

class MSRSignalTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        int m_cpu_idx;
        MockMSRIO m_msrio;
        std::vector<const geopm::IMSR *> m_prog_msr;
        std::vector<std::string> m_prog_field_name;
        std::vector<double> m_prog_value;
        std::vector<geopm::IMSRSignal *> m_msr_sigs;
        std::string m_name;
        uint64_t m_offset;
        std::vector<std::pair<std::string, struct geopm::IMSR::m_encode_s> > m_signals;
        std::vector<std::pair<std::string, struct geopm::IMSR::m_encode_s> > m_controls;
};

void MSRSignalTest::SetUp()
{
    m_cpu_idx = 0;
    m_name = "test-msr";
    m_offset = 0xDEADBEEF;
    m_signals = {std::pair<std::string, struct geopm::IMSR::m_encode_s> ("sig1", {0, 8, 1.0}),
                 std::pair<std::string, struct geopm::IMSR::m_encode_s> ("sig2", {8, 16, 2.0})};
    m_controls = {std::pair<std::string, struct geopm::IMSR::m_encode_s> ("ctl1", {0, 8, 1.0}),
                 std::pair<std::string, struct geopm::IMSR::m_encode_s> ("ctl2", {8, 16, 2.0}),
                 std::pair<std::string, struct geopm::IMSR::m_encode_s> ("ctl3", {27, 56, 4.0})};
    geopm::MSR *stage0 = new geopm::MSR("stage0", 2, m_signals, m_controls);
    geopm::MSR *stage1 = new geopm::MSR("stage1", 8, m_signals, m_controls);
    geopm::MSR *stage2 = new geopm::MSR("stage2", 16, m_signals, m_controls);
    m_prog_msr = {stage0, stage1};//, stage2};
    m_prog_field_name = {"ctl1", "ctl2"};//, "ctl3"};
    m_prog_value = {69.0, 72.0};//, 99.99};

    m_msr_sigs.resize(m_signals.size());
    int idx = 0;
    for (auto msr : m_prog_msr) {
        m_msr_sigs[idx] = new geopm::MSRSignal(m_prog_msr[idx], m_cpu_idx, idx);
        idx++;
    }
}

void MSRSignalTest::TearDown()
{
    for (auto msr_it = m_prog_msr.begin(); msr_it != m_prog_msr.end(); ++msr_it) {
        free((void *)(*msr_it));
    }

    for (auto sig : m_msr_sigs) {
        free(sig);
    }
}

#define MSG_2_IMPLEMENTOR "Congrats, you've implemented the API.  Now update the test."
TEST_F(MSRSignalTest, msr)
{
    std::string name;
    std::string expected_names[] = {"stage0:sig1", "stage1:sig2"};
    int idx = 0;
    uint64_t data = 0xDEADBEEFD00D;
    const uint64_t field = (uint64_t) &data;
    for (auto sig : m_msr_sigs) {
        std::vector<uint64_t> offset;
        EXPECT_EQ(expected_names[idx], sig->name());
        EXPECT_THROW(sig->domain_type(), geopm::Exception) << MSG_2_IMPLEMENTOR;
        EXPECT_THROW(sig->domain_idx(), geopm::Exception) << MSG_2_IMPLEMENTOR;
        EXPECT_THROW(sig->sample(), geopm::Exception);
        EXPECT_EQ(1, sig->num_msr());
        //todo map, then sample again
        sig->offset(offset);
        // expectations on size and content of offset
        sig->map_field(&field);
        EXPECT_EQ(sig->sample(), field);
        idx++;
    }


    //m_msr_sigs.resize(m_signals.size());
    //for (auto signal_it = m_signals.begin(); signal_it != m_signals.end(); ++signal_it, idx++) {
        //m_msr_sig[idx] = new geopm::MSRSignal((*signal_it).second, m_cpu_idx, idx);
        //m_msr->signal_name(idx, name);
        //EXPECT_EQ((*signal_it).first, name);
        //EXPECT_EQ(m_msr->signal_index((*signal_it).first), idx);
    //}

    //uint64_t field = 144;
    //field |= 96 << 8;
    //double value = m_msr->signal(0, field);
    //EXPECT_EQ(144, value);

    //value = m_msr->signal(1, field);
    //EXPECT_EQ(192, value);

    //EXPECT_THROW(m_msr->signal_name(idx, name), geopm::Exception);
    //delete m_msr;
}

TEST_F(MSRSignalTest, prog_counter)
{
    int offset = 1010101;
    m_msr = new geopm::MSR(m_name, m_signals, m_prog_msr, m_prog_field_name, m_prog_value);
    std::string name;
    m_msr->name(name);

    Sequence s1;
    int idx = 0;
    for (auto msr_it = m_prog_msr.begin(); msr_it != m_prog_msr.end(); ++msr_it, idx++) {
        struct geopm::IMSR::m_encode_s encode = m_controls[idx].second;
        double value = m_prog_value[idx];
        EXPECT_CALL(m_msrio, write_msr(m_cpu_idx, _, _, _))
            .InSequence(s1)
            .WillOnce(Invoke([this, msr_it, encode, value] (int cpu_idx, uint64_t offset, uint64_t raw_value, uint64_t write_mask)
                {
                    int shift = encode.begin_bit;
                    double scalar = encode.scalar;
                    double inverse = (1.0 / scalar);
                    uint64_t mask = (((1ULL << (encode.end_bit - shift)) - 1) << shift);
                    EXPECT_EQ((*msr_it)->offset(), offset);
                    EXPECT_EQ(raw_value, mask);
                    EXPECT_EQ(write_mask, ((uint64_t)(value * inverse) << shift) & mask);
                }));
    }

    m_msr->program(offset, m_cpu_idx, &m_msrio);
    EXPECT_EQ(offset, m_msr->offset());
    delete m_msr;
}
#endif

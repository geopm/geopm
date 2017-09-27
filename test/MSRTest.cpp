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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MSR.hpp"
#include "MockMSRIO.hpp"
#include "PlatformTopology.hpp"
#include "PlatformIO.hpp"
#include "Exception.hpp"

using testing::_;
using testing::Invoke;
using testing::Sequence;

using geopm::IMSR;
using geopm::MSR;
using geopm::IPlatformIO;

class MSRTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        int m_cpu_idx;
        MockMSRIO m_msrio;
        std::vector<const IMSR *> m_prog_msr;
        std::vector<std::string> m_prog_field_name;
        std::vector<double> m_prog_value;
        IMSR *m_msr;
        std::string m_name;
        uint64_t m_offset;
        std::vector<std::pair<std::string, struct IMSR::m_encode_s> > m_signals;
        std::vector<std::pair<std::string, struct IMSR::m_encode_s> > m_controls;
        uint64_t m_signal_field;
        double m_control_value;
        std::vector<double> m_expected_sig_values;
        std::vector<uint64_t> m_expected_ctl_fields;
};

void MSRTest::SetUp()
{
    m_cpu_idx = 0;
    m_name = "test-msr";
    m_offset = 0xDEADBEEF;
    m_control_value = 314159.265359;
    m_expected_ctl_fields = {216, 4294967277, 77309411328};
    m_signal_field = 144;
    m_signal_field |= 96 << 8;
    m_expected_sig_values = {144.0, 131072, 3};
    m_signals = {std::pair<std::string, struct IMSR::m_encode_s> ("sig1", (struct IMSR::m_encode_s) {
                                                                           .begin_bit = 0,
                                                                           .end_bit   = 8,
                                                                           .domain    = IPlatformIO::M_DOMAIN_CPU,
                                                                           .function  = IMSR::M_FUNCTION_SCALE,
                                                                           .units     = IMSR::M_UNITS_NONE,
                                                                           .scalar    = 1.0}),
                 std::pair<std::string, struct IMSR::m_encode_s> ("sig2", (struct IMSR::m_encode_s) {
                                                                           .begin_bit = 0,
                                                                           .end_bit   = 32,
                                                                           .domain    = IPlatformIO::M_DOMAIN_CPU,
                                                                           .function  = IMSR::M_FUNCTION_LOG_HALF,
                                                                           .units     = IMSR::M_UNITS_NONE,
                                                                           .scalar    = 2.0}),
                 std::pair<std::string, struct IMSR::m_encode_s> ("sig3", (struct IMSR::m_encode_s) {
                                                                           .begin_bit = 0,
                                                                           .end_bit   = 64,
                                                                           .domain    = IPlatformIO::M_DOMAIN_CPU,
                                                                           .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                                                                           .units     = IMSR::M_UNITS_NONE,
                                                                           .scalar    = 3.0})};
    m_controls = {std::pair<std::string, struct IMSR::m_encode_s> ("ctl1", (struct IMSR::m_encode_s) {
                                                                            .begin_bit = 0,
                                                                            .end_bit   = 8,
                                                                            .domain    = IPlatformIO::M_DOMAIN_CPU,
                                                                            .function  = IMSR::M_FUNCTION_SCALE,
                                                                            .units     = IMSR::M_UNITS_NONE,
                                                                            .scalar    = 0.1}),
                  std::pair<std::string, struct IMSR::m_encode_s> ("ctl2", (struct IMSR::m_encode_s) {
                                                                            .begin_bit = 0,
                                                                            .end_bit   = 32,
                                                                            .domain    = IPlatformIO::M_DOMAIN_CPU,
                                                                            .function  = IMSR::M_FUNCTION_LOG_HALF,
                                                                            .units     = IMSR::M_UNITS_NONE,
                                                                            .scalar    = 0.5}),
                  std::pair<std::string, struct IMSR::m_encode_s> ("ctl3", (struct IMSR::m_encode_s) {
                                                                            .begin_bit = 32,
                                                                            .end_bit   = 64,
                                                                            .domain    = IPlatformIO::M_DOMAIN_CPU,
                                                                            .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                                                                            .units     = IMSR::M_UNITS_NONE,
                                                                            .scalar    = 1.0})};
    MSR *stage0 = new MSR("stage0", 2, m_signals, m_controls);
    MSR *stage1 = new MSR("stage1", 8, m_signals, m_controls);
    MSR *stage2 = new MSR("stage2", 16, m_signals, m_controls);
    m_prog_msr = {stage0, stage1, stage2};
    m_prog_field_name = {"ctl1", "ctl2", "ctl3"};
    m_prog_value = {69.0, 72.0, 99.99};
}

void MSRTest::TearDown()
{
    for (auto msr_it = m_prog_msr.begin(); msr_it != m_prog_msr.end(); ++msr_it) {
        free((void *)(*msr_it));
    }
}

TEST_F(MSRTest, msr)
{
    m_msr = new MSR(m_name, m_offset, m_signals, m_controls);

    EXPECT_EQ(m_name, m_msr->name());
    EXPECT_EQ(m_offset, m_msr->offset());
    EXPECT_EQ(m_signals.size(), (unsigned)m_msr->num_signal());
    EXPECT_EQ(m_controls.size(), (unsigned)m_msr->num_control());
    EXPECT_EQ(geopm::GEOPM_DOMAIN_CPU, m_msr->domain_type());

    // signals
    int idx = 0;
    for (auto signal_it = m_signals.begin(); signal_it != m_signals.end(); ++signal_it, idx++) {
        EXPECT_EQ((*signal_it).first, m_msr->signal_name(idx)) << "idx: " << idx;
        EXPECT_EQ(m_msr->signal_index((*signal_it).first), idx) << "idx: " << idx;
        double value = m_msr->signal(idx, m_signal_field);
        EXPECT_DOUBLE_EQ(m_expected_sig_values[idx], value) << "idx: " << idx;
    }

    // controls
    idx = 0;
    uint64_t field, mask;
    for (auto control_it = m_controls.begin(); control_it != m_controls.end(); ++control_it, idx++) {
        int shift = (*control_it).second.begin_bit;
        EXPECT_EQ((*control_it).first, m_msr->control_name(idx)) << "idx: " << idx;
        EXPECT_EQ(m_msr->control_index((*control_it).first), idx) << "idx: " << idx;
        m_msr->control(idx, m_control_value, field, mask);
        EXPECT_EQ((((1ULL << ((*control_it).second.end_bit - shift)) - 1) << shift), mask) << "idx: " << idx;
        EXPECT_EQ(m_expected_ctl_fields[idx], field) << "idx: " << idx;
    }

    EXPECT_THROW(m_msr->control(idx, 80000000000000.0, field, mask), geopm::Exception);

    idx = -1;
    EXPECT_THROW(m_msr->signal_name(idx), geopm::Exception);
    EXPECT_THROW(m_msr->control_name(idx), geopm::Exception);
    delete m_msr;
}

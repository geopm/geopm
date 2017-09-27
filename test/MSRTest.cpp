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
using geopm::IMSR;
using geopm::MSR;
using geopm::MSRSignal;
using geopm::IMSRSignal;
using geopm::MSRControl;
using geopm::IMSRControl;
using geopm::IPlatformIO;

class MSRTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void ConfigSignals();
        void ConfigControls();

        // signals and controls config
        int m_cpu_idx;
        std::vector<int> m_domain_types;
        std::vector<int> m_function_types;
        std::vector<int> m_unit_types;

        // signals config
        const int M_NUM_SIGNALS = 3;
        std::vector<std::pair<std::string, struct IMSR::m_encode_s> > m_signals;
        std::vector<std::string> m_signal_names;
        std::vector<int> m_sig_begin_bits;
        std::vector<int> m_sig_end_bits;
        std::vector<double> m_signal_scalars;
        uint64_t m_signal_field;
        std::vector<double> m_expected_sig_values;

        // controls config
        const int M_NUM_CONTROLS = 3;
        std::vector<std::pair<std::string, struct IMSR::m_encode_s> > m_controls;
        std::vector<std::string> m_control_names;
        std::vector<int> m_con_begin_bits;
        std::vector<int> m_con_end_bits;
        std::vector<double> m_control_scalars;
        double m_control_value;
        std::vector<uint64_t> m_expected_con_fields;

        // MSR
        std::vector<std::string> m_msr_names;
        std::vector<int> m_msr_offsets;
        std::vector<const IMSR *> m_msrs;
};

void MSRTest::SetUp()
{
    m_cpu_idx = 0;
    m_domain_types = {IPlatformIO::M_DOMAIN_CPU, IPlatformIO::M_DOMAIN_CPU, IPlatformIO::M_DOMAIN_CPU};
    m_function_types = {IMSR::M_FUNCTION_SCALE, IMSR::M_FUNCTION_LOG_HALF, IMSR::M_FUNCTION_7_BIT_FLOAT};
    m_unit_types = {IMSR::M_UNITS_NONE, IMSR::M_UNITS_NONE, IMSR::M_UNITS_NONE};

    ConfigSignals();
    ConfigControls();

    m_msr_names = {"test_msr_0", "test_msr_1", "test_msr_2"};
    m_msr_offsets = {2, 8, 16};

    MSR *stage0 = new MSR(m_msr_names[0], m_msr_offsets[0], m_signals, {});         // signal only
    MSR *stage1 = new MSR(m_msr_names[1], m_msr_offsets[1], {}, m_controls);        // control only
    MSR *stage2 = new MSR(m_msr_names[2], m_msr_offsets[2], m_signals, m_controls); // signal and control

    m_msrs = {stage0, stage1, stage2};
}

void MSRTest::TearDown()
{
    for (auto msr_it = m_msrs.begin(); msr_it != m_msrs.end(); ++msr_it) {
        delete *msr_it;
    }
}

void MSRTest::ConfigSignals()
{
    m_signal_names = {"sig1", "sig2", "sig3"};
    m_sig_begin_bits = {0, 0, 0};
    m_sig_end_bits = {8, 32, 64};
    m_signal_scalars = {1.0, 2.0, 3.0};
    m_signal_field = 144;
    m_signal_field |= 96 << 8;
    m_expected_sig_values = {144.0, 131072, 3};

    ASSERT_EQ(M_NUM_SIGNALS, m_signal_names.size());
    ASSERT_EQ(M_NUM_SIGNALS, m_sig_begin_bits.size());
    ASSERT_EQ(M_NUM_SIGNALS, m_sig_end_bits.size());
    ASSERT_EQ(M_NUM_SIGNALS, m_signal_scalars.size());
    ASSERT_EQ(M_NUM_SIGNALS, m_expected_sig_values.size());

    m_signals.resize(M_NUM_SIGNALS);
    for (int idx = 0; idx < M_NUM_SIGNALS; idx++) {
                 m_signals[idx] = std::pair<std::string, struct IMSR::m_encode_s> (m_signal_names[idx], (struct IMSR::m_encode_s) {
                                                                           .begin_bit = m_sig_begin_bits[idx],
                                                                           .end_bit   = m_sig_end_bits[idx],
                                                                           .domain    = m_domain_types[idx],
                                                                           .function  = m_function_types[idx],
                                                                           .units     = m_unit_types[idx],
                                                                           .scalar    = m_signal_scalars[idx]});
    }
}

void MSRTest::ConfigControls()
{
    m_control_names = {"ctl1", "ctl2", "ctl3"};
    m_con_begin_bits = {0, 0, 32};
    m_con_end_bits = {8, 32, 64};
    m_control_scalars = {0.1, 0.5, 1.0};
    m_control_value = 314159.265359;
    m_expected_con_fields = {216, 4294967277, 77309411328};

    ASSERT_EQ(M_NUM_CONTROLS, m_control_names.size());
    ASSERT_EQ(M_NUM_CONTROLS, m_con_begin_bits.size());
    ASSERT_EQ(M_NUM_CONTROLS, m_con_end_bits.size());
    ASSERT_EQ(M_NUM_CONTROLS, m_control_scalars.size());
    ASSERT_EQ(M_NUM_CONTROLS, m_expected_con_fields.size());

    m_controls.resize(M_NUM_CONTROLS);
    for (int idx = 0; idx < M_NUM_CONTROLS; idx++) {
                 m_controls[idx] = std::pair<std::string, struct IMSR::m_encode_s> (m_control_names[idx], (struct IMSR::m_encode_s) {
                                                                           .begin_bit = m_con_begin_bits[idx],
                                                                           .end_bit   = m_con_end_bits[idx],
                                                                           .domain    = m_domain_types[idx],
                                                                           .function  = m_function_types[idx],
                                                                           .units     = m_unit_types[idx],
                                                                           .scalar    = m_control_scalars[idx]});
    }
}

TEST_F(MSRTest, MSR)
{
    const IMSR *msr = m_msrs[2];
    uint64_t field = 0, mask = 0;
    EXPECT_THROW(msr->control(2, 80000000000000.0, field, mask), geopm::Exception);
    EXPECT_THROW(msr->control(2, -1.0, field, mask), geopm::Exception);
    EXPECT_THROW(msr->signal_name(-1), geopm::Exception);
    EXPECT_THROW(msr->control_name(-1), geopm::Exception);

    int msr_idx = 0;
    for (auto msr_it = m_msrs.begin(); msr_it != m_msrs.end(); ++msr_it, msr_idx++) {
        msr = *msr_it;
        EXPECT_EQ(m_msr_names[msr_idx], msr->name());
        EXPECT_EQ(m_msr_offsets[msr_idx], msr->offset());
        EXPECT_EQ(m_domain_types[msr_idx], msr->domain_type());

        if (msr_idx == 0 || msr_idx == 2) {
            EXPECT_EQ(M_NUM_SIGNALS, (unsigned)msr->num_signal());
        }
        if (msr_idx == 1 || msr_idx == 2) {
            EXPECT_EQ(M_NUM_CONTROLS, (unsigned)msr->num_control());
        }

        // signals
        for (int signal_idx = 0; signal_idx < msr->num_signal(); signal_idx++) {
            EXPECT_EQ(m_signal_names[signal_idx], msr->signal_name(signal_idx)) << "signal_idx: " << signal_idx;
            EXPECT_EQ(signal_idx, msr->signal_index(m_signal_names[signal_idx])) << "signal_idx: " << signal_idx;
            double value = msr->signal(signal_idx, m_signal_field);
            EXPECT_DOUBLE_EQ(m_expected_sig_values[signal_idx], value) << "signal_idx: " << signal_idx;
        }

        // controls
        for (int control_idx = 0; control_idx < msr->num_control(); control_idx++) {
            field = 0;
            mask = 0;
            EXPECT_EQ(m_control_names[control_idx], msr->control_name(control_idx)) << "control_idx: " << control_idx;
            EXPECT_EQ(control_idx, msr->control_index(m_control_names[control_idx])) << "control_idx: " << control_idx;
            msr->control(control_idx, m_control_value, field, mask);
            EXPECT_EQ((((1ULL << (m_con_end_bits[control_idx] - m_con_begin_bits[control_idx])) - 1) << m_con_begin_bits[control_idx]), mask) << "control_idx: " << control_idx;
            EXPECT_EQ(m_expected_con_fields[control_idx], field) << "control_idx: " << control_idx;
        }
    }
}

#define MSG_2_IMPLEMENTOR "Congrats, you've implemented the API.  Now update the test."
TEST_F(MSRTest, MSRSignal)
{
    int msr_idx = 0;
    int sig_idx = 0;
    std::vector<uint64_t> offset;
    IMSRSignal *sig = new MSRSignal(m_msrs[msr_idx], m_cpu_idx, sig_idx);

    EXPECT_EQ((m_msr_names[msr_idx] + ":" + m_signal_names[sig_idx]), sig->name());
    EXPECT_THROW(sig->domain_type(), geopm::Exception) << MSG_2_IMPLEMENTOR;
    EXPECT_THROW(sig->domain_idx(), geopm::Exception) << MSG_2_IMPLEMENTOR;
    EXPECT_THROW(sig->sample(), geopm::Exception);
    EXPECT_EQ(1, sig->num_msr());
    sig->offset(offset);
    EXPECT_EQ(m_msr_offsets[msr_idx], offset[0]);
    sig->map_field(&m_signal_field);
    EXPECT_EQ(m_expected_sig_values[sig_idx], sig->sample());

    delete sig;
}

TEST_F(MSRTest, MSRControl)
{
    int msr_idx = 1;
    int con_idx = 0;
    std::vector<uint64_t> offset;
    uint64_t field = 0, mask = 0;
    IMSRControl *con = new MSRControl(m_msrs[msr_idx], m_cpu_idx, con_idx);

    EXPECT_EQ((m_msr_names[msr_idx] + ":" + m_control_names[con_idx]), con->name());
    EXPECT_THROW(con->domain_type(), geopm::Exception) << MSG_2_IMPLEMENTOR;
    EXPECT_THROW(con->domain_idx(), geopm::Exception) << MSG_2_IMPLEMENTOR;
    EXPECT_THROW(con->adjust(m_control_value), geopm::Exception);
    EXPECT_EQ(1, con->num_msr());
    con->offset(offset);
    EXPECT_EQ(m_msr_offsets[msr_idx], offset[0]);
    con->map_field(&field, &mask);
    con->adjust(m_control_value);
    EXPECT_EQ((((1ULL << (m_con_end_bits[con_idx] - m_con_begin_bits[con_idx])) - 1) << m_con_begin_bits[con_idx]), mask);
    EXPECT_EQ(m_expected_con_fields[con_idx], field);

    delete con;
}

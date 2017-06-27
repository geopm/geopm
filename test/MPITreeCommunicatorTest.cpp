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

#include <mpi.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "gtest/gtest.h"
#include "TreeCommunicator.hpp"
#include "Controller.hpp"
#include "GlobalPolicy.hpp"
#include "geopm_policy.h"
#include "Exception.hpp"
#include "MPIComm.hpp"

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

class MPITreeCommunicatorTest: public :: testing :: Test
{
    public:
        MPITreeCommunicatorTest();
        ~MPITreeCommunicatorTest();
    protected:
        geopm::TreeCommunicator *m_tcomm;
        geopm::GlobalPolicy *m_polctl;
        const std::string m_ctl_path;
};


MPITreeCommunicatorTest::MPITreeCommunicatorTest()
    : m_tcomm(NULL)
    , m_polctl(NULL)
    , m_ctl_path("/tmp/MPIControllerTest.hello.control")
{
    int rank;
    std::vector<int> factor(2);
    factor[0] = 2;
    factor[1] = 8;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!rank) {
        m_polctl = new geopm::GlobalPolicy("", m_ctl_path);
        m_polctl->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
        m_polctl->frequency_mhz(1200);
        m_polctl->write();
    }

    geopm::IComm *tmp_comm = new geopm::MPIComm();
    m_tcomm = new geopm::TreeCommunicator(factor, m_polctl, tmp_comm);
    delete tmp_comm;
}


MPITreeCommunicatorTest::~MPITreeCommunicatorTest()
{
    if (m_polctl) {
        unlink(m_ctl_path.c_str());
        delete m_polctl;
    }
    delete m_tcomm;
}


TEST_F(MPITreeCommunicatorTest, hello)
{
    EXPECT_EQ(1, m_tcomm->num_level() > 0 && m_tcomm->num_level() <= 3);
    EXPECT_EQ(1, m_tcomm->root_level() == 2);
    EXPECT_EQ(1, m_tcomm->level_size(0) == 8);
    EXPECT_EQ(1, m_tcomm->level_size(1) == 2);
    EXPECT_EQ(1, m_tcomm->level_size(2) == 1);
}

TEST_F(MPITreeCommunicatorTest, send_policy_down)
{
    int success;
    struct geopm_policy_message_s policy = {0};
    std::vector <struct geopm_policy_message_s> send_policy;

    for (int level = m_tcomm->num_level() - 1; level > 0; --level) {
        if (level == m_tcomm->root_level()) {
            m_tcomm->get_policy(level, policy);
            policy.flags = m_tcomm->root_level();
        }
        else {
            success = 0;
            while (!success) {
                try {
                    m_tcomm->get_policy(level, policy);
                    EXPECT_EQ(m_tcomm->root_level(), (int)policy.flags);
                    success = 1;
                }
                catch (geopm::Exception ex) {
                    if (ex.err_value() != GEOPM_ERROR_POLICY_UNKNOWN) {
                        throw ex;
                    }
                }
            }
        }
        if (level) {
            send_policy.resize(m_tcomm->level_size(level - 1));
            fill(send_policy.begin(), send_policy.end(), policy);
            m_tcomm->send_policy(level - 1, send_policy);
        }
    }
}

TEST_F(MPITreeCommunicatorTest, send_sample_up)
{
    int success;
    std::vector <struct geopm_sample_message_s> sample;
    struct geopm_sample_message_s send_sample = {0};

    int num_level = m_tcomm->num_level();
    if (m_tcomm->root_level() == num_level - 1) {
        num_level--;
    }
    for (int level = 0; level < num_level; ++level) {
        send_sample.region_id = 1;
        send_sample.signal[0] = m_tcomm->level_rank(level) * (level + 1);
        m_tcomm->send_sample(level, send_sample);
        if (level && m_tcomm->level_rank(level) == 0) {
            sample.resize(m_tcomm->level_size(level - 1));
            success = 0;
            while (!success) {
                try {
                    m_tcomm->get_sample(level, sample);
                    for (int rank = 0; rank < m_tcomm->level_size(level); ++rank) {
                        EXPECT_EQ((uint64_t)rank * level, sample[rank].signal[0]);
                    }
                    success = 1;
                }
                catch (geopm::Exception ex) {
                    if (ex.err_value() != GEOPM_ERROR_SAMPLE_INCOMPLETE) {
                        throw ex;
                    }
                }
            }
        }
    }
}

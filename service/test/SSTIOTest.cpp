/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gmock/gmock-spec-builders.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MockSSTIoctl.hpp"
#include "SSTIO.hpp"
#include "SSTIOImp.hpp"

using geopm::SSTIOImp;
using geopm::SSTIoctl;
using geopm::sst_mbox_interface_batch_s;
using geopm::sst_mmio_interface_batch_s;
using testing::DoAll;
using testing::Field;
using testing::InSequence;
using testing::Return;
using testing::SetArgPointee;
using testing::_;

class SSTIOTest : public :: testing :: Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        std::shared_ptr<MockSSTIoctl> m_ioctl;
        int m_num_cpu = 4;
};

static const geopm::sst_version_s DEFAULT_VERSION {
    /* interface_version */ 1,
    /* driver_version */ 1,
    /* batch_command_limit */ 2,
    /* is_mbox_supported */ 1,
    /* is_mmio_supported */ 1
};

void SSTIOTest::SetUp(void)
{
    m_ioctl = std::make_shared<MockSSTIoctl>();
    ON_CALL(*m_ioctl, version(_))
    .WillByDefault(DoAll(SetArgPointee<0>(DEFAULT_VERSION), Return(0)));
}

void SSTIOTest::TearDown(void)
{

}

TEST_F(SSTIOTest, mbox_batch_reads)
{
    static const uint32_t max_cpus(32);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    {
        // Empty batch. Shouldn't use the ioctl.
        EXPECT_CALL(*m_ioctl, mbox(_)).Times(0);
        sstio.read_batch();
    }
    {
        // Perform a single read in a batch
        sstio.add_mbox_read(0, 0, 0, 0);
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1))).Times(1);
        sstio.read_batch();
    }
    {
        // Add a second read to the batch. Should still only call the ioctl once,
        // but now with two batched entries.
        sstio.add_mbox_read(1, 1, 1, 1);
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 2))).Times(1);
        sstio.read_batch();
    }
    {
        // Add a third read to the batch. Should now call the ioctl twice, since
        // the mock claims to support up to 2 batched commands in our SetUp.
        sstio.add_mbox_read(2, 2, 2, 2);
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 2))).Times(1);
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1))).Times(1);
        sstio.read_batch();
    }
}

TEST_F(SSTIOTest, mmio_batch_reads)
{
    static const uint32_t max_cpus(32);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    {
        // Empty batch. Shouldn't use the ioctl.
        EXPECT_CALL(*m_ioctl, mmio(_)).Times(0);
        sstio.read_batch();
    }
    {
        // Perform a single read in a batch
        sstio.add_mmio_read(0, 0);
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1))).Times(1);
        sstio.read_batch();
    }
    {
        // Add a second read to the batch. Should still only call the ioctl once,
        // but now with two batched entries.
        sstio.add_mmio_read(1, 1);
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 2))).Times(1);
        sstio.read_batch();
    }
    {
        // Add a third read to the batch. Should now call the ioctl twice, since
        // the mock claims to support up to 2 batched commands in our SetUp.
        sstio.add_mmio_read(2, 2);
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 2))).Times(1);
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1))).Times(1);
        sstio.read_batch();
    }
}

TEST_F(SSTIOTest, mbox_batch_writes)
{
    static const uint32_t max_cpus(32);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    {
        // Empty batch. Shouldn't use the ioctl.
        EXPECT_CALL(*m_ioctl, mbox(_)).Times(0);
        sstio.write_batch();
    }
    {
        sstio.add_mbox_write(0, 0, 0, 0, 0, 0, 0);
        // Expect a read, and a write after modify
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1))).Times(2);
        sstio.write_batch();
    }
    {
        sstio.add_mbox_write(1, 1, 1, 1, 1, 1, 1);
        // Expect both reads, and both writes after modify
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 2))).Times(2);
        sstio.write_batch();
    }
    {
        sstio.add_mbox_write(2, 2, 2, 2, 2, 2, 2);
        // Expect all three reads, and their writes after modify
        InSequence s;
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 2))).Times(1);
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1))).Times(1);

        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 2))).Times(1);
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1))).Times(1);
        sstio.write_batch();
    }
}

TEST_F(SSTIOTest, mmio_batch_writes)
{
    static const uint32_t max_cpus(32);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    {
        // Empty batch. Shouldn't use the ioctl.
        EXPECT_CALL(*m_ioctl, mmio(_)).Times(0);
        sstio.write_batch();
    }
    {
        sstio.add_mmio_write(0, 0, 0, 0);
        // Expect a read, and a write after modify
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1))).Times(2);
        sstio.write_batch();
    }
    {
        sstio.add_mmio_write(1, 1, 1, 1);
        // Expect both reads, and both writes after modify
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 2))).Times(2);
        sstio.write_batch();
    }
    {
        sstio.add_mmio_write(2, 2, 2, 2);
        // Expect all three reads, and their writes after modify
        InSequence s;
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 2))).Times(1);
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1))).Times(1);

        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 2))).Times(1);
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1))).Times(1);
        sstio.write_batch();
    }
}

TEST_F(SSTIOTest, sample_batched_reads)
{
    static const uint32_t max_cpus(32);
    static const uint32_t expected_mbox_read_value(123);
    static const uint32_t expected_mmio_read_value(456);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    // Add a mailbox read, then verify that we can sample it.
    auto mbox_read_idx = sstio.add_mbox_read(0, 0, 0, 0);
    {
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1)))
        .WillOnce([](sst_mbox_interface_batch_s *mbox_batch) {
            mbox_batch->interfaces[0].read_value = expected_mbox_read_value;
            return 0;
        });

        sstio.read_batch();
        EXPECT_EQ(expected_mbox_read_value, sstio.sample(mbox_read_idx));
    }
    // Add a mmio read, then verify that we can sample it.
    auto mmio_read_idx = sstio.add_mmio_read(0, 0);
    {
        // Try a new value for the mbox read just to make sure it gets updated
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1)))
        .WillOnce([](sst_mbox_interface_batch_s *mbox_batch) {
            mbox_batch->interfaces[0].read_value = expected_mbox_read_value + 1;
            return 0;
        });

        // Set the value for the new mmio read
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1)))
        .WillOnce([](sst_mmio_interface_batch_s *mmio_batch) {
            mmio_batch->interfaces[0].value = expected_mmio_read_value;
            return 0;
        });

        sstio.read_batch();
        EXPECT_EQ(expected_mbox_read_value + 1, sstio.sample(mbox_read_idx));
        EXPECT_EQ(expected_mmio_read_value, sstio.sample(mmio_read_idx));
    }
}

TEST_F(SSTIOTest, adjust_batched_writes)
{
    static const uint32_t max_cpus(32);
    static const uint32_t expected_mbox_write_value(0x12);
    static const uint32_t expected_mmio_write_value(0x34);
    static const uint32_t read_mask(0xffffffff);
    static const uint32_t write_mask(0xffff);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    // Add a mailbox write, then verify that we can adjust it.
    auto mbox_write_idx = sstio.add_mbox_write(0, 0, 0, 0, 0, 0, read_mask);
    {
        sstio.adjust(mbox_write_idx, expected_mbox_write_value, write_mask);
        uint32_t written_value = 0;
        // Read existing value -- pretend something already exists there so we
        // can make sure the write mask is used.
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1)))
        .WillOnce([](sst_mbox_interface_batch_s *mbox_batch) {
            mbox_batch->interfaces[0].read_value = 0xf0f0f0f0;
            return 0;
        })
        .WillOnce([&written_value](sst_mbox_interface_batch_s *mbox_batch) {
            // Write modified value
            written_value = mbox_batch->interfaces[0].write_value;
            return 0;
        });
        sstio.write_batch();
        EXPECT_EQ(0xf0f00000 | expected_mbox_write_value, written_value);
    }
    // Add a mmio write, then verify that we can adjust it.
    auto mmio_write_idx = sstio.add_mmio_write(0, 0, 0, read_mask);
    {
        // Try a new value for the mbox write just to make sure it gets updated
        sstio.adjust(mbox_write_idx, expected_mbox_write_value + 1, write_mask);
        sstio.adjust(mmio_write_idx, expected_mmio_write_value, write_mask);

        uint32_t written_mbox_value = 0;
        EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1)))
        .WillOnce([](sst_mbox_interface_batch_s *mbox_batch) {
            mbox_batch->interfaces[0].read_value = 0xf0f00000 | expected_mbox_write_value;
            return 0;
        })
        .WillOnce([&written_mbox_value](sst_mbox_interface_batch_s *mbox_batch) {
            written_mbox_value = mbox_batch->interfaces[0].write_value;
            return 0;
        });
        uint32_t written_mmio_value = 0;
        EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1)))
        .WillOnce([](sst_mmio_interface_batch_s *mmio_batch) {
            mmio_batch->interfaces[0].value = 0xf1f1f1f1;
            return 0;
        })
        .WillOnce([&written_mmio_value](sst_mmio_interface_batch_s *mmio_batch) {
            written_mmio_value = mmio_batch->interfaces[0].value;
            return 0;
        });

        sstio.write_batch();
        EXPECT_EQ(0xf0f00000 | (expected_mbox_write_value + 1), written_mbox_value);
        EXPECT_EQ(0xf1f10000 | expected_mmio_write_value, written_mmio_value);
    }
}

TEST_F(SSTIOTest, read_mbox_once)
{
    static const uint32_t max_cpus(32);
    static const uint32_t expected_mbox_read_value(123);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1)))
    .WillOnce([](sst_mbox_interface_batch_s *mbox_batch) {
        mbox_batch->interfaces[0].read_value = expected_mbox_read_value;
        return 0;
    });

    EXPECT_EQ(expected_mbox_read_value, sstio.read_mbox_once(0, 0, 0, 0));
}

TEST_F(SSTIOTest, read_mmio_once)
{
    static const uint32_t max_cpus(32);
    static const uint32_t expected_mmio_read_value(456);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    // Set the value for the new mmio read
    EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1)))
    .WillOnce([](sst_mmio_interface_batch_s *mmio_batch) {
        mmio_batch->interfaces[0].value = expected_mmio_read_value;
        return 0;
    });

    EXPECT_EQ(expected_mmio_read_value, sstio.read_mmio_once(0, 0));
}

TEST_F(SSTIOTest, write_mbox_once)
{
    static const uint32_t max_cpus(32);
    static const uint32_t expected_mbox_write_value(0x12);
    static const uint32_t read_mask(0xffffffff);
    static const uint32_t write_mask(0xffff);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    // Add a mailbox write, then verify that we can adjust it.
    uint32_t written_value = 0;
    // Read existing value -- pretend something already exists there so we
    // can make sure the write mask is used.
    EXPECT_CALL(*m_ioctl, mbox(Field(&sst_mbox_interface_batch_s::num_entries, 1)))
    .WillOnce([](sst_mbox_interface_batch_s *mbox_batch) {
        mbox_batch->interfaces[0].read_value = 0xf0f0f0f0;
        return 0;
    })
    .WillOnce([&written_value](sst_mbox_interface_batch_s *mbox_batch) {
        // Write modified value
        written_value = mbox_batch->interfaces[0].write_value;
        return 0;
    });

    sstio.write_mbox_once(0, 0, 0, 0, 0, 0, read_mask, expected_mbox_write_value, write_mask);
    EXPECT_EQ(0xf0f00000 | expected_mbox_write_value, written_value);
}

TEST_F(SSTIOTest, write_mmio_once)
{
    static const uint32_t max_cpus(32);
    static const uint32_t expected_mmio_write_value(0x34);
    static const uint32_t read_mask(0xffffffff);
    static const uint32_t write_mask(0xffff);
    SSTIOImp sstio(max_cpus, std::static_pointer_cast<SSTIoctl>(m_ioctl));

    uint32_t written_mmio_value = 0;
    EXPECT_CALL(*m_ioctl, mmio(Field(&sst_mmio_interface_batch_s::num_entries, 1)))
    .WillOnce([](sst_mmio_interface_batch_s *mmio_batch) {
        mmio_batch->interfaces[0].value = 0xf1f1f1f1;
        return 0;
    })
    .WillOnce([&written_mmio_value](sst_mmio_interface_batch_s *mmio_batch) {
        written_mmio_value = mmio_batch->interfaces[0].value;
        return 0;
    });

    sstio.write_mmio_once(0, 0, 0, read_mask, expected_mmio_write_value, write_mask);
    EXPECT_EQ(0xf1f10000 | expected_mmio_write_value, written_mmio_value);
}

TEST_F(SSTIOTest, get_punit_from_cpu)
{
    const std::map<uint32_t, uint32_t> expected_cpu_punit_map = {
        {0, 10},
        {1, 11},
        {2, 12}
    };
    InSequence s;
    EXPECT_CALL(
        *m_ioctl,
        get_cpu_id(Field(&geopm::sst_cpu_map_interface_batch_s::num_entries, 2)))
    .WillOnce(
    [&expected_cpu_punit_map](geopm::sst_cpu_map_interface_batch_s *cpu_map_batch) {
        for (size_t i = 0; i < cpu_map_batch->num_entries; ++i) {
            cpu_map_batch->interfaces[i].punit_cpu =
                // Left-shift 1 bit. Simulate everything being hyperthread 0.
                // The test's outcome should not care which hyperthread this maps to.
                expected_cpu_punit_map.at(cpu_map_batch->interfaces[i].cpu_index) << 1 | 0;
        }
        return 0;
    });
    // The 3 CPUs should split over 2 batches since we specified a batch size of
    // 2 in our setup.
    EXPECT_CALL(
        *m_ioctl,
        get_cpu_id(Field(&geopm::sst_cpu_map_interface_batch_s::num_entries, 1)))
    .WillOnce(
    [&expected_cpu_punit_map](geopm::sst_cpu_map_interface_batch_s *cpu_map_batch) {
        for (size_t i = 0; i < cpu_map_batch->num_entries; ++i) {
            cpu_map_batch->interfaces[i].punit_cpu =
                // Left-shift 1 bit. Simulate everything being hyperthread 1.
                // The test's outcome should not care which hyperthread this maps to.
                expected_cpu_punit_map.at(cpu_map_batch->interfaces[i].cpu_index) << 1 | 1;
        }
        return 0;
    });
    SSTIOImp sstio(expected_cpu_punit_map.size(), std::static_pointer_cast<SSTIoctl>(m_ioctl));

    for (const auto &cpu_punit_pair : expected_cpu_punit_map) {
        EXPECT_EQ(cpu_punit_pair.second, sstio.get_punit_from_cpu(cpu_punit_pair.first));
    }
}

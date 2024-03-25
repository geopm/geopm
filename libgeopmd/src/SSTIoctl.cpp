/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SSTIoctl.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>

#include <algorithm>
#include <string>
#include <utility>

#include "geopm/Exception.hpp"
#include "SSTIoctlImp.hpp"

#define GEOPM_IOC_SST_VERSION _IOR(0xfe, 0, struct geopm::sst_version_s *)
#define GEOPM_IOC_SST_GET_CPU_ID _IOWR(0xfe, 1, struct geopm::sst_mbox_interface_batch_s *)
#define GEOPM_IOC_SST_MMIO _IOW(0xfe, 2, struct geopm::sst_mmio_interface_batch_s *)
#define GEOPM_IOC_SST_MBOX _IOWR(0xfe, 3, struct geopm::sst_mbox_interface_batch_s *)

namespace geopm
{
    std::shared_ptr<SSTIoctl> SSTIoctl::make_shared(const std::string &path)
    {
        return std::make_shared<SSTIoctlImp>(path);
    }

    SSTIoctlImp::SSTIoctlImp(const std::string &path)
        : m_path(path)
        , m_fd(open(m_path.c_str(), O_RDWR))
    {
    }

    SSTIoctlImp::~SSTIoctlImp()
    {
        close(m_fd);
    }

    int SSTIoctlImp::version(sst_version_s *version)
    {
        return ioctl(m_fd, GEOPM_IOC_SST_VERSION, version);
    }

    int SSTIoctlImp::get_cpu_id(sst_cpu_map_interface_batch_s *cpu_batch)
    {
        return ioctl(m_fd, GEOPM_IOC_SST_GET_CPU_ID, cpu_batch);
    }

    int SSTIoctlImp::mbox(sst_mbox_interface_batch_s *mbox_batch)
    {
        return ioctl(m_fd, GEOPM_IOC_SST_MBOX, mbox_batch);
    }

    int SSTIoctlImp::mmio(sst_mmio_interface_batch_s *mmio_batch)
    {
        return ioctl(m_fd, GEOPM_IOC_SST_MMIO, mmio_batch);
    }
}

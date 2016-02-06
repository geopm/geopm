/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include "geopm_plugin.h"
#include "Exception.hpp"

#include "TestPlugin.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory)
{
    int err = 0;
    Decider *decider = NULL;
    Platform *platform = NULL;
    PlatformImp *platform_imp = NULL;

    try {
        switch (plugin_type) {
            case GEOPM_PLUGIN_TYPE_DECIDER:
                decider = new DumbDecider;
                geopm_factory_register(factory, decider);
                break;
            case GEOPM_PLUGIN_TYPE_PLATFORM:
                platform = new DumbPlatform;
                geopm_factory_register(factory, platform);
                break;
            case GEOPM_PLUGIN_TYPE_PLATFORM_IMP:
                platform_imp = new DumbPlatformImp;
                geopm_factory_register(factory, platform_imp);
                break;
        }
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}

DumbDecider::DumbDecider()
    : m_name("dumb")
{

}

DumbDecider::~DumbDecider()
{

}

bool DumbDecider::decider_supported(const std::string &description)
{
    return (description == m_name);
}

const std::string& DumbDecider::name(void) const
{
    return m_name;
}

bool DumbDecider::update_policy(Region &curr_region, Policy &curr_policy)
{
    return false;
}

DumbPlatform::DumbPlatform()
    : m_name("dumb")
{

}

DumbPlatform::~DumbPlatform()
{

}

size_t DumbPlatform::capacity(void)
{
    return 0;
}

void DumbPlatform::sample(std::vector<struct geopm_msr_message_s> &msr_msg)
{

}

bool DumbPlatform::model_supported(int platform_id, const std::string &description) const
{
    return false;
}

void DumbPlatform::enforce_policy(uint64_t region_id, Policy &policy) const
{

}

DumbPlatformImp::DumbPlatformImp()
    : m_name("dumb")
{

}

DumbPlatformImp::~DumbPlatformImp()
{

}

bool DumbPlatformImp::model_supported(int platform_id)
{
    return false;
}

std::string DumbPlatformImp::platform_name(void)
{
    return m_name;
}

void DumbPlatformImp::msr_reset(void)
{

}

int DumbPlatformImp::power_control_domain(void) const
{
    return geopm::GEOPM_DOMAIN_PACKAGE;
}

int DumbPlatformImp::frequency_control_domain(void) const
{
    return geopm::GEOPM_DOMAIN_CPU;
}

int DumbPlatformImp::control_domain(void) const
{
    return 0;
}

void DumbPlatformImp::msr_initialize(void)
{

}

double DumbPlatformImp::read_signal(int device_type, int device_index, int signal_type)
{
    return 1.0;
}

void DumbPlatformImp::write_control(int device_type, int device_index, int signal_type, double value)
{

}


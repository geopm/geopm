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

#ifndef TESTPLUGIN_HPP_INCLUDE
#define TESTPLUGIN_HPP_INCLUDE

#include "geopm_plugin.h"
#include "Decider.hpp"
#include "Platform.hpp"
#include "PlatformImp.hpp"

using namespace geopm;

class DumbDecider : public Decider
{
    public:
        DumbDecider();
        virtual ~DumbDecider();
        virtual Decider *clone() const;
        virtual bool update_policy(Region &curr_region, Policy &curr_policy);
        virtual bool decider_supported(const std::string &descripton);
        virtual const std::string& name(void) const;
    protected:
        const std::string m_name;
};

class DumbPlatform : public Platform
{
    public:
        DumbPlatform();
        virtual ~DumbPlatform();
        virtual size_t capacity(void);
        virtual void sample(std::vector<struct geopm_msr_message_s> &msr_msg);
        virtual bool model_supported(int platfrom_id, const std::string &description) const;
        virtual void enforce_policy(uint64_t region_id, Policy &policy) const;
    protected:
        const std::string m_name;
};

class DumbPlatformImp : public PlatformImp
{
    public:
        DumbPlatformImp();
        virtual ~DumbPlatformImp();
        virtual bool model_supported(int platform_id);
        virtual std::string platform_name(void);
        virtual void msr_reset(void);
        virtual int power_control_domain(void) const;
        virtual int frequency_control_domain(void) const;
        virtual int control_domain(void) const;
        virtual double read_signal(int device_type, int device_index, int signal_type);
        virtual void write_control(int device_type, int device_index, int signal_type, double value);
    protected:
        virtual void msr_initialize(void);
        const std::string m_name;
};

#endif

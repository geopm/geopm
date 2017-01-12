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


#include <unistd.h>
#include <string>
#include <string.h>
#include <fstream>

#include "Exception.hpp"
#include "geopm_time.h"
#include "imbalancer.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

class Imbalancer
{
    public:
        Imbalancer();
        Imbalancer(const std::string config_path);
        virtual ~Imbalancer();
        void frac(double delay_frac);
        void enter(void);
        void exit(void);
    private:
        double m_delay_frac;
        struct geopm_time_s m_enter_time;
};


Imbalancer::Imbalancer()
    : m_delay_frac(0.0)
    , m_enter_time({{0,0}})
{

}

Imbalancer::Imbalancer(const std::string config_path)
    : Imbalancer()
{
    if (config_path.length()) {
        char hostname[HOST_NAME_MAX + 1];
        hostname[HOST_NAME_MAX] = '\0';
        if (gethostname(hostname, HOST_NAME_MAX)) {
            throw geopm::Exception("gethostname():", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::ifstream config_stream(config_path, std::ifstream::in);
        std::string this_host;
        double this_frac;
        while(config_stream.good()) {
            config_stream >> this_host >> this_frac;
            if (strncmp(hostname, this_host.c_str(), HOST_NAME_MAX) == 0) {
                frac(this_frac);
            }
        }
        config_stream.close();
    }
}

Imbalancer::~Imbalancer()
{

}

void Imbalancer::frac(double delay_frac)
{
    if (delay_frac >= 0.0) {
        m_delay_frac = delay_frac;
    }
    else {
        throw geopm::Exception("Imbalancer::frac(): delay_fraction is negative",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
}

void Imbalancer::enter(void)
{
    if (m_delay_frac != 0.0) {
        geopm_time(&m_enter_time);
    }
}

void Imbalancer::exit(void)
{
    if (m_delay_frac != 0.0) {
        struct geopm_time_s exit_time;
        geopm_time(&exit_time);
        double delay = geopm_time_diff(&m_enter_time, &exit_time) * m_delay_frac;
        struct geopm_time_s loop_time;
        do {
            geopm_time(&loop_time);
        }
        while (geopm_time_diff(&exit_time, &loop_time) < delay);
    }
}

static Imbalancer &imbalancer(void)
{
    static char *config_path = getenv("IMBALANCER_CONFIG");
    static Imbalancer instance(config_path ? std::string(config_path) : "");
    return instance;
}

int imbalancer_frac(double delay_frac)
{
    int err = 0;
    try {
        imbalancer().frac(delay_frac);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}

int imbalancer_enter(void)
{
    int err = 0;
    try {
        imbalancer().enter();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}

int imbalancer_exit(void)
{
    int err = 0;
    try {
        imbalancer().exit();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}

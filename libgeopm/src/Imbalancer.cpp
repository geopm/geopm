/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <unistd.h>
#include <string>
#include <string.h>
#include <fstream>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_time.h"
#include "geopm_imbalancer.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

class Imbalancer
{
    public:
        Imbalancer();
        Imbalancer(const std::string &config_path);
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

Imbalancer::Imbalancer(const std::string &config_path)
    : Imbalancer()
{
    if (config_path.length()) {
        std::ifstream config_stream(config_path, std::ifstream::in);
        std::string this_host;
        double this_frac;
        while(config_stream.good()) {
            config_stream >> this_host >> this_frac;
            if (geopm::hostname() == this_host) {
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

int geopm_imbalancer_frac(double delay_frac)
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

int geopm_imbalancer_enter(void)
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

int geopm_imbalancer_exit(void)
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

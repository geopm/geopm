/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm_pio_rt.h"

#include <vector>
#include <cstring>

#include "geopm/PlatformIO.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "PlatformIOProf.hpp"
#include "ApplicationIO.hpp"
#include "ApplicationSampler.hpp"
#include "geopm/Exception.hpp"
#include "geopm_time.h"

using geopm::PlatformIOProf;
using geopm::ApplicationIOImp;
using geopm::ApplicationSampler;

extern "C" {

    int geopm_pio_rt_num_signal_name(void)
    {
        int result = 0;
        try {
            result = PlatformIOProf::platform_io().signal_names().size();
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    static int geopm_pio_rt_name_set_idx(int name_idx, size_t result_max,
                                         const std::set<std::string> &name_set, char *result)
    {
        // Check user inputs
        if (name_idx < 0 ||
            (size_t)name_idx >= name_set.size() ||
            result_max == 0) {
            return GEOPM_ERROR_INVALID;
        }
        int err = 0;
        int name_count = 0;
        for (const auto &ns_it : name_set) {
            if (name_count == name_idx) {
                result[result_max - 1] = '\0';
                strncpy(result, ns_it.c_str(), result_max);
                if (result[result_max - 1] != '\0') {
                    err = GEOPM_ERROR_INVALID;
                    result[result_max - 1] = '\0';
                }
                break;
            }
            ++name_count;
        }
        return err;
    }

    int geopm_pio_rt_signal_name(int name_idx, size_t result_max, char *result)
    {
        int err = 0;
        if (result_max != 0) {
            result[0] = '\0';
        }
        try {
            std::set<std::string> name_set = PlatformIOProf::platform_io().signal_names();
            err = geopm_pio_rt_name_set_idx(name_idx, result_max, name_set, result);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_num_control_name(void)
    {
        int result = 0;
        try {
            result = PlatformIOProf::platform_io().control_names().size();
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_rt_control_name(int name_idx, size_t result_max, char *result)
    {
        int err = 0;
        if (result_max != 0) {
            result[0] = '\0';
        }
        try {
            std::set<std::string> name_set = PlatformIOProf::platform_io().control_names();
            err = geopm_pio_rt_name_set_idx(name_idx, result_max, name_set, result);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_signal_domain_type(const char *signal_name)
    {
        int result = 0;
        try {
            const std::string signal_name_string(signal_name);
            result = PlatformIOProf::platform_io().signal_domain_type(signal_name_string);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_rt_control_domain_type(const char *control_name)
    {
        int result = 0;
        try {
            result = PlatformIOProf::platform_io().control_domain_type(control_name);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_rt_read_signal(const char *signal_name, int domain_type,
                              int domain_idx, double *result)
    {
        int err = 0;
        try {
            *result = PlatformIOProf::platform_io().read_signal(signal_name, domain_type, domain_idx);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_write_control(const char *control_name, int domain_type,
                                   int domain_idx, double setting)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().write_control(control_name, domain_type, domain_idx, setting);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_push_signal(const char *signal_name, int domain_type, int domain_idx)
    {
        int result = 0;
        try {
            result = PlatformIOProf::platform_io().push_signal(signal_name, domain_type, domain_idx);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_rt_push_control(const char *control_name, int domain_type, int domain_idx)
    {
        int result = 0;
        try {
            result = PlatformIOProf::platform_io().push_control(control_name, domain_type, domain_idx);
        }
        catch (...) {
            result = geopm::exception_handler(std::current_exception());
            result = result < 0 ? result : GEOPM_ERROR_RUNTIME;
        }
        return result;
    }

    int geopm_pio_rt_sample(int signal_idx, double *result)
    {
        int err = 0;
        try {
            *result = PlatformIOProf::platform_io().sample(signal_idx);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_adjust(int control_idx, double setting)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().adjust(control_idx, setting);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_read_batch(void)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().read_batch();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_write_batch(void)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().write_batch();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_save_control(void)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().save_control();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_restore_control(void)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().restore_control();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_save_control_dir(const char *save_dir)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().save_control(save_dir);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_restore_control_dir(const char *save_dir)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().restore_control(save_dir);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_signal_description(const char *signal_name, size_t description_max,
                                     char *description)
    {
        int err = 0;
        try {
            std::string description_string = PlatformIOProf::platform_io().signal_description(signal_name);
            description[description_max - 1] = '\0';
            strncpy(description, description_string.c_str(), description_max);
            if (description[description_max - 1] != '\0') {
                description[description_max - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_control_description(const char *control_name, size_t description_max,
                                      char *description)
    {
        int err = 0;
        try {
            std::string description_string = PlatformIOProf::platform_io().control_description(control_name);
            description[description_max - 1] = '\0';
            strncpy(description, description_string.c_str(), description_max);
            if (description[description_max - 1] != '\0') {
                description[description_max - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_signal_info(const char *signal_name,
                              int *aggregation_type,
                              int *format_type,
                              int *behavior_type)
    {
        int err = 0;
        try {
            auto agg_func = PlatformIOProf::platform_io().agg_function(signal_name);
            *aggregation_type = geopm::Agg::function_to_type(std::move(agg_func));
            auto format_func = PlatformIOProf::platform_io().format_function(signal_name);
            *format_type = geopm::string_format_function_to_type(std::move(format_func));
            *behavior_type = PlatformIOProf::platform_io().signal_behavior(signal_name);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }


    int geopm_pio_rt_start_batch_server(int client_pid,
                                     int num_signal,
                                     const struct geopm_request_s *signal_config,
                                     int num_control,
                                     const struct geopm_request_s *control_config,
                                     int *server_pid,
                                     int key_size,
                                     char *server_key)
    {
        int err = 0;
        try {
            std::vector<geopm_request_s> signal_config_vec(num_signal);
            if (signal_config != nullptr) {
                std::copy(signal_config,
                          signal_config + num_signal,
                          signal_config_vec.begin());
            }
            std::vector<geopm_request_s> control_config_vec(num_control);
            if (control_config != nullptr) {
                std::copy(control_config,
                          control_config + num_control,
                          control_config_vec.begin());
            }
            std::string server_key_str;
            PlatformIOProf::platform_io().start_batch_server(client_pid,
                                                    signal_config_vec,
                                                    control_config_vec,
                                                    *server_pid,
                                                    server_key_str);
            strncpy(server_key, server_key_str.c_str(), key_size);
            if (server_key[key_size - 1] != '\0') {
                server_key[key_size - 1] = '\0';
                err = GEOPM_ERROR_INVALID;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_pio_rt_stop_batch_server(int server_pid)
    {
        int err = 0;
        try {
            PlatformIOProf::platform_io().stop_batch_server(server_pid);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    // @todo Add missing
    // void geopm_pio_rt_reset(void)

    int geopm_pio_rt_check_valid_value(double value)
    {
        return PlatformIOProf::platform_io().is_valid_value(value) ? 0 : GEOPM_ERROR_INVALID;
    }

    static int geopm_pio_rt_update_helper(bool is_once)
    {
        int err = 0;
        static auto app_io = std::make_unique<ApplicationIOImp>();
        static auto &app_sampler = geopm::ApplicationSampler::application_sampler();
        try {
            if (is_once) {
                app_sampler.connect(app_io->connect());
            }
            else {
                geopm_time_s curr_time;
                geopm_time(&curr_time);
                app_sampler.update(curr_time);
            }
        }
        catch (const geopm::Exception &ex) {
            err = 1;
        }
        return err;
    }

    int geopm_pio_rt_update(void)
    {
        static int err = geopm_pio_rt_update_helper(true);
        if (!err) {
            err = geopm_pio_rt_update_helper(false);
        }
        return err;
    }

}

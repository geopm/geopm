/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef GEOPM_PIO_H_INCLUDE
#define GEOPM_PIO_H_INCLUDE

#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

int geopm_pio_num_signal_name(void);

int geopm_pio_signal_name(int name_idx,
                          size_t result_max,
                          char *result);

int geopm_pio_num_control_name(void);

int geopm_pio_control_name(int name_index,
                           size_t result_max,
                           char *result);

int geopm_pio_signal_domain_type(const char *signal_name);

int geopm_pio_control_domain_type(const char *control_name);

int geopm_pio_read_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx,
                          double *result);

int geopm_pio_write_control(const char *control_name,
                            int domain_type,
                            int domain_idx,
                            double setting);

int geopm_pio_push_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx);

int geopm_pio_push_control(const char *control_name,
                           int domain_type,
                           int domain_idx);

int geopm_pio_sample(int signal_idx,
                     double *result);

int geopm_pio_adjust(int control_idx,
                     double setting);

int geopm_pio_read_batch(void);

int geopm_pio_write_batch(void);

int geopm_pio_save_control(void);

int geopm_pio_save_control_dir(const char *save_dir);

int geopm_pio_restore_control(void);

int geopm_pio_restore_control_dir(const char *save_dir);

int geopm_pio_signal_description(const char *signal_name,
                                 size_t description_max,
                                 char *description);

int geopm_pio_control_description(const char *control_name,
                                  size_t description_max,
                                  char *description);


/// @brief C interface to get enums associated with a signal name.
///
/// This interface supports DBus PlatformGetSignalInfo method.  This C
/// interface is implemented using several PlatformIO methods unlike
/// the other wrappers in this header.
///
/// @param [in] signal_name Name of signal to query.
///
/// @param [out] aggregation_type One of the Agg::m_type_e enum values
///        describing the way the signal is aggregated.
///
/// @param [out] format_type One of the geopm::string_format_e
///        enums defined in Helper.hpp that defines how to format
///        the signal as a string.
///
/// @param [out] behavior_type One of the
///        IOGroup::m_signal_behavior_e enum values that decribes
///        the signals behavior over time.
///
/// @return Zero on success, error value on failure.
int geopm_pio_signal_info(const char *signal_name,
                          int *aggregation_type,
                          int *format_type,
                          int *behavior_type);


struct geopm_request_s;

// Either call through to DBusServer::start_batch or noop based on
// GEOPM_SERVICE_BUILD define
int geopm_pio_start_batch_server(int client_pid,
                                 int num_signal,
                                 const struct geopm_request_s *signal_config,
                                 int num_control,
                                 const struct geopm_request_s *control_config,
                                 int *server_pid,
                                 int key_size,
                                 char *server_key);

// Either call through to DBusServer::stop_batch or noop based on
// GEOPM_SERVICE_BUILD define
int geopm_pio_stop_batch_server(int server_pid);

int geopm_pio_format_signal(double signal,
                            int format_type,
                            size_t result_max,
                            char *result);

#ifdef __cplusplus
}
#endif
#endif

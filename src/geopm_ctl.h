/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef GEOPM_CTL_H_INCLUDE
#define GEOPM_CTL_H_INCLUDE

#include <mpi.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque structure which is a handle for a geopm::Controller object. */
struct geopm_ctl_c;

/************************/
/* OBJECT INSTANTIATION */
/************************/

/// @brief creates a geopm_ctl_c object, ctl which is an opaque structure
///        that holds the state used to execute the control algorithm with
///        one of the other functions described in this header file.
///
/// @details The control algorithm relies on feedback about the application profile.
///
/// @param comm The user provides an MPI communicator, which must
///             have at least one process running on every compute node under control.
///
/// @param ctl The out parameter pointer to the created geopm_ctl_c object
int geopm_ctl_create(MPI_Comm comm,
                     struct geopm_ctl_c **ctl);

/// @brief destroys all resources associated with the ctl structure which
///        allocated by a previous call to geopm_ctl_create().
int geopm_ctl_destroy(struct geopm_ctl_c *ctl);

/********************/
/* POWER MANAGEMENT */
/********************/

/// @brief steps the control algorithm continuously until the application
///        signals shutdown.
int geopm_ctl_run(struct geopm_ctl_c *ctl);

/// @brief creates a POSIX thread running the control algorithm continuously
///        until the application signals shutdown.
///
/// @details With this method of launch the supporting MPI implementation
///          must be enabled for MPI_THREAD_MULTIPLE using MPI_Init_thread()
///
/// @param ctl is an opaque structure  that holds the state used to execute the control algorithm
///
/// @param attr is used to create the POSIX thread
///
/// @param thread is used to create the POSIX thread
int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                      const pthread_attr_t *attr,
                      pthread_t *thread);
#ifdef __cplusplus
}
#endif
#endif

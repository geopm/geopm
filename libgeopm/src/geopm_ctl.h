/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_BATCH_REQUEST_H_INCLUDE
#define GEOPM_BATCH_REQUEST_H_INCLUDE

#ifdef __cplusplus
extern "C"
{
#endif

    struct geopm_batch_request_s;

    int geopm_batch_request_create(const char *file_path,
                                   struct geopm_batch_request_s **request);

    int geopm_batch_request_destroy(struct geopm_batch_request_s *request);

    int geopm_batch_request_num_sample(const struct geopm_batch_request_s *request, int *num_sample);

    int geopm_batch_request_read(struct geopm_batch_request_s *request,
                                 size_t num_sample,
                                 double *sample);

#ifdef __cplusplus
}
#endif
#endif

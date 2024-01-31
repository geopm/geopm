/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TUTORIAL_REGION_H_INCLUDE
#define TUTORIAL_REGION_H_INCLUDE

int tutorial_sleep(double big_o, int do_report);
int tutorial_dgemm(double big_o, int do_report);
int tutorial_stream(double big_o, int do_report);
int tutorial_all2all(double big_o, int do_report);
int tutorial_dgemm_static(double big_o, int do_report);
int tutorial_stream_profiled(double big_o, int do_report);

#endif

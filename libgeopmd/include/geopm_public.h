/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GEOPM_PUBLIC_H_INCLUDE
#define GEOPM_PUBLIC_H_INCLUDE
/*
 * Indicates that a symbol **is** part of a GEOPM library's public interface
 */
#define GEOPM_PUBLIC __attribute__((visibility("default")))

/*
 * Indicates that a symbol **is not** part of a GEOPM library's public interface
 */
#define GEOPM_PRIVATE __attribute__((visibility("hidden")))
#endif

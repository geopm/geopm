#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

CFLAGS_EXTRA = -std=c99 -D_POSIX_C_SOURCE=200809L -O3 -lrt -qmkl=parallel

rapl_pkg_limit_test: rapl_pkg_limit_test.c
	icx $(CFLAGS_EXTRA) $(CFLAGS) $(LDFLAGS) $(LIBS) -fiopenmp $^ -o $@

check: rapl_pkg_limit_test
	./rapl_pkg_limit_test 150

clean:
	rm -f rapl_pkg_limit_test

.PHONY: clean test

#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


test_sleep_region: test_sleep_region.c
	gcc ${GEOPM_CFLAGS} ${GEOPM_CPPFLAGS} ${GEOPM_LDFLAGS} -lgeopm -lgeopmd -g -O0 $^ -o $@

clean:
	rm -f test_sleep_region

.PHONY: clean

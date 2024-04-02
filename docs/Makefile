#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

prefix ?= /usr
datarootdir ?= /share

all: man

man:
	sphinx-build -M man source build -W

html:
	sphinx-build -M html source build -W

# Nobody asked for this build target, so don't make it a build-blocker unless
# we decide to keep it. If we choose to keep it, then be sure to ratchet in all
# the errors, add -W back to the options, and add this as a dependency to the
# `docs` target
geopmlint:
	sphinx-build -M geopmlint source build

clean:
	rm -rf docs/build

PHONY_TARGETS += all man html geopmlint clean install

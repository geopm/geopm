#!/bin/bash
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Make distribution archive
make dist

# Build the Debian source package
dpkg-buildpackage --build=source

# Build the Debian binary package
cargo deb

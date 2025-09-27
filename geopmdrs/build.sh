#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x

export RUSTFLAGS="-C control-flow-guard"

# Create VERSION file
if [ ! -e VERSION ]; then
    python3 -c "from setuptools_scm import get_version; print(get_version('..'))" | sed -e 's|.dev|-dev|' > VERSION
    if [ $? -ne 0 ]; then
        echo "WARNING:  VERSION file does not exist and setuptools_scm failed, setting version to 0.0.0" 1>&2
        echo "0.0.0" > VERSION
    fi
fi
sed -e "s|@VERSION@|$(cat VERSION)|" Cargo.toml.in > Cargo.toml
cargo vendor
cargo build
cargo build -r
cargo install cargo-deb
cargo deb
echo "Warning, unable to build a RPM package, if required, add target/release/geopmd-proxy to the PATH of geopmd"

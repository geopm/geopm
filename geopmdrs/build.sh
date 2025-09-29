#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -x

export RUSTFLAGS="-C control-flow-guard"

# Create VERSION file
if [ ! -e VERSION ]; then
    python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > VERSION_RAW
    cat VERSION_RAW | sed -e 's|.dev|-dev|' > VERSION
    if [ $? -ne 0 ]; then
        echo "WARNING:  VERSION file does not exist and setuptools_scm failed, setting version to 0.0.0" 1>&2
        echo "0.0.0" > VERSION
    fi
fi
sed -e "s|@VERSION@|$(cat VERSION)|" Cargo.toml.in > Cargo.toml
cargo vendor
cargo build
cargo build -r
# Generate geopmd-proxy.spec from template for RPM packaging (no build invoked here)
if [ -f geopmd-proxy.spec.in ]; then
    sed -e "s|@VERSION@|$(cat VERSION_RAW)|" geopmd-proxy.spec.in > geopmd-proxy.spec
fi
# Create source tarball from root git tree for RPM (geopm-<version>.tar.gz)
VERSION_STR=$(cat VERSION_RAW)
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo "..")
pushd "${REPO_ROOT}" >/dev/null
ARCHIVE_NAME="geopm-${VERSION_STR}"
git archive --format=tar --prefix="${ARCHIVE_NAME}/" HEAD | gzip -9 > "${ARCHIVE_NAME}.tar.gz"
popd >/dev/null
ln -sf "${REPO_ROOT}/${ARCHIVE_NAME}.tar.gz" "${ARCHIVE_NAME}.tar.gz"
echo "Created git archive source tarball: ${ARCHIVE_NAME}.tar.gz"

# Attempt to build a native package (RPM or DEB) depending on host
PKG_BUILT=0
VERSION_FULL=$(cat VERSION_RAW)

if command -v rpmbuild >/dev/null 2>&1 && [ -f geopmd-proxy.spec ]; then
    echo "Detected rpmbuild; attempting RPM build" >&2
    # Prepare rpmbuild tree under a temp directory
    RPMTMP=$(mktemp -d)
    mkdir -p "${RPMTMP}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
    cp -a "${REPO_ROOT}/${ARCHIVE_NAME}.tar.gz" "${RPMTMP}/SOURCES/"
    cp geopmd-proxy.spec "${RPMTMP}/SPECS/"
    ( cd "${RPMTMP}/SPECS" && rpmbuild --define "_topdir ${RPMTMP}" -ba geopmd-proxy.spec ) && PKG_BUILT=1 || echo "RPM build failed" >&2
    if [ ${PKG_BUILT} -eq 1 ]; then
        find "${RPMTMP}/RPMS" -type f -name "*.rpm" -exec cp -v {} . \;
        find "${RPMTMP}/SRPMS" -type f -name "*.src.rpm" -exec cp -v {} . \;
        echo "RPM packages created in $(pwd)" >&2
    fi
fi

if [ ${PKG_BUILT} -eq 0 ] && command -v dpkg-buildpackage >/dev/null 2>&1; then
    echo "Detected dpkg-buildpackage; attempting Debian build" >&2
    # Generate debian/changelog if template exists
    if [ -f debian/changelog.in ]; then
        DATE_STR=$(date -R)
        sed -e "s|@VERSION@|${VERSION_FULL}|" -e "s|@DATE@|${DATE_STR}|" debian/changelog.in > debian/changelog
    fi
    # Build the deb (binary only, unsigned)
    dpkg-buildpackage -us -uc -b || echo "Debian package build failed" >&2
    if ls ../geopmd-proxy_* 1>/dev/null 2>&1; then
        PKG_BUILT=1
        echo "Debian packages created in parent directory" >&2
    fi
fi

if [ ${PKG_BUILT} -eq 0 ]; then
    echo "Warning: no native package built. Add target/release/geopmd-proxy to PATH if needed." >&2
fi

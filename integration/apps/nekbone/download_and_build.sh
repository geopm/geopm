#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Clear out old versions:
rm -rf nekbone

# Acquire the source:
svn checkout https://repocafe.cels.anl.gov/repos/nekbone
base_dir=$PWD

# Change directories to the unpacked files.
cd nekbone/trunk/nekbone

# Patch nekbone with the patch utility:
patch -p1 < $base_dir/0001-Fix-whitespace-issues.patch
patch -p1 < $base_dir/0002-Link-w-GEOPM.patch
patch -p1 < $base_dir/0003-Only-run-the-12th-order-polynomial.patch
patch -p1 < $base_dir/0004-Increase-maximum-number-of-elements-to-32768.patch
patch -p1 < $base_dir/0005-Add-Epoch-markup.patch
patch -p1 < $base_dir/0006-Increase-iterations-to-2000.patch
patch -p1 < $base_dir/0007-Update-GEOPM_PREFIX.patch

# Build
cd test/example1
./makenek-intel

#!/bin/bash
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

# Checks that the BIOS for a Skylake node is configured to support RAPL.

# This script must be run as root.
# MSR bitfields and unit conversion factors are specific to Skylake.
# Adjust the two package CPUs below if using a SKU with different
# number of cores

#set -x

MSR_TOOLS=/home/drguttma/msr-tools
RDMSR=${MSR_TOOLS}/rdmsr
WRMSR=${MSR_TOOLS}/wrmsr

# MSR_PKG_POWER_LIMIT register offset
OFFSET=0x610
# Representative CPU for each package
# The MSR is package-scoped so all CPUs on
# the same package share the register
# These values are for a 20-core SKU.
PKG0_CPU=0
PKG1_CPU=20

# first sanity check the lock bit
LOCK_FIELD=63:63
LOCK_BIT_0=$($RDMSR -x -f $LOCK_FIELD -p $PKG0_CPU $OFFSET)
if [ $LOCK_BIT_0 -ne 0 ]; then
    echo "Fail: RAPL lock bit is set on package 0."
fi
LOCK_BIT_1=$($RDMSR -x -f $LOCK_FIELD -p $PKG1_CPU $OFFSET)
if [ $LOCK_BIT_1 -ne 0 ]; then
    echo "Fail: RAPL lock bit is set on package 1."
fi

if [ $LOCK_BIT_0 -eq 0 ] && [ $LOCK_BIT_1 -eq 0 ]; then
    echo "Success: RAPL lock bit looks sane."
fi

# TODO: check PL1_LIMIT_ENABLE (bit 15)

# try to set a power cap and see if it holds
POWER_LIMIT_FIELD=14:0
POWER_UNITS=0.125

BEGIN_PKG0_MSR=$($RDMSR -x -p $PKG0_CPU $OFFSET)
BEGIN_PKG1_MSR=$($RDMSR -x -p $PKG1_CPU $OFFSET)
BEGIN_PKG0_POWER=$($RDMSR -x -f $POWER_LIMIT_FIELD -p $PKG0_CPU $OFFSET)
BEGIN_PKG1_POWER=$($RDMSR -x -f $POWER_LIMIT_FIELD -p $PKG1_CPU $OFFSET)

echo "Initial value of PL1 limit:"
python3 -c "print(\"Package 0: {}W\".format(0x$BEGIN_PKG0_POWER * $POWER_UNITS))"
python3 -c "print(\"Package 1: {}W\".format(0x$BEGIN_PKG1_POWER * $POWER_UNITS))"

if [ $BEGIN_PKG0_MSR != $BEGIN_PKG1_MSR ]; then
    echo "Warning: initial power limits of two packages differ."
fi

# Change bit 3 and write the register.
# This is equivalent to changing the power limit by 1 watt.
# Warning: make sure not to write the lock bit.
# Node must be rebooted to reset it.
SET_POWER=3851000148430
if [ "$SET_POWER" == "$BEGIN_PKG0_MSR" ]; then
    echo "Error: SET_POWER is same as package0 original value.  No changes will be tested."
fi
if [ "$SET_POWER" == "$BEGIN_PKG1_MSR" ]; then
    echo "Error: SET_POWER is same as package1 original value.  No changes will be tested."
fi

$WRMSR -p $PKG0_CPU $OFFSET 0x$SET_POWER
$WRMSR -p $PKG1_CPU $OFFSET 0x$SET_POWER

# Check if power changed
END_PKG0_MSR=$($RDMSR -x -p $PKG0_CPU $OFFSET)
END_PKG1_MSR=$($RDMSR -x -p $PKG1_CPU $OFFSET)
if [ "$END_PKG0_MSR" != "$SET_POWER" ]; then
    echo "Fail: Power setting on package 0 silently ignored."
else
    echo "Success: Power limit changed on package 0."
fi
if [ "$END_PKG1_MSR" != "$SET_POWER" ]; then
    echo "Fail: Power setting on package 1 silently ignored."
else
    echo "Success: Power limit changed on package 1."
fi

# Restore initial power limit values
$WRMSR -p $PKG0_CPU $OFFSET 0x$BEGIN_PKG0_MSR
$WRMSR -p $PKG1_CPU $OFFSET 0x$BEGIN_PKG1_MSR
# Sanity check
if [ "$($RDMSR -p $PKG0_CPU $OFFSET)" != "$BEGIN_PKG0_MSR" ]; then
    echo "Warning: Failed to restore package 0 power limit."
else
    echo "Success: Power limit restored on package 0."
fi

if [ "$($RDMSR -p $PKG1_CPU $OFFSET)" != "$BEGIN_PKG1_MSR" ]; then
    echo "Warning: Failed to restore package 1 power limit."
else
    echo "Success: Power limit restored on package 1."
fi

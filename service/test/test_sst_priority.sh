#!/bin/bash
#  Copyright (c) 2015 - 2021, Intel Corporation
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

if [[ $# -gt 0 ]] && [[ $1 == '--help' ]]; then
    echo "
    Set Core Priority with SST
    --------------------------

    Runs a CPU stress test binary (stress-ng) on all cores of the
    platform.  While the all of the CPUs are loaded the GEOPM service
    is used to configure the Intel SST feature to give high priority
    to two cores (cores 0 and 5).  The CPU frequencies of the first 8
    cores on the system are printed before and after configuring SST
    core priority.

    This test is a work in progress.

"
    exit 0
fi

# Input for geopmsession to read frequency

# Input for geopmsession to configure SST
BENCH_LOG=$(mktemp)
SESSION_LOG=$(mktemp)
READ_REQUEST=$(mktemp)
WRITE_REQUEST=$(mktemp)

echo 'CPU_FREQUENCY_STATUS core 0
CPU_FREQUENCY_STATUS core 1
CPU_FREQUENCY_STATUS core 2
CPU_FREQUENCY_STATUS core 3
CPU_FREQUENCY_STATUS core 4
CPU_FREQUENCY_STATUS core 5
CPU_FREQUENCY_STATUS core 6
CPU_FREQUENCY_STATUS core 7' > ${READ_REQUEST}

echo 'SST::COREPRIORITY_ENABLE:ENABLE board 0 1
SST::TURBO_ENABLE:ENABLE board 0 1
SST::COREPRIORITY:ASSOCIATION board 0 3
SST::COREPRIORITY:ASSOCIATION core 0 0
SST::COREPRIORITY:ASSOCIATION core 5 0' > ${WRITE_REQUEST}

# Run stress workload on all cores
stress-ng --parallel 0 --class cpu >& $BENCH_LOG &
BENCH_ID=$!

# Print CPU frequencies for the first 8 cores
cat ${READ_REQUEST} | geopmsession > ${SESSION_LOG}

# Open a write session with the GEOPM service configuring SST
cat ${WRITE_REQUEST} | geopmsession -w -t 600 &
SESSION_ID=$!
sleep 1

# Read the CPU frequencies again and note different values
cat ${READ_REQUEST} | geopmsession >> ${SESSION_LOG}
cat ${SESSION_LOG} | python3 -c "import sys;
expect = [1, -1, -1, -1, -1, 1, -1, -1];
delta = [b - a for (a, b) in zip(*[[float(x) for x in line.split(',')] for line in sys.stdin if line.strip()])];
result = [e * d > 0 for (e, d) in zip(expect, delta)];
if False in result:
    sys.stderr.write('Change in frequency for first eight CPUs:\n    {}\n\n'.format(delta));
    sys.stderr.write('FAIL\n');
    exit(-1);
print('SUCCESS\n');
exit(0);
"

# Kill the benchmark and the write session
kill -9 ${BENCH_ID}
kill -9 ${SESSION_ID}
rm ${BENCH_LOG}
rm ${SESSION_LOG}
rm ${READ_REQUEST}
rm ${WRITE_REQUEST}

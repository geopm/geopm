#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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

"
    exit 0
fi

if [[ ! -e /dev/isst_interface ]]; then
    echo "Warning: Skipping $0, isst_interface driver is not present" 1>&2
    exit 0
fi

BENCH_LOG=$(mktemp)
SESSION_LOG=$(mktemp)
READ_REQUEST=$(mktemp)
WRITE_REQUEST=$(mktemp)

# Input for geopmsession to read frequency
echo 'CPU_FREQUENCY_STATUS core 0
CPU_FREQUENCY_STATUS core 1
CPU_FREQUENCY_STATUS core 2
CPU_FREQUENCY_STATUS core 3
CPU_FREQUENCY_STATUS core 4
CPU_FREQUENCY_STATUS core 5
CPU_FREQUENCY_STATUS core 6
CPU_FREQUENCY_STATUS core 7' > ${READ_REQUEST}

# Input for geopmsession to configure SST
echo 'SST::COREPRIORITY_ENABLE:ENABLE board 0 1
SST::TURBO_ENABLE:ENABLE board 0 1
SST::COREPRIORITY:ASSOCIATION board 0 3
SST::COREPRIORITY:ASSOCIATION core 0 0
SST::COREPRIORITY:ASSOCIATION core 5 0
SST::COREPRIORITY:0:WEIGHT board 0 0
SST::COREPRIORITY:0:FREQUENCY_MIN board 0 3.0e9
SST::COREPRIORITY:0:FREQUENCY_MAX board 0 4.0e9
SST::COREPRIORITY:3:WEIGHT board 0 15
SST::COREPRIORITY:3:FREQUENCY_MIN board 0 1.0e9
SST::COREPRIORITY:3:FREQUENCY_MAX board 0 4.0e9' > ${WRITE_REQUEST}

# Run stress workload on all cores
stress-ng --parallel 0 --class cpu >& $BENCH_LOG &
BENCH_ID=$!
sleep 1

# Print CPU frequencies for the first 8 cores
cat ${READ_REQUEST} | geopmsession > ${SESSION_LOG}

# Open a write session with the GEOPM service configuring SST
for req in $(cat ${WRITE_REQUEST}); do
    geopmwrite ${req}
done

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
err=$?

# Kill the benchmark and the write session
kill -9 ${BENCH_ID}
rm ${BENCH_LOG}
rm ${SESSION_LOG}
rm ${READ_REQUEST}
rm ${WRITE_REQUEST}
exit $err

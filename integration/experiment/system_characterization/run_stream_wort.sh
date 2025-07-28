#!/usr/bin/env bash

CORE_COUNT=$(geopmread -d | grep "core" | awk '{print $2}')
RANK_COUNT=2
export OMP_NUM_THREADS=$(((CORE_COUNT-4)/RANK_COUNT))
export KMP_AFFINITY="granularity=core"
echo '{"loop-count": 1,"region": ["stream"],"big-o": [1.0]}' > geopmbench.config
/usr/bin/time -f'Time: %e' 2>&1 mpiexec -n ${RANK_COUNT} -ppn ${RANK_COUNT} geopmbench geopmbench.config

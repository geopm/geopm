#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [[ $# != 1 ]]; then
    echo "Usage: $0 system_name"
    echo
    exit -1
fi

#
# This script is a hacky solution.  It uses geopmread and geopmwrite
# which should not be referenced in the service directory.  To fix
# this problem, geopmaccess must be modified to show the description
# text as a command line option.  It would be best if that output
# conformed to reST conventions so we could avoid the sed filter.
#

system=$1
outfile=source/signals_${system}.rst
line="${system} Platform Signals"
underline=$(echo ${line} | sed -e 's|.|=|g')
echo ${line} > ${outfile}
echo ${underline} >> ${outfile}
echo >> ${outfile}
for sname in $(geopmread); do
    geopmread --info ${sname} |
        sed -e 's|^    \(.*\)|    - \1:|' -e 's|:$||' >> ${outfile}
done

outfile=source/controls_${system}.rst
line="${system} Platform Controls"
underline=$(echo ${line} | sed -e 's|.|=|g')
echo ${line} > ${outfile}
echo ${underline} >> ${outfile}
echo >> ${outfile}

for cname in $(geopmwrite); do
    geopmwrite --info ${cname} |
        sed -e 's|^    \(.*\)|    - \1:|' -e 's|:$||' >> ${outfile}
done

#!/bin/bash

if [[ $# != 1 ]]; then
    echo "Usage: $0 system_name"
    echo
    exit -1
fi
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

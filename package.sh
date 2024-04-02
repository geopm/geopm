#!/bin/bash

if grep -i ubuntu /etc/os-release || grep -i debian /etc/os-release; then
    pkg=deb
else
    pkg=rpm
fi

for cdir in libgeopmd libgeopm; do 
    cd $cdir
    ./autogen.sh && ./configure && make $pkg
    cd -
done

for pdir in geopmdpy geopmpy docs; do
    cd $pdir
    ./make_$pkg.sh
    cd -
done

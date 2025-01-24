# Golang Bindings for libgeopmd

This directory contains the geopmdgo package which provides bindings to some
of the C interfaces provided by libgeopmd.  These bindings allow the user to
interact with the PlatformIO and PlatformTopo interfaces in the Go language.

## The examples directory

A golang based implementation of the geopmread command line interface is included
as an example of how to use the GEOPM interfaces in Golang.

## The test script

A simple bash script called `test.sh` can be executed to run unit tests, and the
example application.  Check the return code from this script to validate
correctness.

```bash

cd geopm/geopmdgo
./test.sh
if [[ $? -ne 0 ]]; then
    echo "Error: Test script has failed" 1>&2
else
    echo "SUCCESS!"
fi

```

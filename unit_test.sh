#!/bin/bash

#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/build/geopm/lib $GEOPM_GDB ./test/.libs/geopm_test # --gtest_filter=$1
LD_LIBRARY_PATH=".libs:$LD_LIBRARY_PATH" ./test/.libs/geopm_test --gtest_filter=$1

#!/bin/bash

LD_LIBRARY_PATH=".libs:$LD_LIBRARY_PATH" ./test/.libs/geopm_test --gtest_filter=$1

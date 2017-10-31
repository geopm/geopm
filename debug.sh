#!/bin/bash
LD_LIBRARY_PATH=".libs:$LD_LIBRARY_PATH" gdb -q -iex "set auto-load safe-path $LD_LIBRARY_PATH/*" \
    --command=.gdbinit.geopm.local ./test/.libs/geopm_test

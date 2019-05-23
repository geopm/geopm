#!/bin/bash

grep '^Region stream' -A 10 $1 | \
egrep '^Region stream|requested' | \
sed 's|.*\(stream-[0-9\.]*-dgemm-[0-9\.]*\).*| \1|g' | \
sed 's|    requested-online-frequency: |:|' | \
tr -d '\n' | tr ' ' '\n' | grep . | sed 's|stream-||' | sort -n | sed 's|^|stream-|'

#!/bin/bash

set -e
set -x
TMP_FILE=$(mktemp)
geopmsession --help > $TMP_FILE
test -s $TMP_FILE
rm -f $TMP_FILE

#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

echo 'SERVICE::TIME board 0' | geopmsession -t 1000 -p 1 > /dev/null &
echo $!

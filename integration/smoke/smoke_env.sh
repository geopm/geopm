#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [ -f ${HOME}/.geopmrc ]; then
    source ${HOME}/.geopmrc
fi

if [ ! -z ${GEOPM_SYSTEM_ENV} ]; then
    source ${GEOPM_SYSTEM_ENV}
fi

GEOPM_SOURCE=${GEOPM_SOURCE:?Please set GEOPM_SOURCE in your environment.}
source ${GEOPM_SOURCE}/integration/config/run_env.sh
EXP_DIR=${GEOPM_SOURCE}/integration/experiment

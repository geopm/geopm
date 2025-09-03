#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

export GEOPM_SOURCE="${GEOPM_SOURCE?-Set GEOPM_SOURCE to full path of file with initialization parameters}"
export PYTHONPATH=${GEOPM_SOURCE}/:${GEOPM_SOURCE}/geopmpy/:${GEOPM_SOURCE}/geopmdpy/:${HOME}/.local/lib/python3.6/site-packages/:$PYTHONPATH

export GEOPM_INSTALL="${GEOPM_INSTALL?-Set full path to geopm installation directory}"
export PATH=${GEOPM_INSTALL}/bin:$PATH
export LD_LIBRARY_PATH=${GEOPM_INSTALL}/lib:$LD_LIBRARY_PATH


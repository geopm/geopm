#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

EXTRA_DIST += shell_completion/README.md \
              shell_completion/geopmlaunch.bash \
              shell_completion/geopmread.bash \
              shell_completion/geopmwrite.bash \
              #end

if ENABLE_BASH_COMPLETION
dist_bashcomp_DATA = shell_completion/geopmlaunch.bash \
                     shell_completion/geopmread.bash \
                     shell_completion/geopmwrite.bash \
                     # end
endif

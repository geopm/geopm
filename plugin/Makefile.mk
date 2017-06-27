#  Copyright (c) 2015, 2016, 2017, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

if ENABLE_MPI
pkglib_LTLIBRARIES += libgeopmpi_mpi.la
libgeopmpi_mpi_la_SOURCES = src/geopm_mpi_comm.cpp \
                            plugin/MPIComm.hpp \
                            plugin/MPIComm.cpp \
                            # end
endif

pkglib_LTLIBRARIES += libgeopmpi_governing.la
libgeopmpi_governing_la_SOURCES = plugin/GoverningDecider.cpp \
                                  plugin/GoverningDecider.hpp \
                                  # end

pkglib_LTLIBRARIES += libgeopmpi_balancing.la
libgeopmpi_balancing_la_SOURCES = plugin/BalancingDecider.cpp \
                                  plugin/BalancingDecider.hpp \
                                  # end

# -module required to force .so generation of plugin.
if ENABLE_MPI
libgeopmpi_mpi_la_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
libgeopmpi_mpi_la_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
libgeopmpi_mpi_la_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS) -module
endif
libgeopmpi_governing_la_LDFLAGS = $(AM_LDFLAGS) -module
libgeopmpi_balancing_la_LDFLAGS = $(AM_LDFLAGS) -module

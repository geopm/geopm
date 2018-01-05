#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

check_LTLIBRARIES += libgeopmpi_test.la
libgeopmpi_test_la_SOURCES = test/plugin/TestPlugin.cpp \
                             test/plugin/TestPlugin.hpp \
                             #end

# -module required to force .so generation of test plugin.
libgeopmpi_test_la_LDFLAGS = $(LDFLAGS) $(AM_LDFLAGS) -module

if ENABLE_MPI
    check_PROGRAMS += test_plugin_app
    test_plugin_app_SOURCES = test/plugin/TestPluginApp.cpp
    test_plugin_app_LDADD = libgeopm.la
    test_plugin_app_CPPFLAGS = $(AM_CPPFLAGS) $(MPI_CFLAGS)
    test_plugin_app_LDFLAGS = $(AM_LDFLAGS) $(MPI_CXXLDFLAGS) $(MPI_CXXLIBS)
    test_plugin_app_CFLAGS = $(AM_CFLAGS) $(MPI_CFLAGS)
    test_plugin_app_CXXFLAGS = $(AM_CXXFLAGS) $(MPI_CXXFLAGS)
endif

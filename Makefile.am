#  Copyright (c) 2015, Intel Corporation
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

AM_CPPFLAGS = -I$(top_srcdir)/src
lib_LTLIBRARIES = libgeopm.la libgeopmpolicy.la

libgeopm_la_LDFLAGS = -version-info $(geopm_abi_version)
libgeopmpolicy_la_LDFLAGS = -version-info $(geopm_abi_version)

libgeopm_la_SOURCES = src/CircularBuffer.cpp \
                      src/CircularBuffer.hpp \
                      src/Controller.cpp \
                      src/Controller.hpp \
                      src/Decider.cpp \
                      src/Decider.hpp \
                      src/IVTPlatform.cpp \
                      src/IVTPlatform.hpp \
                      src/IVTPlatformImp.cpp \
                      src/IVTPlatformImp.hpp \
                      src/Observation.cpp \
                      src/Observation.hpp \
                      src/Phase.cpp \
                      src/Phase.hpp \
                      src/Platform.cpp \
                      src/PlatformFactory.cpp \
                      src/PlatformFactory.hpp \
                      src/Platform.hpp \
                      src/PlatformImp.cpp \
                      src/PlatformImp.hpp \
                      src/PlatformTopology.cpp \
                      src/PlatformTopology.hpp \
                      src/Policy.cpp \
                      src/Policy.hpp \
                      src/PowerModel.cpp \
                      src/PowerModel.hpp \
                      src/TreeCommunicator.cpp \
                      src/TreeCommunicator.hpp \
                      src/geopm_ctl_spawn.c \
                      src/geopm_policy.c \
                      # end

libgeopmpolicy_la_SOURCES = src/PolicyController.cpp \
                            src/PolicyController.hpp \
                            src/geopm_policy.c \
                            src/geopm_policy.h \
                            # end

include_HEADERS = src/geopm.h \
                  src/geopm_policy.h
                  # end

bin_PROGRAMS = geopmctl

geopmctl_SOURCES = src/geopmctl_main.c \
                   src/geopm.h \
                   # end

geopmctl_LDADD = libgeopm.la

EXTRA_DIST = CONTRIBUTING \
             autogen.sh \
             # end

dist_doc_DATA = README COPYING VERSION

clean-local: clean-local-coverage

clean-local-coverage:
	rm -f `find -name \*.gcno` `find -name \*.gcda`

check_PROGRAMS =
TESTS =

include test/Makefile.mk

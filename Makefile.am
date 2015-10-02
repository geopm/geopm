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

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(top_srcdir)/src
lib_LTLIBRARIES = libgeopm.la \
                  libgeopmpolicy.la \
                  # end

libgeopm_la_LDFLAGS = -version-info $(geopm_abi_version)
libgeopmpolicy_la_LDFLAGS = -version-info $(geopm_abi_version)

libgeopm_la_SOURCES = src/CircularBuffer.cpp \
                      src/CircularBuffer.hpp \
                      src/Controller.cpp \
                      src/Controller.hpp \
                      src/Decider.cpp \
                      src/Decider.hpp \
                      src/Exception.cpp \
                      src/Exception.hpp \
                      src/GlobalPolicy.cpp \
                      src/GlobalPolicy.hpp \
                      src/HSXPlatformImp.cpp \
                      src/HSXPlatformImp.hpp \
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
                      src/Profile.cpp \
                      src/Profile.hpp \
                      src/TreeCommunicator.cpp \
                      src/TreeCommunicator.hpp \
                      src/geopm_ctl_spawn.c \
                      src/geopm_error.h \
                      src/geopm_policy_message.c \
                      src/geopm_policy_message.h \
                      src/geopm_version.c \
                      # end

libgeopmpolicy_la_SOURCES = src/CircularBuffer.cpp \
                            src/CircularBuffer.hpp \
                            src/Exception.cpp \
                            src/Exception.hpp \
                            src/GlobalPolicy.cpp \
                            src/GlobalPolicy.hpp \
                            src/HSXPlatformImp.cpp \
                            src/HSXPlatformImp.hpp \
                            src/IVTPlatform.cpp \
                            src/IVTPlatform.hpp \
                            src/IVTPlatformImp.cpp \
                            src/IVTPlatformImp.hpp \
                            src/Observation.cpp \
                            src/Observation.hpp \
                            src/Phase.cpp \
                            src/Phase.hpp \
                            src/PlatformImp.cpp \
                            src/PlatformImp.hpp \
                            src/Platform.cpp \
                            src/Platform.hpp \
                            src/PlatformFactory.cpp \
                            src/PlatformFactory.hpp \
                            src/PlatformTopology.cpp \
                            src/PlatformTopology.hpp \
                            src/PowerModel.cpp \
                            src/PowerModel.hpp \
                            src/geopm_error.h \
                            src/geopm_policy.h \
                            src/geopm_version.c \
                            # end

include_HEADERS = src/geopm.h \
                  src/geopm_policy.h \
                  src/geopm_policy_message.h \
                  # end

bin_PROGRAMS = geopmctl \
               geopmpolicy \
               # end

geopmctl_SOURCES = src/geopmctl_main.c \
                   src/geopm.h \
                   # end

geopmctl_LDADD = libgeopm.la

geopmpolicy_SOURCES = src/geopmpolicy_main.c \
                      src/geopm_policy.h \
                      src/geopm_policy_message.h \
                      # end

geopmpolicy_LDADD = libgeopmpolicy.la

dist_man_MANS = man/geopm.3 \
                man/geopmctl.3 \
                man/geopm_ctl_c.3 \
                man/geopmkey.3 \
                man/geopmpolicy.3 \
                man/geopm_policy_c.3 \
                man/geopm_prof_c.3 \
                man/geopm_version.3 \
                # end

EXTRA_DIST = autogen.sh \
             ChangeLog \
             configure.ac \
             CONTRIBUTING \
             copying_headers/header.freeBSD3 \
             copying_headers/test-license \
             copying_headers/MANIFEST.EXEMPT \
             copying_headers/MANIFEST.freeBSD3 \
             examples/README \
             examples/threaded_step_example.c \
             geopm.spec.mk \
             geopm-$(VERSION).spec \
             Makefile.am \
             MANIFEST \
             TODO \
             # end

dist_doc_DATA = README \
                COPYING \
                VERSION \
                # end

CLEANFILES = geopm-$(VERSION).spec
DISTCLEANFILES = VERSION

clean-local: clean-local-coverage

clean-local-coverage:
	rm -f `find -name \*.gcno` `find -name \*.gcda`

geopm-$(VERSION).spec:
	$(MAKE) version="$(VERSION)" -f geopm.spec.mk $@

rpm: dist
	$(MAKE) version="$(VERSION)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)" \
		-f geopm.spec.mk $@

check_PROGRAMS =
TESTS =

include test/Makefile.mk

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

AM_CPPFLAGS += -I$(googletest)/include
AM_CPPFLAGS += -I$(googlemock)/include
BUILT_SOURCES += $(googletest_suite)/VERSION
EXTRA_DIST += $(googletest_suite_archive)
DISTCLEANFILES += $(googletest_suite)/VERSION \
                  $(googletest_suite_archive) \
                  # end

dist-googletest: googletest_archive_check
check-am: libgmock.a libgtest.a
clean-local-googletest: clean-local-gmock

googletest_version = 1.8.1
googletest_suite = googletest-release-$(googletest_version)
googlemock = $(googletest_suite)/googlemock
googletest = $(googletest_suite)/googletest
googletest_suite_archive = release-$(googletest_version).tar.gz
googletest_url = https://github.com/google/googletest/archive/$(googletest_suite_archive)
googletest_sha1 = 152b849610d91a9dfa1401293f43230c2e0c33f8

$(googletest_suite_archive):
	wget --timeout=20 $(googletest_url) || \
	curl --connect-timeout 20 -O $(googletest_url) || \
	echo "Warning: Unable to download gmock archive" 2>&1 && \
	touch $(googletest_suite_archive)

googletest_archive_check: $(googletest_suite_archive)
	@if [ ! -s $^ ]; then \
	    echo "Error: The gmock archive is empty" 2>&1; \
	    exit -1; \
	fi
	@if [ $$(sha1sum $^ | awk '{print $$1}') != $(googletest_sha1) ]; then \
	    echo "Error: The gmock archive does not have the correct SHA-1 checksum" 2>&1; \
	    exit -1; \
	fi
	@echo '[ PASSED ] googletest_archive_check'

$(googletest_suite)/VERSION: $(googletest_suite_archive)
	@if [ ! -s $^ ]; then \
	    mkdir -p $(googletest_suite); \
	    touch $(googletest_suite)/VERSION; \
	elif [ $$(sha1sum $^ | awk '{print $$1}') != $(googletest_sha1) ]; then \
	    echo "Error: The gmock archive does not have the correct SHA-1 checksum" 2>&1; \
	    exit -1; \
	else \
	    rm -rf $(googletest_suite); \
	    tar -xvf $^; \
	    echo $(googletest_version) > $(googletest_suite)/VERSION; \
	fi

libgmock.a: $(googletest_suite)/VERSION
	@if [ ! -s $^ ]; then \
	    echo "Error: Failure to extract or download gmock archive" 2>&1; \
	    exit -1; \
	fi
	$(CXX) $(CXXFLAGS) -isystem $(googlemock)/include -I$(googlemock) -isystem $(googletest)/include -I$(googletest) -pthread -fno-delete-null-pointer-checks \
	      -c $(googlemock)/src/gmock-all.cc
	ar -rv libgmock.a gmock-all.o

libgtest.a: $(googletest_suite)/VERSION
	@if [ ! -s $^ ]; then \
	    echo "Error: Failure to extract or download gmock archive" 2>&1; \
	    exit -1; \
	fi
	$(CXX) $(CXXFLAGS) -isystem $(googletest)/include -I$(googletest) -pthread -fno-delete-null-pointer-checks \
	      -c $(googletest)/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o

clean-local-gmock:
	rm -rf libgtest.a libgmock.a $(googletest_suite)

PHONY_TARGETS += googletest_archive_check clean-local-gmock

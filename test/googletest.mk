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

AM_CPPFLAGS += -I$(googletest)/include
AM_CPPFLAGS += -I$(googlemock)/include
BUILT_SOURCES += $(googlemock)/VERSION
EXTRA_DIST += $(googlemock_archive)
DISTCLEANFILES += $(googlemock)/VERSION \
                  $(googlemock_archive) \
                  # end

dist-googletest: googlemock_archive_check
check-am: libgmock.a libgtest.a
clean-local-googletest: clean-local-gmock

googlemock_version = 1.7.0
googlemock = gmock-$(googlemock_version)
googletest = $(googlemock)/gtest
googlemock_archive = $(googlemock).zip
googlemock_url = http://pkgs.fedoraproject.org/repo/pkgs/gmock/gmock-1.7.0.zip/073b984d8798ea1594f5e44d85b20d66/$(googlemock_archive)
googlemock_sha1 = f9d9dd882a25f4069ed9ee48e70aff1b53e3c5a5

$(googlemock_archive):
	wget --timeout=20 $(googlemock_url) || \
	curl --connect-timeout 20 -O $(googlemock_url) || \
	echo "Warning: Unable to download gmock archive" 2>&1 && \
	touch $(googlemock_archive)

googlemock_archive_check: $(googlemock_archive)
	@if [ ! -s $^ ]; then \
	    echo "Error: The gmock archive is empty" 2>&1; \
	    exit -1; \
	fi
	@if [ $$(sha1sum $^ | awk '{print $$1}') != $(googlemock_sha1) ]; then \
	    echo "Error: The gmock archive does not have the correct SHA-1 checksum" 2>&1; \
	    exit -1; \
	fi
	@echo '[ PASSED ] googlemock_archive_check'

$(googlemock)/VERSION: $(googlemock_archive)
	@if [ ! -s $^ ]; then \
	    mkdir -p $(googlemock); \
	    touch $(googlemock)/VERSION; \
	elif [ $$(sha1sum $^ | awk '{print $$1}') != $(googlemock_sha1) ]; then \
	    echo "Error: The gmock archive does not have the correct SHA-1 checksum" 2>&1; \
	    exit -1; \
	else \
	    rm -rf $(googlemock); \
	    unzip $^; \
	    echo $(googlemock_version) > $(googlemock)/VERSION; \
	fi

libgmock.a: $(googlemock)/VERSION
	@if [ ! -s $^ ]; then \
	    echo "Error: Failure to extract or download gmock archive" 2>&1; \
	    exit -1; \
	fi
	$(CXX) $(CXXFLAGS) -isystem $(googlemock)/include -I$(googlemock) -isystem $(googletest)/include -I$(googletest) -pthread -fno-delete-null-pointer-checks \
	      -c $(googlemock)/src/gmock-all.cc
	ar -rv libgmock.a gmock-all.o

libgtest.a: $(googlemock)/VERSION
	@if [ ! -s $^ ]; then \
	    echo "Error: Failure to extract or download gmock archive" 2>&1; \
	    exit -1; \
	fi
	$(CXX) $(CXXFLAGS) -isystem $(googletest)/include -I$(googletest) -pthread -fno-delete-null-pointer-checks \
	      -c $(googletest)/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o

clean-local-gmock:
	rm -rf libgtest.a libgmock.a $(googlemock)

PHONY_TARGETS += googlemock_archive_check clean-local-gmock

#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

AM_CPPFLAGS += -I$(googletest)/include
AM_CPPFLAGS += -I$(googlemock)/include
BUILT_SOURCES += $(googletest_suite)/VERSION
EXTRA_DIST += $(googletest_suite_archive_download)
DISTCLEANFILES += $(googletest_suite)/VERSION \
                  $(googletest_suite_archive) \
                  # end

dist-googletest: googletest_archive_check
check-am: libgmock.a libgtest.a
clean-local-googletest: clean-local-gmock

googletest_version = 1.12.1
googletest_suite = googletest-release-$(googletest_version)
googlemock = $(googletest_suite)/googlemock
googletest = $(googletest_suite)/googletest
googletest_suite_archive = release-$(googletest_version).tar.gz
googletest_suite_archive_download = googletest-$(googletest_suite_archive)
googletest_url = https://github.com/google/googletest/archive/$(googletest_suite_archive)
googletest_sha1 = cdddd449d4e3aa7bd421d4519c17139ea1890fe7
$(googletest_suite_archive_download):
	wget --timeout=20 -O $(googletest_suite_archive_download) $(googletest_url) || \
	curl --connect-timeout 20 -L -o $(googletest_suite_archive_download) $(googletest_url) || \
	echo "Warning: Unable to download gmock archive" 2>&1 && \
	touch $(googletest_suite_archive_download)

googletest_archive_check: $(googletest_suite_archive_download)
	@if [ ! -s $^ ]; then \
	    echo "Error: The gmock archive is empty" 2>&1; \
	    exit -1; \
	fi
	@if [ $$(sha1sum $^ | awk '{print $$1}') != $(googletest_sha1) ]; then \
	    echo "Error: The gmock archive does not have the correct SHA-1 checksum" 2>&1; \
	    exit -1; \
	fi
	@echo '[ PASSED ] googletest_archive_check'

$(googletest_suite)/VERSION: $(googletest_suite_archive_download)
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
	$(CXX) $(CXXFLAGS) -fPIC -fPIE -isystem $(googlemock)/include -I$(googlemock) -isystem $(googletest)/include -I$(googletest) -pthread -std=c++17\
	      -c $(googlemock)/src/gmock-all.cc
	ar -rv libgmock.a gmock-all.o

libgtest.a: $(googletest_suite)/VERSION
	@if [ ! -s $^ ]; then \
	    echo "Error: Failure to extract or download gmock archive" 2>&1; \
	    exit -1; \
	fi
	$(CXX) $(CXXFLAGS) -fPIC -fPIE -isystem $(googletest)/include -I$(googletest) -pthread -std=c++17 \
	      -c $(googletest)/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o

clean-local-gmock:
	rm -rf libgtest.a libgmock.a $(googletest_suite)

PHONY_TARGETS += googletest_archive_check clean-local-gmock

#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

geopmhash_corpus_archive_name = geopmhash_corpus.tar.gz
geopmhash_corpus_archive = fuzz_test/$(geopmhash_corpus_archive_name)
geopmhash_corpus_dir = fuzz_test/geopmhash_corpus/.dir

corpus_archives_base_url = https://geopm.github.io/fuzz

EXTRA_DIST += $(geopmhash_corpus_archive) \
              fuzz_test/run_fuzz_tests.sh \
              # end
BUILT_SOURCES += $(geopmhash_corpus_dir)
DISTCLEANFILES += $(geopmhash_corpus_archive)

clean-local-fuzztest:
	rm -rf $(geopmhash_corpus_dir)

if ENABLE_FUZZTESTS
check_PROGRAMS += fuzz_test/geopmhash_fuzz_test \
                  fuzz_test/geopmhash_reg_test \
                  # end
endif
fuzz_test_geopmhash_fuzz_test_SOURCES = fuzz_test/geopmhash_harness.cpp
fuzz_test_geopmhash_reg_test_SOURCES = fuzz_test/geopmhash_harness.cpp \
                                       fuzz_test/StandaloneFuzzTargetMain.cpp \
                                       # end
fuzz_test_geopmhash_fuzz_test_CXXFLAGS = $(AM_CXXFLAGS) -fsanitize=fuzzer -fno-inline
fuzz_test_geopmhash_reg_test_CXXFLAGS = $(AM_CXXFLAGS) -fno-inline
fuzz_test_geopmhash_fuzz_test_LDADD = libgeopmd.la
fuzz_test_geopmhash_reg_test_LDADD = libgeopmd.la

$(geopmhash_corpus_dir): $(geopmhash_corpus_archive)
	tar -xvf $< -C fuzz_test
	touch $@

$(geopmhash_corpus_archive):
	wget --timeout=20 -O $@ $(corpus_archives_base_url)/$(geopmhash_corpus_archive_name) || \
	curl --connect-timeout 20 -L -o $@ $(corpus_archives_base_url)/$(geopmhash_corpus_archive_name) || \
	echo "Warning: Unable to download corpus archive $@"

PHONY_TARGETS += clean-local-fuzztest

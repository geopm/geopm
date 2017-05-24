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

prefix ?= /usr
exec_prefix ?= $(prefix)
bindir ?= $(prefix)/bin
libexecdir ?= $(exec_prefix)/libexec

EXTRA_DIST += scripts/geopmpy/launcher.py \
              scripts/geopmpy/plotter.py \
              scripts/geopmpy/io.py \
              scripts/geopmpy/version.py \
              scripts/geopmpy/__init__.py \
              scripts/geopmaprun \
              scripts/geopmsrun \
              scripts/geopmplotter \
              scripts/setup.py \
              scripts/MANIFEST.in \
              scripts/test/TestAffinity.py \
              scripts/test/geopm_context.py \
              scripts/test/__init__.py \
              scripts/test/geopmpy_test.sh
              # end

PYTEST_TESTS = scripts/test/pytest_links/TestAffinity.test_affinity_0 \
               scripts/test/pytest_links/TestAffinity.test_affinity_1 \
               scripts/test/pytest_links/TestAffinity.test_affinity_2 \
               scripts/test/pytest_links/TestAffinity.test_affinity_3 \
               scripts/test/pytest_links/TestAffinity.test_affinity_4 \
               scripts/test/pytest_links/TestAffinity.test_affinity_5 \
               scripts/test/pytest_links/TestAffinity.test_affinity_6 \
               scripts/test/pytest_links/TestAffinity.test_affinity_7 \
               scripts/test/pytest_links/TestAffinity.test_affinity_8 \
               scripts/test/pytest_links/TestAffinity.test_affinity_9 \
               scripts/test/pytest_links/TestAffinity.test_affinity_10 \
               scripts/test/pytest_links/TestAffinity.test_affinity_11 \
               scripts/test/pytest_links/TestAffinity.test_affinity_12 \
               # end

TESTS += $(PYTEST_TESTS)

pytest-checkprogs: $(PYTEST_TESTS)

PHONY_TARGETS += pytest-checkprogs

$(PYTEST_TESTS): scripts/test/pytest_links/%:
	mkdir -p scripts/test/pytest_links
	ln -s ../geopmpy_test.sh $@

clean-local-pytest-script-links:
	rm -f scripts/test/pytest_links/*

CLEAN_LOCAL_TARGETS += clean-local-pytest-script-links

install-python:
	cd scripts && ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)

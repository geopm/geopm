#  Copyright (c) 2015 - 2021, Intel Corporation
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

EXTRA_DIST += geopmdpy_test/__main__.py \
              geopmdpy_test/TestPlatformService.py \
              geopmdpy_test/TestDBusXML.py \
              geopmdpy_test/TestError.py \
              geopmdpy_test/TestPIO.py \
              geopmdpy_test/TestTopo.py \
              # end

GEOPMDPY_TESTS = geopmdpy_test/pytest_links/TestPlatformService.test_get_group_access_empty \
                 geopmdpy_test/pytest_links/TestPlatformService.test_get_group_access_default \
                 geopmdpy_test/pytest_links/TestPlatformService.test_get_group_access_named \
                 geopmdpy_test/pytest_links/TestDBusXML.test_xml_parse_no_doc \
                 geopmdpy_test/pytest_links/TestDBusXML.test_xml_parse_with_doc \
                 geopmdpy_test/pytest_links/TestError.test_error_message \
                 geopmdpy_test/pytest_links/TestPIO.test_domain_name \
                 geopmdpy_test/pytest_links/TestPIO.test_signal_names \
                 geopmdpy_test/pytest_links/TestPIO.test_control_names \
                 geopmdpy_test/pytest_links/TestPIO.test_read_signal \
                 geopmdpy_test/pytest_links/TestPIO.test_write_control \
                 geopmdpy_test/pytest_links/TestTopo.test_num_domain \
                 geopmdpy_test/pytest_links/TestTopo.test_domain_domain_nested \
                 geopmdpy_test/pytest_links/TestTopo.test_domain_name_type \
                 # end

TESTS = $(GEOPMDPY_TESTS)

geopmdpy-checkprogs: $(GEOPMDPY_TESTS)

PHONY_TARGETS += pytest-checkprogs

$(GEOPMDPY_TESTS): geopmdpy_test/pytest_links/%:
	mkdir -p geopmdpy_test/pytest_links
	rm -f $@
	ln -s $(abs_srcdir)/geopmdpy_test/geopmdpy_test.sh $@

CLEAN_LOCAL_TARGETS += clean-local-geopmdpy-script-links

clean-local-geopmdpy-script-links:
	rm -f geopmdpy_test/pytest_links/*

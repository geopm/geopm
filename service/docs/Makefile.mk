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

EXTRA_DIST += docs/geninfo.sh \
              docs/source/admin.rst \
              docs/source/analysis.rst \
              docs/source/build.rst \
              docs/source/client.rst \
              docs/source/conf.py \
              docs/source/contrib.rst \
              docs/source/controls_SKX.rst \
              docs/source/devel.rst \
              docs/source/index.rst \
              docs/source/info.rst \
              docs/source/readme.rst \
              docs/source/reference.rst \
              docs/source/requires.rst \
              docs/source/runtime.rst \
              docs/source/signals_SKX.rst \
              docs/source/_static/.dirfile \
              # end

docs: libgeopmd.la $(abs_srcdir)/geopmdpy/version.py
	LD_LIBRARY_PATH=.libs:$(LD_LIBRARY_PATH) \
	PYTHONPATH=$(abs_srcdir):$(PYTHONPATH) \
	sphinx-build -M html $(abs_srcdir)/docs/source docs/build

clean-local-docs:
	rm -rf docs/build

CLEAN_LOCAL_TARGETS += clean-local-docs
PHONY_TARGETS += docs clean-local-docs

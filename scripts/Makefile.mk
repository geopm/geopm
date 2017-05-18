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

EXTRA_DIST += scripts/geopm/launcher.py \
              scripts/geopm/plotter.py \
              scripts/geopm/io.py \
              scripts/geopm/version.py \
              scripts/geopm/__init__.py \
              scripts/geopmaprun \
              scripts/geopmsrun \
              scripts/geopmplotter \
              scripts/setup.py \
              scripts/MANIFEST.in \
              # end

install-python:
	$(INSTALL) -d $(DESTDIR)$(libexecdir)/geopm/geopm
	$(INSTALL) -m 644 scripts/geopm/launcher.py $(DESTDIR)$(libexecdir)/geopm/geopm
	$(INSTALL) -m 644 scripts/geopm/plotter.py $(DESTDIR)$(libexecdir)/geopm/geopm
	$(INSTALL) -m 644 scripts/geopm/io.py $(DESTDIR)$(libexecdir)/geopm/geopm
	$(INSTALL) -m 644 scripts/geopm/version.py $(DESTDIR)$(libexecdir)/geopm/geopm
	$(INSTALL) -m 644 scripts/geopm/__init__.py $(DESTDIR)$(libexecdir)/geopm/geopm
	$(INSTALL) scripts/setup.py $(DESTDIR)$(libexecdir)/geopm
	$(INSTALL) scripts/geopmsrun $(DESTDIR)$(libexecdir)/geopm
	$(INSTALL) scripts/geopmaprun $(DESTDIR)$(libexecdir)/geopm
	$(INSTALL) scripts/geopmplotter $(DESTDIR)$(libexecdir)/geopm
	$(INSTALL) -m 644 scripts/MANIFEST.in $(DESTDIR)$(libexecdir)/geopm
	$(INSTALL) -m 644 README $(DESTDIR)$(libexecdir)/geopm
	$(INSTALL) -m 644 COPYING $(DESTDIR)$(libexecdir)/geopm
	ln -sf $(libexecdir)/geopm/geopmsrun $(DESTDIR)$(bindir)/geopmsrun
	ln -sf $(libexecdir)/geopm/geopmaprun $(DESTDIR)$(bindir)/geopmaprun
	ln -sf $(libexecdir)/geopm/geopmplotter $(DESTDIR)$(bindir)/geopmplotter

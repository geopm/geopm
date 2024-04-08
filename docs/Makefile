#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

prefix ?= /usr
datarootdir ?= $(prefix)/share
mandir ?= $(datarootdir)/man

all:
	# Do not make anything by default
	-

man:
	PYTHONPATH=../../geopmdpy:../../geopmpy \
	    sphinx-build -M man source build -W

html:
	PYTHONPATH=../../geopmdpy:../../geopmpy \
	    sphinx-build -M html source build -W

# Nobody asked for this build target, so don't make it a build-blocker unless
# we decide to keep it. If we choose to keep it, then be sure to ratchet in all
# the errors, add -W back to the options, and add this as a dependency to the
# `docs` target
geopmlint:
	PYTHONPATH=../../geopmdpy:../../geopmpy:$PYTHONPATH \
	    sphinx-build -M geopmlint source build

clean:
	rm -rf docs/build VERSION DATE debian/changelog geopm-docs-*

VERSION:
	python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > $@

DATE:
	date +'%a, %d %b %Y %H:%M:%S %z' > $@

dist: man VERSION DATE
	VERSION=`cat VERSION` && DATE=`cat DATE` && \
	cat debian/changelog.in | sed -e "s|@VERSION@|$$VERSION|g" \
	                              -e "s|@DATE@|$$DATE|g" > debian/changelog && \
	cd .. && tar --transform="s|docs|geopm-docs-$$VERSION|" -h -zcvf geopm-docs-$$VERSION.tar.gz docs && \
	mv geopm-docs-$$VERSION.tar.gz docs

install_man:
	mkdir -p $(DESTDIR)$(mandir)/man1
	mkdir -p $(DESTDIR)$(mandir)/man3
	mkdir -p $(DESTDIR)$(mandir)/man7
	install -m644 build/man/*.1 $(DESTDIR)$(mandir)/man1
	install -m644 build/man/*.3 $(DESTDIR)$(mandir)/man3
	install -m644 build/man/*.7 $(DESTDIR)$(mandir)/man7

.PHONY: all man html geopmlint clean DATE VERSION dist install_man

#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

io.github.geopm.xml:
	PYTHONPATH=../geopmdpy \
	    python3 -m geopmdpy.dbus_xml > $@ || \
	    echo "Warning: Could not import geopmdpy.dbus_xml, will not update io.github.geopm.xml"

VERSION:
	python3 -c "from setuptools_scm import get_version; print(get_version('..'))" > $@

DATE:
	date +'%a, %d %b %Y %H:%M:%S %z' > $@

dist: io.github.geopm.xml VERSION DATE
	VERSION=`cat VERSION` && DATE=`cat DATE` && \
	cat debian/changelog.in | sed -e "s|@VERSION@|$$VERSION|g" \
	                              -e "s|@DATE@|$$DATE|g" > debian/changelog && \
	sed -e "s|@VERSION@|$$VERSION|" geopm-systemd.spec.in > geopm-systemd.spec && \
	cd .. && tar --transform="s|systemd|geopm-systemd-$$VERSION|" -h -zcvf geopm-systemd-$$VERSION.tar.gz systemd && \
	mv geopm-systemd-$$VERSION.tar.gz systemd

.PHONY: VERSION io.github.geopm.xml

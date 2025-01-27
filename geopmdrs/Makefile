#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

all:
	# Do not make anything by default
	-

clean_all:
	rm -rf VERSION DATE

VERSION:
	python3 -c "from setuptools_scm import get_version; print(get_version('..'))" | sed -e 's|.dev.*||' > $@

DATE:
	date +'%a, %d %b %Y %H:%M:%S %z' > $@

MANIFEST:
	git ls-tree --full-tree --name-only -r HEAD geopmdrs > MANIFEST.partial
	echo geopmdrs/VERSION >> MANIFEST.partial
	echo geopmdrs/DATE >> MANIFEST.partial
	echo geopmdrs/debian/changelog >> MANIFEST.partial
	echo geopmdrs/Cargo.toml >> MANIFEST.partial
	echo geopmdrs/MANIFEST >> MANIFEST.partial
	sort MANIFEST.partial > $@
	rm MANIFEST.partial

dist: VERSION DATE MANIFEST
	VERSION=`cat VERSION` && DATE=`cat DATE` && \
	cat debian/changelog.in | sed -e "s|@VERSION@|$$VERSION|g" \
	                              -e "s|@DATE@|$$DATE|g" > debian/changelog && \
	cat Cargo.toml.in | sed -e "s|@VERSION@|$$VERSION|g" > Cargo.toml && \
	cd .. && tar -T geopmdrs/MANIFEST --transform="s|geopmdrs|geopmdrs-$$VERSION|" -h -zcvf geopmdrs_$$VERSION.orig.tar.gz

.PHONY: all clean_all DATE MANIFEST dist

#!/bin/bash
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

if [ $# -ne 3 ]; then
    echo "Usage: $0 top_srcdir ronn_file man_file"
    exit -1
fi

top_srcdir=$1
ronn_file=$2
man_file=$3

function replace_blurb {
    in_file=$1
    out_file=$2
    blurb_ln=$(grep -n '^@BLURB@$' $in_file | awk -F: '{print $1}')
    if [ -n "$blurb_ln" ]; then
        # Replace @BLURB@ if found
        head -n $(($blurb_ln - 1)) $in_file > $out_file
        cat $top_srcdir/BLURB >> $out_file
        tail -n +$(($blurb_ln + 1)) $in_file  >> $out_file
    else
        cp $in_file $out_file
    fi
}

if ! which ronn >& /dev/null; then
    echo "Warning: ronn required to create man pages with proper formatting" 1>&2
    echo "         try the following command: gem install ronn" 1>&2
    if [ -f $man_file ]; then
        echo "         man page already exists, will do nothing without ronn" 1>&2
    else
        echo "         copying markdown into man page without ronn" 1>&2
        set -e
        set -x
        tmp=$(mktemp /tmp/geopm-gen-man.XXXXXXXXXX)
        # Convert ronn comments
        sed 's|^\[//\]: #|.\\\"|' $ronn_file > $tmp
        replace_blurb $tmp $man_file
        rm $tmp
    fi
else
    set -e
    version=$(cat $top_srcdir/VERSION)
    date=$(date "+%Y-%m-%d")
    org="GEOPM $version"
    header_roff=$top_srcdir/ronn/header.roff
    header_html=$top_srcdir/ronn/header.html
    sedex_http='s|\(#include.*\) \\fIhttp.*|\1\\fR|g'

    tmp_ronn=$(mktemp /tmp/geopm-gen-man.XXXXXXXXXX)
    tmp_roff=$(mktemp /tmp/geopm-gen-man.XXXXXXXXXX)
    tmp_html=$(mktemp /tmp/geopm-gen-man.XXXXXXXXXX)
    all_tmp="$tmp_ronn $tmp_roff $tmp_html"

    set -x
    replace_blurb $ronn_file $tmp_ronn

    # generate roff add licence header and remove url from man page output
    ronn --date="$date" --organization="$org" --pipe --roff $tmp_ronn > $tmp_roff
    cat $header_roff $tmp_roff | sed -e "$sedex_http" > $man_file

    # generate html and add license header
    ronn --date="$date" --organization="$org" --pipe --html $tmp_ronn > $tmp_html
    cat $header_html $tmp_html > $man_file.html

    rm $all_tmp
fi

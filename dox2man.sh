#!/bin/bash
in=$1

lynx -width 1000 -dump $in | \
grep -A 100000 '^Constructor' | grep -B 100000 '______________________' | \
grep -v '____' | grep -v '^Constructor' | grep -v 'Member Function Documentation' | \
sed -e 's|\[[0-9]*\]||g; s|◆ \(.*\)()|  * `\1`():|g' | \
awk '/^  \* .*():$/ { empty_cc = 2 } ; /^$/ { if (empty_cc > 0) { print "```" ; empty_cc -= 1 } else { print } } ; /^..*$/'

#!/bin/bash
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

ret=0
if ! make && make checkprogs; then
    echo "Build failed"
    exit 1
fi

if [ $CC == "clang" ]; then
    export LD_LIBRARY_PATH="/usr/local/clang/lib:$LD_LIBRARY_PATH"
fi

echo "Running the Python 3 linter."
if ! scripts/test/check_python3_compatibility.sh > "$TRAVIS_BUILD_DIR/scripts/py3k_lint.log" 2>&1; then
    echo "The Python 3 linter failed."
    ret=1
fi

echo "Checking include guards in headers."
DIR="src/ test/"
for file in $(find $DIR -name "*.h" -o -name "*.hpp"); do
    guard=$(basename $file | sed "s|\.|_|g" | tr 'a-z' 'A-Z')_INCLUDE
    if [ $(grep -c $guard $file) -lt 2 ]; then
        echo "$file has missing or incorrect include guard"
        ret=1
    fi
done

# Exit early for linter checks
if [ $ret -ne 0 ]; then
    exit $ret
fi

echo "Running main unit tests..."
if ! make check; then
    echo "Main unit tests failed."
    ret=1
fi

pushd "$TRAVIS_BUILD_DIR/scripts"
export LD_LIBRARY_PATH="$TRAVIS_BUILD_DIR/.libs:$LD_LIBRARY_PATH"

export PYENV_VERSION=2.7
echo "Running Python 2 unit tests..."
if ! python setup.py test > py2_ut.log 2>&1; then
    echo "Python 2 unit tests failed."
    ret=1
fi

export PYENV_VERSION=3.6
echo "Running Python 3 unit tests..."
if ! python setup.py test > py3_ut.log 2>&1; then
    echo "Python 3 unit tests failed."
    ret=1
fi

unset PYENV_VERSION
popd

echo "Running test-dist..."
if ! ./copying_headers/test-dist; then
    echo "test-dist failed."
    ret=1
fi

exit $ret

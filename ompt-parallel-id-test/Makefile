#  Copyright (c) 2017, Intel Corporation
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

# Test fails with icc version 17.0.1 20161005
# LLVM omp version 4.0.0

all: check

openmp-4.0.0.src.tar.xz:
	wget http://releases.llvm.org/4.0.0/openmp-4.0.0.src.tar.xz

openmp-4.0.0.src/runtime/src/libomp.so: openmp-4.0.0.src.tar.xz
	tar xvf openmp-4.0.0.src.tar.xz
	cd openmp-4.0.0.src/runtime && cmake -DLIBOMP_OMPT_SUPPORT=on -DCMAKE_C_COMPILER=icc -DCMAKE_CXX_COMPILER=icpc && make

libomp.so: openmp-4.0.0.src/runtime/src/libomp.so
	cp openmp-4.0.0.src/runtime/src/libomp.so .

libiomp5.so: libomp.so
	ln -s $^ $@

ompt.h: libiomp5.so
	cp openmp-4.0.0.src/runtime/exports/common.ompt/include/ompt.h .

test-parallel-id: test-parallel-id.c ompt.h
	icc -fopenmp -I$(PWD) -L$(PWD) test-parallel-id.c -o $@
	echo [PASS]

check: test-parallel-id
	LD_LIBRARY_PATH=:$LD_LIBRARY_PATH ./test-parallel-id

clean:
	rm -f test-parallel-id

distclean:
	rm -f openmp-4.0.0.src.tar.xz
	rm -rf openmp-4.0.0.src
	rm -f libiomp5.so libiomp.so

.PHONY: all check clean distclean

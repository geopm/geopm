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

FORTRAN_LINKS = test/fortran/fortran_links/allctypesf90 \
                test/fortran/fortran_links/allocmemf90 \
                test/fortran/fortran_links/allpairf90 \
                test/fortran/fortran_links/allredint8f90 \
                test/fortran/fortran_links/allredopttf90 \
                test/fortran/fortran_links/alltoallvf90 \
                test/fortran/fortran_links/alltoallwf90 \
                test/fortran/fortran_links/atomicityf90 \
                test/fortran/fortran_links/attrmpi1f90 \
                test/fortran/fortran_links/baseattr2f90 \
                test/fortran/fortran_links/baseattr3f90 \
                test/fortran/fortran_links/baseattrf90 \
                test/fortran/fortran_links/baseattrwinf90 \
                test/fortran/fortran_links/baseenvf90 \
                test/fortran/fortran_links/c2f2cf90 \
                test/fortran/fortran_links/c2f2ciof90 \
                test/fortran/fortran_links/c2f2cwinf90 \
                test/fortran/fortran_links/cartcrf90 \
                test/fortran/fortran_links/commattr2f90 \
                test/fortran/fortran_links/commattr3f90 \
                test/fortran/fortran_links/commattrf90 \
                test/fortran/fortran_links/commerrf90 \
                test/fortran/fortran_links/commnamef90 \
                test/fortran/fortran_links/createf90 \
                test/fortran/fortran_links/dgraph_unwgtf90 \
                test/fortran/fortran_links/dgraph_wgtf90 \
                test/fortran/fortran_links/exscanf90 \
                test/fortran/fortran_links/fandcattrf90 \
                test/fortran/fortran_links/fileerrf90 \
                test/fortran/fortran_links/fileinfof90 \
                test/fortran/fortran_links/gaddressf90 \
                test/fortran/fortran_links/get_elem_d \
                test/fortran/fortran_links/get_elem_u \
                test/fortran/fortran_links/hindex1f90 \
                test/fortran/fortran_links/hindexed_blockf90 \
                test/fortran/fortran_links/indtype \
                test/fortran/fortran_links/infotest2f90 \
                test/fortran/fortran_links/infotestf90 \
                test/fortran/fortran_links/inplacef90 \
                test/fortran/fortran_links/kinds \
                test/fortran/fortran_links/miscfilef90 \
                test/fortran/fortran_links/nonblockingf90 \
                test/fortran/fortran_links/nonblocking_inpf90 \
                test/fortran/fortran_links/packef90 \
                test/fortran/fortran_links/profile1f90 \
                test/fortran/fortran_links/red_scat_blockf90 \
                test/fortran/fortran_links/redscatf90 \
                test/fortran/fortran_links/reducelocalf90 \
                test/fortran/fortran_links/setviewcurf90 \
                test/fortran/fortran_links/shpositionf90 \
                test/fortran/fortran_links/sizeof2 \
                test/fortran/fortran_links/sizeof \
                test/fortran/fortran_links/split_typef90 \
                test/fortran/fortran_links/statusesf90 \
                test/fortran/fortran_links/structf \
                test/fortran/fortran_links/trf90 \
                test/fortran/fortran_links/typeattr2f90 \
                test/fortran/fortran_links/typeattr3f90 \
                test/fortran/fortran_links/typeattrf90 \
                test/fortran/fortran_links/typecntsf90 \
                test/fortran/fortran_links/typem2f90 \
                test/fortran/fortran_links/typename3f90 \
                test/fortran/fortran_links/typenamef90 \
                test/fortran/fortran_links/typesnamef90 \
                test/fortran/fortran_links/typesubf90 \
                test/fortran/fortran_links/uallreducef90 \
                test/fortran/fortran_links/vw_inplacef90 \
                test/fortran/fortran_links/winaccf90 \
                test/fortran/fortran_links/winattr2f90 \
                test/fortran/fortran_links/winattrf90 \
                test/fortran/fortran_links/winerrf90 \
                test/fortran/fortran_links/winfencef90 \
                test/fortran/fortran_links/wingetf90 \
                test/fortran/fortran_links/wingroupf90 \
                test/fortran/fortran_links/winnamef90 \
                test/fortran/fortran_links/winscale1f90 \
                test/fortran/fortran_links/winscale2f90 \
                test/fortran/fortran_links/wtimef90 \
                test/fortran/fortran_links/greqf90 \
                test/fortran/fortran_links/spawnargvf03 \
                test/fortran/fortran_links/spawnargvf90 \
                test/fortran/fortran_links/spawnf90 \
                test/fortran/fortran_links/spawnmult2f90 \
                test/fortran/fortran_links/spawnmultf03 \
                test/fortran/fortran_links/spawnmultf90 \
                #end


FORTRAN_TESTS = test/fortran/allctypesf90 \
                test/fortran/allocmemf90 \
                test/fortran/allpairf90 \
                test/fortran/allredint8f90 \
                test/fortran/allredopttf90 \
                test/fortran/alltoallvf90 \
                test/fortran/alltoallwf90 \
                test/fortran/atomicityf90 \
                test/fortran/attrmpi1f90 \
                test/fortran/baseattr2f90 \
                test/fortran/baseattr3f90 \
                test/fortran/baseattrf90 \
                test/fortran/baseattrwinf90 \
                test/fortran/baseenvf90 \
                test/fortran/c2f2cf90 \
                test/fortran/c2f2ciof90 \
                test/fortran/c2f2cwinf90 \
                test/fortran/cartcrf90 \
                test/fortran/commattr2f90 \
                test/fortran/commattr3f90 \
                test/fortran/commattrf90 \
                test/fortran/commerrf90 \
                test/fortran/commnamef90 \
                test/fortran/createf90 \
                test/fortran/dgraph_unwgtf90 \
                test/fortran/dgraph_wgtf90 \
                test/fortran/exscanf90 \
                test/fortran/fandcattrf90 \
                test/fortran/fileerrf90 \
                test/fortran/fileinfof90 \
                test/fortran/gaddressf90 \
                test/fortran/get_elem_d \
                test/fortran/get_elem_u \
                test/fortran/hindex1f90 \
                test/fortran/hindexed_blockf90 \
                test/fortran/indtype \
                test/fortran/infotest2f90 \
                test/fortran/infotestf90 \
                test/fortran/inplacef90 \
                test/fortran/kinds \
                test/fortran/miscfilef90 \
                test/fortran/nonblockingf90 \
                test/fortran/nonblocking_inpf90 \
                test/fortran/packef90 \
                test/fortran/profile1f90 \
                test/fortran/red_scat_blockf90 \
                test/fortran/redscatf90 \
                test/fortran/reducelocalf90 \
                test/fortran/setviewcurf90 \
                test/fortran/shpositionf90 \
                test/fortran/sizeof2 \
                test/fortran/sizeof \
                test/fortran/spawnargvf03 \
                test/fortran/spawnargvf90 \
                test/fortran/spawnf90 \
                test/fortran/spawnmult2f90 \
                test/fortran/spawnmultf03 \
                test/fortran/spawnmultf90 \
                test/fortran/split_typef90 \
                test/fortran/statusesf90 \
                test/fortran/structf \
                test/fortran/trf90 \
                test/fortran/typeattr2f90 \
                test/fortran/typeattr3f90 \
                test/fortran/typeattrf90 \
                test/fortran/typecntsf90 \
                test/fortran/typem2f90 \
                test/fortran/typename3f90 \
                test/fortran/typenamef90 \
                test/fortran/typesnamef90 \
                test/fortran/typesubf90 \
                test/fortran/uallreducef90 \
                test/fortran/vw_inplacef90 \
                test/fortran/winaccf90 \
                test/fortran/winattr2f90 \
                test/fortran/winattrf90 \
                test/fortran/winerrf90 \
                test/fortran/winfencef90 \
                test/fortran/wingetf90 \
                test/fortran/wingroupf90 \
                test/fortran/winnamef90 \
                test/fortran/winscale1f90 \
                test/fortran/winscale2f90 \
                test/fortran/wtimef90 \
                test/fortran/greqf90 \
                #end

EXTRA_DIST += test/fortran/geopm_fortran_test.sh \
              #end

if ENABLE_MPI
if ENABLE_FORTRAN
    check_PROGRAMS += $(FORTRAN_TESTS)
    TESTS += $(FORTRAN_LINKS)

noinst_LTLIBRARIES += libmtest.la

FORTRAN_COMMON = test/fortran/mpitest.h \
                 test/fortran/mpitestconf.h \
                 test/fortran/c2f902cio.c \
                 test/fortran/c2f90multio.c \
                 test/fortran/checksizes.c \
                 test/fortran/createf90types.c \
                 test/fortran/c2f902cwin.c \
                 test/fortran/c2f902c.c \
                 test/fortran/c2f90mult.c \
                 test/fortran/attrlangc.c \
                 test/fortran/fandcattrc.c \
                 test/fortran/dummyf90.f90 \
                 test/fortran/mtestf90.f90 \
                 #end

# Target for building test programs.
fortran-checkprogs: $(FORTRAN_TESTS) $(FORTRAN_LINKS)

PHONY_TARGETS += fortran-checkprogs

$(FORTRAN_LINKS): test/fortran/fortran_links/%:
	mkdir -p test/fortran/fortran_links
	ln -s ../geopm_fortran_test.sh $@

clean-local-fortran-script-links:
	rm -f test/fortran/fortran_links/*

CLEAN_LOCAL_TARGETS += clean-local-fortran-script-links

libmtest_la_SOURCES = $(FORTRAN_COMMON)
libmtest_la_CPPFLAGS = $(AM_CPPFLAGS)
libmtest_la_CFLAGS = -c $(AM_CFLAGS) $(MPI_CFLAGS)
libmtest_la_LDFLAGS = $(AM_LDFLAGS) $(MPI_LDFLAGS)
libmtest_la_FCFLAGS = -c $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_allctypesf90_SOURCES = test/fortran/allctypesf90.f90
test_fortran_allctypesf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_allctypesf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_allocmemf90_SOURCES = test/fortran/allocmemf90.f90
test_fortran_allocmemf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
if HAVE_GFORTRAN
test_fortran_allocmemf90_FCFLAGS += -fcray-pointer
endif
test_fortran_allocmemf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_allpairf90_SOURCES = test/fortran/allpairf90.f90
test_fortran_allpairf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_allpairf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_allredint8f90_SOURCES = test/fortran/allredint8f90.f90
test_fortran_allredint8f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_allredint8f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_allredopttf90_SOURCES = test/fortran/allredopttf90.f90
test_fortran_allredopttf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_allredopttf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_alltoallvf90_SOURCES = test/fortran/alltoallvf90.f90
test_fortran_alltoallvf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_alltoallvf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_alltoallwf90_SOURCES = test/fortran/alltoallwf90.f90
test_fortran_alltoallwf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_alltoallwf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_atomicityf90_SOURCES = test/fortran/atomicityf90.f90
test_fortran_atomicityf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_atomicityf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_attrmpi1f90_SOURCES = test/fortran/attrmpi1f90.f90
test_fortran_attrmpi1f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_attrmpi1f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_baseattr2f90_SOURCES = test/fortran/baseattr2f90.f90
test_fortran_baseattr2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_baseattr2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_baseattr3f90_SOURCES = test/fortran/baseattr3f90.f90
test_fortran_baseattr3f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_baseattr3f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_baseattrf90_SOURCES = test/fortran/baseattrf90.f90
test_fortran_baseattrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_baseattrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_baseattrwinf90_SOURCES = test/fortran/baseattrwinf90.f90
test_fortran_baseattrwinf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_baseattrwinf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_baseenvf90_SOURCES = test/fortran/baseenvf90.f90
test_fortran_baseenvf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_baseenvf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_c2f2cf90_SOURCES = test/fortran/c2f2cf90.f90
test_fortran_c2f2cf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_c2f2cf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_c2f2ciof90_SOURCES = test/fortran/c2f2ciof90.f90
test_fortran_c2f2ciof90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_c2f2ciof90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_c2f2cwinf90_SOURCES = test/fortran/c2f2cwinf90.f90
test_fortran_c2f2cwinf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_c2f2cwinf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_cartcrf90_SOURCES = test/fortran/cartcrf90.f90
test_fortran_cartcrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_cartcrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_commattr2f90_SOURCES = test/fortran/commattr2f90.f90
test_fortran_commattr2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_commattr2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_commattr3f90_SOURCES = test/fortran/commattr3f90.f90
test_fortran_commattr3f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_commattr3f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_commattrf90_SOURCES = test/fortran/commattrf90.f90
test_fortran_commattrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_commattrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_commerrf90_SOURCES = test/fortran/commerrf90.f90
test_fortran_commerrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_commerrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_commnamef90_SOURCES = test/fortran/commnamef90.f90
test_fortran_commnamef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_commnamef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_createf90_SOURCES = test/fortran/createf90.f90
test_fortran_createf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_createf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_dgraph_unwgtf90_SOURCES = test/fortran/dgraph_unwgtf90.f90
test_fortran_dgraph_unwgtf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_dgraph_unwgtf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_dgraph_wgtf90_SOURCES = test/fortran/dgraph_wgtf90.f90
test_fortran_dgraph_wgtf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_dgraph_wgtf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_exscanf90_SOURCES = test/fortran/exscanf90.f90
test_fortran_exscanf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_exscanf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_fandcattrf90_SOURCES = test/fortran/fandcattrf90.f90
test_fortran_fandcattrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_fandcattrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_fileerrf90_SOURCES = test/fortran/fileerrf90.f90
test_fortran_fileerrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_fileerrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_fileinfof90_SOURCES = test/fortran/fileinfof90.f90
test_fortran_fileinfof90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_fileinfof90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_gaddressf90_SOURCES = test/fortran/gaddressf90.f90
test_fortran_gaddressf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_gaddressf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_get_elem_d_SOURCES = test/fortran/get_elem_d.f90
test_fortran_get_elem_d_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_get_elem_d_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_get_elem_u_SOURCES = test/fortran/get_elem_u.f90
test_fortran_get_elem_u_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_get_elem_u_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_greqf90_SOURCES = test/fortran/greqf90.f90
test_fortran_greqf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_greqf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_hindex1f90_SOURCES = test/fortran/hindex1f90.f90
test_fortran_hindex1f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_hindex1f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_hindexed_blockf90_SOURCES = test/fortran/hindexed_blockf90.f90
test_fortran_hindexed_blockf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_hindexed_blockf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_indtype_SOURCES = test/fortran/indtype.f90
test_fortran_indtype_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_indtype_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_infotest2f90_SOURCES = test/fortran/infotest2f90.f90
test_fortran_infotest2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_infotest2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_infotestf90_SOURCES = test/fortran/infotestf90.f90
test_fortran_infotestf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_infotestf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_inplacef90_SOURCES = test/fortran/inplacef90.f90
test_fortran_inplacef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_inplacef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_kinds_SOURCES = test/fortran/kinds.f90
test_fortran_kinds_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_kinds_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_miscfilef90_SOURCES = test/fortran/miscfilef90.f90
test_fortran_miscfilef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_miscfilef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_nonblockingf90_SOURCES = test/fortran/nonblockingf90.f90
test_fortran_nonblockingf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_nonblockingf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_nonblocking_inpf90_SOURCES = test/fortran/nonblocking_inpf90.f90
test_fortran_nonblocking_inpf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_nonblocking_inpf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_packef90_SOURCES = test/fortran/packef90.f90
test_fortran_packef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_packef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_profile1f90_SOURCES = test/fortran/profile1f90.f90
test_fortran_profile1f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_profile1f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_red_scat_blockf90_SOURCES = test/fortran/red_scat_blockf90.f90
test_fortran_red_scat_blockf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_red_scat_blockf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_redscatf90_SOURCES = test/fortran/redscatf90.f90
test_fortran_redscatf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_redscatf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_reducelocalf90_SOURCES = test/fortran/reducelocalf90.f90
test_fortran_reducelocalf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_reducelocalf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_setviewcurf90_SOURCES = test/fortran/setviewcurf90.f90
test_fortran_setviewcurf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_setviewcurf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_shpositionf90_SOURCES = test/fortran/shpositionf90.f90
test_fortran_shpositionf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_shpositionf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_sizeof2_SOURCES = test/fortran/sizeof2.f90
test_fortran_sizeof2_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_sizeof2_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_sizeof_SOURCES = test/fortran/sizeof.f90
test_fortran_sizeof_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_sizeof_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_spawnargvf03_SOURCES = test/fortran/spawnargvf03.f90
test_fortran_spawnargvf03_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_spawnargvf03_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_spawnargvf90_SOURCES = test/fortran/spawnargvf90.f90
test_fortran_spawnargvf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_spawnargvf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_spawnf90_SOURCES = test/fortran/spawnf90.f90
test_fortran_spawnf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_spawnf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_spawnmult2f90_SOURCES = test/fortran/spawnmult2f90.f90
test_fortran_spawnmult2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_spawnmult2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_spawnmultf03_SOURCES = test/fortran/spawnmultf03.f90
test_fortran_spawnmultf03_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_spawnmultf03_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_spawnmultf90_SOURCES = test/fortran/spawnmultf90.f90
test_fortran_spawnmultf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_spawnmultf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_split_typef90_SOURCES = test/fortran/split_typef90.f90
test_fortran_split_typef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_split_typef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_statusesf90_SOURCES = test/fortran/statusesf90.f90
test_fortran_statusesf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_statusesf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_structf_SOURCES = test/fortran/structf.f90
test_fortran_structf_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_structf_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_trf90_SOURCES = test/fortran/trf90.f90
test_fortran_trf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_trf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typeattr2f90_SOURCES = test/fortran/typeattr2f90.f90
test_fortran_typeattr2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typeattr2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typeattr3f90_SOURCES = test/fortran/typeattr3f90.f90
test_fortran_typeattr3f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typeattr3f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typeattrf90_SOURCES = test/fortran/typeattrf90.f90
test_fortran_typeattrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typeattrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typecntsf90_SOURCES = test/fortran/typecntsf90.f90
test_fortran_typecntsf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typecntsf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typem2f90_SOURCES = test/fortran/typem2f90.f90
test_fortran_typem2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typem2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typename3f90_SOURCES = test/fortran/typename3f90.f90
test_fortran_typename3f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typename3f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typenamef90_SOURCES = test/fortran/typenamef90.f90
test_fortran_typenamef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typenamef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typesnamef90_SOURCES = test/fortran/typesnamef90.f90
test_fortran_typesnamef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typesnamef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_typesubf90_SOURCES = test/fortran/typesubf90.f90
test_fortran_typesubf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_typesubf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_uallreducef90_SOURCES = test/fortran/uallreducef90.f90
test_fortran_uallreducef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_uallreducef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_vw_inplacef90_SOURCES = test/fortran/vw_inplacef90.f90
test_fortran_vw_inplacef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_vw_inplacef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winaccf90_SOURCES = test/fortran/winaccf90.f90
test_fortran_winaccf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winaccf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winattr2f90_SOURCES = test/fortran/winattr2f90.f90
test_fortran_winattr2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winattr2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winattrf90_SOURCES = test/fortran/winattrf90.f90
test_fortran_winattrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winattrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winerrf90_SOURCES = test/fortran/winerrf90.f90
test_fortran_winerrf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winerrf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winfencef90_SOURCES = test/fortran/winfencef90.f90
test_fortran_winfencef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winfencef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_wingetf90_SOURCES = test/fortran/wingetf90.f90
test_fortran_wingetf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_wingetf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_wingroupf90_SOURCES = test/fortran/wingroupf90.f90
test_fortran_wingroupf90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_wingroupf90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winnamef90_SOURCES = test/fortran/winnamef90.f90
test_fortran_winnamef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winnamef90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winscale1f90_SOURCES = test/fortran/winscale1f90.f90
test_fortran_winscale1f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winscale1f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_winscale2f90_SOURCES = test/fortran/winscale2f90.f90
test_fortran_winscale2f90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_winscale2f90_LDADD = libmtest.la $(MPI_FLIBS)
test_fortran_wtimef90_SOURCES = test/fortran/wtimef90.f90
test_fortran_wtimef90_FCFLAGS = $(AM_FCFLAGS) $(FCFLAGS) $(MPI_FCFLAGS)
test_fortran_wtimef90_LDADD = libmtest.la $(MPI_FLIBS)
else
# Target for building test programs.
fortran-checkprogs:
endif
else
# Target for building test programs.
fortran-checkprogs:
endif

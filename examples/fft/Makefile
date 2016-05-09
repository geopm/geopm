#-------------------------------------------------------------------------#
#                                                                         #
#        N  A  S     P A R A L L E L     B E N C H M A R K S  3.3         #
#                                                                         #
#                                   F T                                   #
#                                                                         #
#-------------------------------------------------------------------------#
#                                                                         #
#    This benchmark is part of the NAS Parallel Benchmark 3.3 suite.      #
#    It is described in NAS Technical Reports 95-020 and 02-007           #
#                                                                         #
#    Permission to use, copy, distribute and modify this software         #
#    for any purpose with or without fee is hereby granted.  We           #
#    request, however, that all derived work reference the NAS            #
#    Parallel Benchmarks 3.3. This software is provided "as is"           #
#    without express or implied warranty.                                 #
#                                                                         #
#    Information on NPB 3.3, including the technical report, the          #
#    original specifications, source code, results and information        #
#    on how to submit new results, is available at:                       #
#                                                                         #
#           http://www.nas.nasa.gov/Software/NPB/                         #
#                                                                         #
#    Send comments or suggestions to  npb@nas.nasa.gov                    #
#                                                                         #
#          NAS Parallel Benchmarks Group                                  #
#          NASA Ames Research Center                                      #
#          Mail Stop: T27A-1                                              #
#          Moffett Field, CA   94035-1000                                 #
#                                                                         #
#          E-mail:  npb@nas.nasa.gov                                      #
#          Fax:     (650) 604-3957                                        #
#                                                                         #
#-------------------------------------------------------------------------#

SHELL=/bin/sh

ifndef CLASS
   CLASS = C
endif

#include make.def

#include make.common
FC = mpifort
# Options for icc
#CC = icc
#FCFLAGS = -g -O3 -openmp -xAVX -shared-intel -mcmodel=medium -fpic -I/home/cmcantal/build/geopm/lib/ifort/geopm-x86_64
# Options for gcc
CC = gcc
FCFLAGS = -g -O3 -fopenmp -fpic -I${HOME}/build/geopm/lib/gfortran/modules/geopm-x86_64
OBJS = randi8.o print_results.o timers.o
PROGRAM = ft.${CLASS}

${PROGRAM}: print_header ${OBJS}
	${FC} ${FCFLAGS} -o ${PROGRAM} ${OBJS} ft.o -L${HOME}/build/geopm/lib -lgeopmfort -lgeopm

checksum: print_header_checksum ${OBJS}
	${FC} ${FCFLAGS} -o ${PROGRAM}.checksum ${OBJS}

print_header: ft.f90 global.f90 mpinpb.h setparams
	@echo
	@echo 'Building ${PROGRAM} with CLASS=${CLASS}'
	@echo 'Use: make CLASS=<class> to overwrite default value C'
	@echo
	rm -f npbparams.h ft.o
	./setparams ft 1 ${CLASS}
	${FC} -c ${FCFLAGS} ft.f90 -o ft.o

print_header_checksum: ft.f90 global.f90 mpinpb.h setparams
	@echo
	@echo 'Building ${PROGRAM} with CLASS=${CLASS} (with checksums)'
	@echo 'Use: make CLASS=<class> to overwrite default value C'
	@echo
	rm -f npbparams.h ft.o
	./setparams ft 1 ${CLASS}
	${FC} -c -DCHECKSUM ${FCFLAGS} ft.f90

.F.o:
	${FC} -c ${FCFLAGS} $<

.f90.o:
	${FC} -c ${FCFLAGS} $<

# setparams creates an npbparam.h file for each benchmark
# configuration. npbparams.h also contains info about how a benchmark
# was compiled and linked

setparams: setparams.c
	$(CC) -o setparams setparams.c

clean:
	- rm -f *.o npbparams.h ft.C ft.C.checksum setparams

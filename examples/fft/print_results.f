!-------------------------------------------------------------------------!
!                                                                         !
!        N  A  S     P A R A L L E L     B E N C H M A R K S  3.3         !
!                                                                         !
!                                   F T                                   !
!                                                                         !
!-------------------------------------------------------------------------!
!                                                                         !
!    This benchmark is part of the NAS Parallel Benchmark 3.3 suite.      !
!    It is described in NAS Technical Reports 95-020 and 02-007           !
!                                                                         !
!    Permission to use, copy, distribute and modify this software         !
!    for any purpose with or without fee is hereby granted.  We           !
!    request, however, that all derived work reference the NAS            !
!    Parallel Benchmarks 3.3. This software is provided "as is"           !
!    without express or implied warranty.                                 !
!                                                                         !
!    Information on NPB 3.3, including the technical report, the          !
!    original specifications, source code, results and information        !
!    on how to submit new results, is available at:                       !
!                                                                         !
!           http://www.nas.nasa.gov/Software/NPB/                         !
!                                                                         !
!    Send comments or suggestions to  npb@nas.nasa.gov                    !
!                                                                         !
!          NAS Parallel Benchmarks Group                                  !
!          NASA Ames Research Center                                      !
!          Mail Stop: T27A-1                                              !
!          Moffett Field, CA   94035-1000                                 !
!                                                                         !
!          E-mail:  npb@nas.nasa.gov                                      !
!          Fax:     (650) 604-3957                                        !
!                                                                         !
!-------------------------------------------------------------------------!


      subroutine print_results(name, class, n1, n2, n3, niter,
     >               nprocs_compiled, nprocs_total,
     >               t, mops, optype, verified, npbversion,
     >               compiletime, cs1, cs2, cs3, cs4, cs5, cs6, cs7)

      implicit none
      character*2 name
      character*1 class
      integer n1, n2, n3, niter, nprocs_compiled, nprocs_total, j
      double precision t, mops
      character optype*24, size*15
      logical verified
      character*(*) npbversion, compiletime,
     >              cs1, cs2, cs3, cs4, cs5, cs6, cs7

         write (*, 2) name
 2       format(//, ' ', A2, ' Benchmark Completed.')

         write (*, 3) Class
 3       format(' Class           = ', 12x, a12)

c   If this is not a grid-based problem (EP, FT, CG), then
c   we only print n1, which contains some measure of the
c   problem size. In that case, n2 and n3 are both zero.
c   Otherwise, we print the grid size n1xn2xn3

         if ((n2 .eq. 0) .and. (n3 .eq. 0)) then
            if (name(1:2) .eq. 'EP') then
               write(size, '(f15.0)' ) 2.d0**n1
               j = 15
               if (size(j:j) .eq. '.') j = j - 1
               write (*,42) size(1:j)
 42            format(' Size            = ',9x, a15)
            else
               write (*,44) n1
 44            format(' Size            = ',12x, i12)
            endif
         else
            write (*, 4) n1,n2,n3
 4          format(' Size            =  ',9x, i4,'x',i4,'x',i4)
         endif

         write (*, 5) niter
 5       format(' Iterations      = ', 12x, i12)

         write (*, 6) t
 6       format(' Time in seconds = ',12x, f12.2)

         write (*,7) nprocs_total
 7       format(' Total processes = ', 12x, i12)

         write (*,8) nprocs_compiled
 8       format(' Compiled procs  = ', 12x, i12)

         write (*,9) mops
 9       format(' Mop/s total     = ',12x, f12.2)

         write (*,10) mops/float( nprocs_total )
 10      format(' Mop/s/process   = ', 12x, f12.2)

         write(*, 11) optype
 11      format(' Operation type  = ', a24)

         if (verified) then
            write(*,12) '  SUCCESSFUL'
         else
            write(*,12) 'UNSUCCESSFUL'
         endif
 12      format(' Verification    = ', 12x, a)

         write(*,13) npbversion
 13      format(' Version         = ', 12x, a12)

c         write(*,14) compiletime
c 14      format(' Compile date    = ', 12x, a12)
c
c
c         write (*,121) cs1
c 121     format(/, ' Compile options:', /,
c     >          '    MPIF77       = ', A)
c
c         write (*,122) cs2
c 122     format('    FLINK        = ', A)
c
c         write (*,123) cs3
c 123     format('    FMPI_LIB     = ', A)
c
c         write (*,124) cs4
c 124     format('    FMPI_INC     = ', A)
c
c         write (*,125) cs5
c 125     format('    FFLAGS       = ', A)
c
c         write (*,126) cs6
c 126     format('    FLINKFLAGS   = ', A)
c
c         write(*, 127) cs7
c 127     format('    RAND         = ', A)

         write (*,130)
 130     format(//' Please send feedbacks and/or'
     >            ' the results of this run to:'//
     >            ' NPB Development Team '/
     >            ' Internet: npb@nas.nasa.gov'//)
c 130     format(//' Please send the results of this run to:'//
c     >            ' NPB Development Team '/
c     >            ' Internet: npb@nas.nasa.gov'/
c     >            ' '/
c     >            ' If email is not available, send this to:'//
c     >            ' MS T27A-1'/
c     >            ' NASA Ames Research Center'/
c     >            ' Moffett Field, CA  94035-1000'//
c     >            ' Fax: 650-604-3957'//)


         return
         end


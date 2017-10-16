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

!TO REDUCE THE AMOUNT OF MEMORY REQUIRED BY THE BENCHMARK WE NO LONGER
!STORE THE ENTIRE TIME EVOLUTION ARRAY "EX" FOR ALL TIME STEPS, BUT
!JUST FOR THE FIRST. ALSO, IT IS STORED ONLY FOR THE PART OF THE GRID
!FOR WHICH THE CALLING PROCESSOR IS RESPONSIBLE, SO THAT THE MEMORY
!USAGE BECOMES SCALABLE. THIS NEW ARRAY IS CALLED "TWIDDLE" (SEE
!NPB3.0-SER)

!TO AVOID PROBLEMS WITH VERY LARGE ARRAY SIZES THAT ARE COMPUTED BY
!MULTIPLYING GRID DIMENSIONS (CAUSING INTEGER OVERFLOW IN THE VARIABLE
!NTOTAL) AND SUBSEQUENTLY DIVIDING BY THE NUMBER OF PROCESSORS, WE
!COMPUTE THE SIZE OF ARRAY PARTITIONS MORE CONSERVATIVELY AS
!((NX*NY)/NP)*NZ, WHERE NX, NY, AND NZ ARE GRID DIMENSIONS AND NP IS
!THE NUMBER OF PROCESSORS, THE RESULT IS STORED IN "NTDIVNP". FOR THE
!PERFORMANCE CALCULATION WE STORE THE TOTAL NUMBER OF GRID POINTS IN A
!FLOATING POINT NUMBER "NTOTAL_F" INSTEAD OF AN INTEGER.
!THIS FIX WILL FAIL IF THE NUMBER OF PROCESSORS IS SMALL.

!UGLY HACK OF SUBROUTINE IPOW46: FOR VERY LARGE GRIDS THE SINGLE EXPONENT
!FROM NPB2.3 MAY NOT FIT IN A 32-BIT INTEGER. HOWEVER, WE KNOW THAT THE
!"EXPONENT" ARGUMENT OF THIS ROUTINE CAN ALWAYS BE FACTORED INTO A TERM
!DIVISIBLE BY NX (EXP_1) AND ANOTHER TERM (EXP_2). NX IS USUALLY A POWER
!OF TWO, SO WE CAN KEEP HALVING IT UNTIL THE PRODUCT OF EXP_1
!AND EXP_2 IS SMALL ENOUGH (NAMELY EXP_2 ITSELF). THIS UPDATED VERSION
!OF IPWO46, WHICH NOW TAKES THE TWO FACTORS OF "EXPONENT" AS SEPARATE
!ARGUMENTS, MAY BREAK DOWN IF EXP_1 DOES NOT CONTAIN A LARGE POWER OF TWO.

!---------------------------------------------------------------------
!
! Authors: D. Bailey
!          W. Saphir
!          R. F. Van der Wijngaart
!
!---------------------------------------------------------------------

!---------------------------------------------------------------------


!---------------------------------------------------------------------
! FT benchmark
!---------------------------------------------------------------------

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      program ft
      use geopm
      use, intrinsic :: ISO_C_BINDING


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none

      include 'mpif.h'
      include 'global.fi'
      integer i, ierr

!---------------------------------------------------------------------
! u0, u1, u2 are the main arrays in the problem.
! Depending on the decomposition, these arrays will have different
! dimensions. To accomodate all possibilities, we allocate them as
! one-dimensional arrays and pass them to subroutines for different
! views
!  - u0 contains the initial (transformed) initial condition
!  - u1 and u2 are working arrays
!---------------------------------------------------------------------

      double complex, dimension(:), allocatable :: u0, &
                      u1, &
                      u2
      double precision, dimension(:), allocatable :: twiddle
!---------------------------------------------------------------------
! Large arrays are in common so that they are allocated on the
! heap rather than the stack. This common block is not
! referenced directly anywhere else. Padding is to avoid accidental
! cache problems, since all array sizes are powers of two.
!---------------------------------------------------------------------

      double complex pad1(3), pad2(3), pad3(3)
!      common /bigarrays/ u0, pad1, u1, pad2, u2, pad3, twiddle

      integer iter, mynode, my_rank, comm_size
      integer numnodes, mylen
      double precision total_time, mflops
      logical verified
      character class
      integer ntdivnp_i

      integer  threads, num_ranks, nc, j, err
      logical haveit, looking
      character (len=MPI_MAX_PROCESSOR_NAME):: myname
      character (len=MPI_MAX_PROCESSOR_NAME), &
                allocatable :: nodes(:),procs(:)

      call MPI_Init(ierr)

      pswitch = 0

!---------------------------------------------------------------------
! Run the entire problem once to make sure all data is touched.
! This reduces variable startup costs, which is important for such a
! short benchmark. The other NPB 2 implementations are similar.
!---------------------------------------------------------------------
      do i = 1, t_max
         call timer_clear(i)
      end do

      call timer_start(T_init)
      call setup()

! Jonathan: register phases with the auto-tuner
! Need to do an MPI barrier to make sure all ranks have completed
! at_init
!      call MPI_Barrier(MPI_COMM_WORLD, ierr)
!      call at_get_num_connections(num_ranks)
!      call MPI_Comm_rank(MPI_COMM_WORLD, my_rank, ierr)
!      call MPI_Comm_size(MPI_COMM_WORLD, comm_size, ierr)
! If we are on multiple nodes then create per node communicator
! so that we can have per node barriers after phase exit
! We will make a list of node names by gathering names from all
! ranks. Then we look up which one we are and use that as or color
! to MPI_Comm_split
!      if (num_ranks /= comm_size) then
!        call MPI_Get_processor_name(myname,mylen,ierr)
!        numnodes = comm_size / num_ranks
!        allocate(procs(comm_size))
!        allocate(nodes(numnodes))
!        call MPI_Allgather(myname, MPI_MAX_PROCESSOR_NAME,
!     >                     MPI_CHARACTER, procs, MPI_MAX_PROCESSOR_NAME,
!     >                     MPI_CHARACTER, MPI_COMM_WORLD,ierr)
! Lets create a list of unique name entries
!        nodes(1)=procs(1)
!        nc=1
!        do i=2,comm_size
!          haveit=.false.
!          do j=1,nc
!            if(nodes(j) .eq. procs(i))haveit=.true.
!          enddo
!          if(haveit .eq. .false.)then
!            nc=nc+1
!            nodes(nc)=procs(i)
!          endif
!        enddo

! Now let's see which node we belong to
!        mynode=0
!        looking=.true.
!        do while (looking)
!          mynode=mynode+1
!          if(nodes(mynode) .eq. myname)looking=.false.
!        enddo

!        call MPI_Comm_split(MPI_COMM_WORLD, mynode, 0, node_comm, ierr)
!        call MPI_Comm_size(node_comm, num_ranks, ierr)
!      else
!        node_comm = MPI_COMM_WORLD;
!      endif
!      threads = num_ranks * omp_get_max_threads()
! Jonathan

! Ilya: dynamic memory allocation
      ntdivnp_i=((nx*ny)/np)*nz
      allocate (u0(ntdivnp_i))
      allocate (u1(ntdivnp_i))
      allocate (u2(ntdivnp_i))
      allocate (twiddle(ntdivnp_i))
      call compute_indexmap(twiddle, dims(1,3), dims(2,3), dims(3,3))
      call compute_initial_conditions(u1, dims(1,1), dims(2,1), &
                                     dims(3,1))
      call fft_init (dims(1,1))
      call fft(1, u1, u0, ntdivnp_i)
      call timer_stop(T_init)
      if (me .eq. 0) then
         print *,'Initialization time =',timer_read(T_init)
      endif

!---------------------------------------------------------------------
! Start over from the beginning. Note that all operations must
! be timed, in contrast to other benchmarks.
!---------------------------------------------------------------------
      do i = 1, t_max
         call timer_clear(i)
      end do
      call MPI_Barrier(MPI_COMM_WORLD, ierr)

      call timer_start(T_total)
      if (timers_enabled) call timer_start(T_setup)

      call compute_indexmap(twiddle, dims(1,3), dims(2,3), dims(3,3))
      call compute_initial_conditions(u1, dims(1,1), dims(2,1), &
                                     dims(3,1))
      call fft_init (dims(1,1))

!      if (timers_enabled) call synchup()
      if (timers_enabled) call timer_stop(T_setup)

      if (timers_enabled) call timer_start(T_fft)
      call fft(1, u1, u0, ntdivnp_i)
      if (timers_enabled) call timer_stop(T_fft)

      do iter = 1, niter
         ierr = geopm_prof_epoch()
         if (timers_enabled) call timer_start(T_evolve)
         call evolve(u0, u1, twiddle, dims(1,1), dims(2,1), dims(3,1))
         if (timers_enabled) call timer_stop(T_evolve)
         if (timers_enabled) call timer_start(T_fft)
         call fft(-1, u1, u2, ntdivnp_i)
         if (timers_enabled) call timer_stop(T_fft)
!         if (timers_enabled) call synchup()
!#IFDEF CHECKSUM
!         if (timers_enabled) call timer_start(T_checksum)
!         call checksum(iter, u2, dims(1,1), dims(2,1), dims(3,1))
!         if (timers_enabled) call timer_stop(T_checksum)
!#ELSE
         if (me .eq. 0) then
           write (*, 30) iter
 30        format (' Iteration: ',I5)
         endif
!#ENDIF
      end do

      call verify(nx, ny, nz, niter, verified, class)
      call timer_stop(t_total)
!      if (np .ne. np_min) verified = .false.
      total_time = timer_read(t_total)

      if( total_time .ne. 0. ) then
         mflops = 1.0d-6*ntotal_f * &
                  (14.8157+7.19641*log(ntotal_f) &
               +  (5.23518+7.21113*log(ntotal_f))*niter) &
                      /total_time
      else
         mflops = 0.0
      endif
      if (me .eq. 0) then
         call print_results('FT', class, nx, ny, nz, niter, np, np, &
          total_time, mflops, '          floating point', verified, &
          npbversion, compiletime, cs1, cs2, cs3, cs4, cs5, cs6, cs7)
      endif
      if (timers_enabled) call print_timers()
      call MPI_Finalize(ierr)
      end

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine evolve(u0, u1, twiddle, d1, d2, d3)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
! evolve u0 -> u1 (t time steps) in fourier space
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'global.fi'
      include 'mpif.h'
      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer d1, d2, d3
      double precision exi
      double complex u0(d1,d2,d3)
      double complex u1(d1,d2,d3)
      double precision twiddle(d1,d2,d3)
      integer i, j, k, tid, ierr

      integer(8) region_id
      integer(8) num_it
      integer(8) chunk_size
      integer num_thread
!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel
      num_it = d3
      chunk_size = num_it / num_thread
      if (chunk_size .eq. 0) then
          chunk_size = 1
      end if
      ierr = geopm_prof_region(c_char_'AT_PHASE_EVOLVE'//c_null_char, &
                               GEOPM_REGION_HINT_MEMORY, region_id)
      ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,k,tid)
      tid = omp_get_thread_num()
      ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
      do k = 1, d3
         do j = 1, d2
            do i = 1, d1
               u0(i,j,k) = u0(i,j,k)*(twiddle(i,j,k))
               u1(i,j,k) = u0(i,j,k)
            end do
         end do
         ierr = geopm_tprof_post()
      end do
!$omp end do
!$omp end parallel
      ierr = geopm_prof_exit(region_id)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine compute_initial_conditions(u0, d1, d2, d3)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
! Fill in array u0 with initial conditions from
! random number generator
!---------------------------------------------------------------------
      implicit none
      include 'global.fi'
      integer d1, d2, d3
      double complex u0(d1, d2, d3)
      integer k
      double precision x0, start, an, dummy

!---------------------------------------------------------------------
! 0-D and 1-D layouts are easy because each processor gets a contiguous
! chunk of the array, in the Fortran ordering sense.
! For a 2-D layout, it's a bit more complicated. We always
! have entire x-lines (contiguous) in processor.
! We can do ny/np1 of them at a time since we have
! ny/np1 contiguous in y-direction. But then we jump
! by z-planes (nz/np2 of them, total).
! For the 0-D and 1-D layouts we could do larger chunks, but
! this turns out to have no measurable impact on performance.
!---------------------------------------------------------------------


      start = seed
!---------------------------------------------------------------------
! Jump to the starting element for our first plane.
!---------------------------------------------------------------------
      call ipow46(a, 2*nx, (zstart(1)-1)*ny + (ystart(1)-1), an)
      dummy = randlc(start, an)
      call ipow46(a, 2*nx, ny, an)

!---------------------------------------------------------------------
! Go through by z planes filling in one square at a time.
!---------------------------------------------------------------------
      do k = 1, dims(3, 1) ! nz/np2
         x0 = start
         call vranlc(2*nx*dims(2, 1), x0, a, u0(1, 1, k))
         if (k .ne. dims(3, 1)) dummy = randlc(start, an)
      end do

      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine ipow46(a, exp_1, exp_2, result)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
! compute a^exponent mod 2^46
!---------------------------------------------------------------------

      implicit none
      double precision a, result, dummy, q, r
      integer exp_1, exp_2, n, n2, ierr
      external randlc
      double precision randlc
      logical  two_pow
!---------------------------------------------------------------------
! Use
!   a^n = a^(n/2)*a^(n/2) if n even else
!   a^n = a*a^(n-1)       if n odd
!---------------------------------------------------------------------
      result = 1
      if (exp_2 .eq. 0 .or. exp_1 .eq. 0) return
      q = a
      r = 1
      n = exp_1
      two_pow = .true.

      do while (two_pow)
         n2 = n/2
         if (n2 * 2 .eq. n) then
            dummy = randlc(q, q)
            n = n2
         else
            n = n * exp_2
            two_pow = .false.
         endif
      end do

      do while (n .gt. 1)
         n2 = n/2
         if (n2 * 2 .eq. n) then
            dummy = randlc(q, q)
            n = n2
         else
            dummy = randlc(r, q)
            n = n-1
         endif
      end do
      dummy = randlc(r, q)
      result = r
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine setup

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'mpinpb.fi'
      include 'global.fi'

      integer ierr, i, j, fstatus

      debug = .FALSE.
      fstatus = 1

      call MPI_Comm_size(MPI_COMM_WORLD, np, ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, me, ierr)

      if (.not. convertdouble) then
         dc_type = MPI_DOUBLE_COMPLEX
      else
         dc_type = MPI_COMPLEX
      endif

      fstatus = 1;
      if (me .eq. 0) then
         write(*, 1000)

!         open (unit=2,file='timer.flag',status='old',iostat=fstatus)
         timers_enabled = .false.
         if (fstatus .eq. 0) then
            timers_enabled = .true.
            close(2)
         endif

!         open (unit=2,file='inputft.data',status='old', iostat=fstatus)

         if (fstatus .eq. 0) then
            write(*,233)
 233        format(' Reading from input file inputft.data')
            read (2,*) niter
            read (2,*) layout_type
            read (2,*) np1, np2
            close(2)

!---------------------------------------------------------------------
! check to make sure input data is consistent
!---------------------------------------------------------------------


!---------------------------------------------------------------------
! 1. product of processor grid dims must equal number of processors
!---------------------------------------------------------------------

            if (np1 * np2 .ne. np) then
               write(*, 238)
 238           format(' np1 and np2 given in input file are not valid.')
               write(*, 239) np1*np2, np
 239           format(' Product is ', i5, ' and should be ', i5)
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif

!---------------------------------------------------------------------
! 2. layout type must be valid
!---------------------------------------------------------------------

            if (layout_type .ne. layout_0D .and. &
               layout_type .ne. layout_1D .and. &
               layout_type .ne. layout_2D) then
               write(*, 240)
 240           format(' Layout type specified in inputft.data is &
                       invalid ')
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif

!---------------------------------------------------------------------
! 3. 0D layout must be 1x1 grid
!---------------------------------------------------------------------

            if (layout_type .eq. layout_0D .and. &
                 (np1 .ne.1 .or. np2 .ne. 1)) then
               write(*, 241)
 241           format(' For 0D layout, both np1 and np2 must be 1 ')
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif
!---------------------------------------------------------------------
! 4. 1D layout must be 1xN grid
!---------------------------------------------------------------------

            if (layout_type .eq. layout_1D .and. np1 .ne. 1) then
               write(*, 242)
 242           format(' For 1D layout, np1 must be 1 ')
               call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
            endif

         else
            write(*,234)
            niter = niter_default
            if (np .eq. 1) then
               np1 = 1
               np2 = 1
               layout_type = layout_0D
            else if (np .le. nz) then
               np1 = 1
               np2 = np
               layout_type = layout_1D
            else
               np1 = nz
               np2 = np/nz
               layout_type = layout_2D
            endif
         endif

! Ilya: not needed with dynamic memory allocation
!         if (np .lt. np_min) then
!            write(*, 10) np_min
! 10         format(' Error: Compiled for ', I5, ' processors. ')
!            write(*, 11) np
! 11         format(' Only ',  i5, ' processors found ')
!            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
!         endif

 234     format(' No input file inputft.data. Using compiled defaults')
         write(*, 1001) nx, ny, nz
         write(*, 1002) niter
         write(*, 1004) np
         write(*, 1005) np1, np2
!         if (np .ne. np_min) write(*, 1006) np_min

         if (layout_type .eq. layout_0D) then
            write(*, 1010) '0D'
         else if (layout_type .eq. layout_1D) then
            write(*, 1010) '1D'
         else
            write(*, 1010) '2D'
         endif

 1000 format(//,' NAS Parallel Benchmarks 3.3 -- FT Benchmark',/)
 1001    format(' Size                : ', i4, 'x', i4, 'x', i4)
 1002    format(' Iterations          : ', 7x, i7)
 1004    format(' Number of processes : ', 7x, i7)
 1005    format(' Processor array     : ', 5x, i4, 'x', i4)
 1006    format(' WARNING: compiled for ', i5, ' processes. ', &
               ' Will not verify. ')
 1010    format(' Layout type         : ', 9x, A5)
      endif


!---------------------------------------------------------------------
! Broadcast parameters
!---------------------------------------------------------------------
      call MPI_BCAST(np1, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
      call MPI_BCAST(np2, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
      call MPI_BCAST(layout_type, 1, MPI_INTEGER, 0, &
                     MPI_COMM_WORLD, ierr)
      call MPI_BCAST(niter, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
      call MPI_BCAST(timers_enabled, 1, MPI_LOGICAL, 0,  &
                     MPI_COMM_WORLD, ierr)

      if (np1 .eq. 1 .and. np2 .eq. 1) then
        layout_type = layout_0D
      else if (np1 .eq. 1) then
         layout_type = layout_1D
      else
         layout_type = layout_2D
      endif

      if (layout_type .eq. layout_0D) then
         do i = 1, 3
            dims(1, i) = nx
            dims(2, i) = ny
            dims(3, i) = nz
         end do
      else if (layout_type .eq. layout_1D) then
         dims(1, 1) = nx
         dims(2, 1) = ny
         dims(3, 1) = nz

         dims(1, 2) = nx
         dims(2, 2) = ny
         dims(3, 2) = nz

         dims(1, 3) = nz
         dims(2, 3) = nx
         dims(3, 3) = ny
      else if (layout_type .eq. layout_2D) then
         dims(1, 1) = nx
         dims(2, 1) = ny
         dims(3, 1) = nz

         dims(1, 2) = ny
         dims(2, 2) = nx
         dims(3, 2) = nz

         dims(1, 3) = nz
         dims(2, 3) = nx
         dims(3, 3) = ny

      endif
      do i = 1, 3
         dims(2, i) = dims(2, i) / np1
         dims(3, i) = dims(3, i) / np2
      end do


!---------------------------------------------------------------------
! Determine processor coordinates of this processor
! Processor grid is np1xnp2.
! Arrays are always (n1, n2/np1, n3/np2)
! Processor coords are zero-based.
!---------------------------------------------------------------------
      me2 = mod(me, np2)  ! goes from 0...np2-1
      me1 = me/np2        ! goes from 0...np1-1
!---------------------------------------------------------------------
! Communicators for rows/columns of processor grid.
! commslice1 is communicator of all procs with same me1, ranked as me2
! commslice2 is communicator of all procs with same me2, ranked as me1
! mpi_comm_split(comm, color, key, ...)
!---------------------------------------------------------------------
      call MPI_Comm_split(MPI_COMM_WORLD, me1, me2, commslice1, ierr)
      call MPI_Comm_split(MPI_COMM_WORLD, me2, me1, commslice2, ierr)
!      if (timers_enabled) call synchup()

      if (debug) print *, 'proc coords: ', me, me1, me2

!---------------------------------------------------------------------
! Determine which section of the grid is owned by this
! processor.
!---------------------------------------------------------------------
      if (layout_type .eq. layout_0d) then

         do i = 1, 3
            xstart(i) = 1
            xend(i)   = nx
            ystart(i) = 1
            yend(i)   = ny
            zstart(i) = 1
            zend(i)   = nz
         end do

      else if (layout_type .eq. layout_1d) then

         xstart(1) = 1
         xend(1)   = nx
         ystart(1) = 1
         yend(1)   = ny
         zstart(1) = 1 + me2 * nz/np2
         zend(1)   = (me2+1) * nz/np2

         xstart(2) = 1
         xend(2)   = nx
         ystart(2) = 1
         yend(2)   = ny
         zstart(2) = 1 + me2 * nz/np2
         zend(2)   = (me2+1) * nz/np2

         xstart(3) = 1
         xend(3)   = nx
         ystart(3) = 1 + me2 * ny/np2
         yend(3)   = (me2+1) * ny/np2
         zstart(3) = 1
         zend(3)   = nz

      else if (layout_type .eq. layout_2d) then

         xstart(1) = 1
         xend(1)   = nx
         ystart(1) = 1 + me1 * ny/np1
         yend(1)   = (me1+1) * ny/np1
         zstart(1) = 1 + me2 * nz/np2
         zend(1)   = (me2+1) * nz/np2

         xstart(2) = 1 + me1 * nx/np1
         xend(2)   = (me1+1)*nx/np1
         ystart(2) = 1
         yend(2)   = ny
         zstart(2) = zstart(1)
         zend(2)   = zend(1)

         xstart(3) = xstart(2)
         xend(3)   = xend(2)
         ystart(3) = 1 + me2 *ny/np2
         yend(3)   = (me2+1)*ny/np2
         zstart(3) = 1
         zend(3)   = nz
      endif

!---------------------------------------------------------------------
! Set up info for blocking of ffts and transposes.  This improves
! performance on cache-based systems. Blocking involves
! working on a chunk of the problem at a time, taking chunks
! along the first, second, or third dimension.
!
! - In cffts1 blocking is on 2nd dimension (with fft on 1st dim)
! - In cffts2/3 blocking is on 1st dimension (with fft on 2nd and 3rd dims)

! Since 1st dim is always in processor, we'll assume it's long enough
! (default blocking factor is 16 so min size for 1st dim is 16)
! The only case we have to worry about is cffts1 in a 2d decomposition.
! so the blocking factor should not be larger than the 2nd dimension.
!---------------------------------------------------------------------

      fftblock = fftblock_default
      fftblockpad = fftblockpad_default

      if (layout_type .eq. layout_2d) then
         if (dims(2, 1) .lt. fftblock) fftblock = dims(2, 1)
         if (dims(2, 2) .lt. fftblock) fftblock = dims(2, 2)
         if (dims(2, 3) .lt. fftblock) fftblock = dims(2, 3)
      endif

      if (fftblock .ne. fftblock_default) fftblockpad = fftblock+3

      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine compute_indexmap(twiddle, d1, d2, d3)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
! compute function from local (i,j,k) to ibar^2+jbar^2+kbar^2
! for time evolution exponent.
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'mpinpb.fi'
      include 'global.fi'
      integer d1, d2, d3, ierr
      integer i, j, k, ii, ii2, jj, ij2, kk, tid
      double precision ap, twiddle(d1, d2, d3)

      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer(8) region_id, num_it, chunk_size
      integer num_thread
!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel

!---------------------------------------------------------------------
! this function is very different depending on whether
! we are in the 0d, 1d or 2d layout. Compute separately.
! basically we want to convert the fortran indices
!   1 2 3 4 5 6 7 8
! to
!   0 1 2 3 -4 -3 -2 -1
! The following magic formula does the trick:
! mod(i-1+n/2, n) - n/2
!---------------------------------------------------------------------

      ap = - 4.d0 * alpha * pi *pi

      if (layout_type .eq. layout_0d) then ! xyz layout
         num_it = dims(1,3)
         chunk_size = num_it / num_thread
         if (chunk_size .eq. 0) then
             chunk_size = 1
         end if
         ierr = geopm_prof_region(c_char_'AT_PHASE_INDEXMAP_1'//c_null_char, &
                                 GEOPM_REGION_HINT_COMPUTE, region_id)
         ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,k,ii,ii2,jj,ij2,kk,tid)
         tid = omp_get_thread_num()
         ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
         do i = 1, dims(1,3)
            ii =  mod(i+xstart(3)-2+nx/2, nx) - nx/2
            ii2 = ii*ii
            do j = 1, dims(2,3)
               jj = mod(j+ystart(3)-2+ny/2, ny) - ny/2
               ij2 = jj*jj+ii2
               do k = 1, dims(3,3)
                  kk = mod(k+zstart(3)-2+nz/2, nz) - nz/2
                  twiddle(i,j,k) = dexp(ap*dfloat(kk*kk+ij2))
               end do
            end do
            ierr = geopm_tprof_post()
         end do
!$omp end do
!$omp end parallel
         ierr = geopm_prof_exit(region_id)
!         call MPI_Barrier(node_comm, ierr)
      else if (layout_type .eq. layout_1d) then ! zxy layout
         num_it = dims(2,3)
         chunk_size = num_it / num_thread
         if (chunk_size .eq. 0) then
             chunk_size = 1
         end if
         ierr = geopm_prof_region(c_char_'AT_PHASE_INDEXMAP_2'//c_null_char, &
                                  GEOPM_REGION_HINT_COMPUTE, region_id)
         ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,k,ii,ii2,jj,ij2,kk,tid)
         tid = omp_get_thread_num()
         ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
         do i = 1,dims(2,3)
            ii =  mod(i+xstart(3)-2+nx/2, nx) - nx/2
            ii2 = ii*ii
            do j = 1,dims(3,3)
               jj = mod(j+ystart(3)-2+ny/2, ny) - ny/2
               ij2 = jj*jj+ii2
               do k = 1,dims(1,3)
                  kk = mod(k+zstart(3)-2+nz/2, nz) - nz/2
                  twiddle(k,i,j) = dexp(ap*dfloat(kk*kk+ij2))
               end do
            end do
            ierr = geopm_tprof_post()
         end do
!$omp end do
!$omp end parallel
         ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      else if (layout_type .eq. layout_2d) then ! zxy layout
         num_it = dims(2,3)
         chunk_size = num_it / num_thread
         if (chunk_size .eq. 0) then
             chunk_size = 1
         end if
         ierr = geopm_prof_region(c_char_'AT_PHASE_INDEXMAP_3'//c_null_char, &
                                 GEOPM_REGION_HINT_COMPUTE, region_id)
         ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,k,ii,ii2,jj,ij2,kk,tid)
         tid = omp_get_thread_num()
         ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
         do i = 1,dims(2,3)
            ii =  mod(i+xstart(3)-2+nx/2, nx) - nx/2
            ii2 = ii*ii
            do j = 1, dims(3,3)
               jj = mod(j+ystart(3)-2+ny/2, ny) - ny/2
               ij2 = jj*jj+ii2
               do k =1,dims(1,3)
                  kk = mod(k+zstart(3)-2+nz/2, nz) - nz/2
                  twiddle(k,i,j) = dexp(ap*dfloat(kk*kk+ij2))
               end do
            end do
            ierr = geopm_tprof_post()
         end do
!$omp end do
!$omp end parallel
         ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      else
         print *, ' Unknown layout type ', layout_type
         stop
      endif
      return
      end



!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine print_timers()

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      integer i, ierr
      include 'global.fi'
      include 'mpinpb.fi'
      character*25 tstrings(T_max+2)
      double precision t1(T_max+2), tsum(T_max+2), &
                      tming(T_max+2), tmaxg(T_max+2)
      data tstrings / '          total ', &
                     '          setup ', &
                     '            fft ', &
                     '         evolve ', &
                     '       checksum ', &
                     '         fftlow ', &
                     '        fftcopy ', &
                     '      transpose ', &
                     ' transpose1_loc ', &
                     ' transpose1_glo ', &
                     ' transpose1_fin ', &
                     ' transpose2_loc ', &
                     ' transpose2_glo ', &
                     ' transpose2_fin ', &
                     '           sync ', &
                     '           init ', &
                     '        totcomp ', &
                     '        totcomm ' /

      do i = 1, t_max
         t1(i) = timer_read(i)
      end do
      t1(t_max+2) = t1(t_transxzglo) + t1(t_transxyglo) + t1(t_synch)
      t1(t_max+1) = t1(t_total) - t1(t_max+2)

      call MPI_Reduce(t1, tsum,  t_max+2, MPI_DOUBLE_PRECISION, &
                     MPI_SUM, 0, MPI_COMM_WORLD, ierr)
      call MPI_Reduce(t1, tming, t_max+2, MPI_DOUBLE_PRECISION, &
                     MPI_MIN, 0, MPI_COMM_WORLD, ierr)
      call MPI_Reduce(t1, tmaxg, t_max+2, MPI_DOUBLE_PRECISION, &
                     MPI_MAX, 0, MPI_COMM_WORLD, ierr)

      if (me .ne. 0) return
      write(*, 800) np
      do i = 1, t_max+2
         if (tsum(i) .ne. 0.0d0) then
            write(*, 810) i, tstrings(i), tming(i), tmaxg(i), tsum(i)/np
         endif
      end do
 800  format(' nprocs =', i6, 19x, 'minimum', 5x, 'maximum', &
            5x, 'average')
 810  format(' timer ', i2, '(', A16, ') :', 3(2X,F10.4))
      return
      end



!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine fft(dir, x1, x2, ntdivnp_i)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer dir
      integer ntdivnp_i
      double complex x1(ntdivnp_i), x2(ntdivnp_i)

      double complex scratch(fftblockpad_default*maxdim*2)

!---------------------------------------------------------------------
! note: args x1, x2 must be different arrays
! note: args for cfftsx are (direction, layout, xin, xout, scratch)
!       xin/xout may be the same and it can be somewhat faster
!       if they are
! note: args for transpose are (layout1, layout2, xin, xout)
!       xin/xout must be different
!---------------------------------------------------------------------

      if (dir .eq. 1) then
         if (layout_type .eq. layout_0d) then
            call cffts1(1, dims(1,1), dims(2,1), dims(3,1), &
                       x1, x1, scratch)
            call cffts2(1, dims(1,2), dims(2,2), dims(3,2), &
                       x1, x1, scratch)
            call cffts3(1, dims(1,3), dims(2,3), dims(3,3), &
                       x1, x2, scratch)
         else if (layout_type .eq. layout_1d) then
            call cffts1(1, dims(1,1), dims(2,1), dims(3,1), &
                       x1, x1, scratch)
            call cffts2(1, dims(1,2), dims(2,2), dims(3,2), &
                       x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_xy_z(2, 3, x1, x2, ntdivnp_i)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(1, dims(1,3), dims(2,3), dims(3,3), &
                       x2, x2, scratch)
         else if (layout_type .eq. layout_2d) then
            call cffts1(1, dims(1,1), dims(2,1), dims(3,1), &
                       x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_y(1, 2, x1, x2, ntdivnp_i)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(1, dims(1,2), dims(2,2), dims(3,2), &
                       x2, x2, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_z(2, 3, x2, x1, ntdivnp_i)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(1, dims(1,3), dims(2,3), dims(3,3), &
                       x1, x2, scratch)
         endif
      else
         if (layout_type .eq. layout_0d) then
            call cffts3(-1, dims(1,3), dims(2,3), dims(3,3), &
                       x1, x1, scratch)
            call cffts2(-1, dims(1,2), dims(2,2), dims(3,2), &
                       x1, x1, scratch)
            call cffts1(-1, dims(1,1), dims(2,1), dims(3,1), &
                       x1, x2, scratch)
         else if (layout_type .eq. layout_1d) then
            call cffts1(-1, dims(1,3), dims(2,3), dims(3,3), &
                       x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_yz(3, 2, x1, x2, ntdivnp_i)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts2(-1, dims(1,2), dims(2,2), dims(3,2), &
                       x2, x2, scratch)
            call cffts1(-1, dims(1,1), dims(2,1), dims(3,1), &
                       x2, x2, scratch)
         else if (layout_type .eq. layout_2d) then
            call cffts1(-1, dims(1,3), dims(2,3), dims(3,3), &
                       x1, x1, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_z(3, 2, x1, x2, ntdivnp_i)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(-1, dims(1,2), dims(2,2), dims(3,2), &
                       x2, x2, scratch)
            if (timers_enabled) call timer_start(T_transpose)
            call transpose_x_y(2, 1, x2, x1, ntdivnp_i)
            if (timers_enabled) call timer_stop(T_transpose)
            call cffts1(-1, dims(1,1), dims(2,1), dims(3,1), &
                       x1, x2, scratch)
         endif
      endif
      return
      end

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine cffts1(is, d1, d2, d3, x, xout, y)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none

      include 'global.fi'
      include 'mpif.h'
      integer is, d1, d2, d3, logd1
      double complex x(d1,d2,d3)
      double complex xout(d1,d2,d3)
      double complex y(fftblockpad, d1, 2)
      integer i, j, k, jj, tid, ierr, phase

      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer(8) region_id
      integer(8) num_it
      integer(8) chunk_size
      integer num_thread
!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel
      num_it = d3
      chunk_size = num_it / num_thread
      if (chunk_size .eq. 0) then
          chunk_size = 1
      end if

      logd1 = ilog2(d1)
      pswitch = IEOR(pswitch, 1)

      if (pswitch .eq. 0) then
         ierr = geopm_prof_region(c_char_'AT_PHASE_FFT_1'//c_null_char, &
                                 GEOPM_REGION_HINT_COMPUTE, region_id)
      else
         ierr = geopm_prof_region(c_char_'AT_PHASE_FFT_1_2'//c_null_char, &
                                 GEOPM_REGION_HINT_COMPUTE, region_id)
      endif
      ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,k,jj,tid,y)
      tid = omp_get_thread_num()
      ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
      do k = 1, d3
         do jj = 0, d2 - fftblock, fftblock
            if (timers_enabled) call timer_start(T_fftcopy)
            do j = 1, fftblock
               do i = 1, d1
                  y(j,i,1) = x(i,j+jj,k)
               enddo
            enddo
            if (timers_enabled) call timer_stop(T_fftcopy)

            if (timers_enabled) call timer_start(T_fftlow)
            call cfftz (is, logd1, d1, y, y(1,1,2))
            if (timers_enabled) call timer_stop(T_fftlow)

            if (timers_enabled) call timer_start(T_fftcopy)
            do j = 1, fftblock
               do i = 1, d1
                  xout(i,j+jj,k) = y(j,i,1)
               enddo
            enddo
            if (timers_enabled) call timer_stop(T_fftcopy)
         enddo
         ierr = geopm_tprof_post()
      enddo
!$omp end do
!$omp end parallel
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine cffts2(is, d1, d2, d3, x, xout, y)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none

      include 'global.fi'
      include 'mpif.h'
      integer is, d1, d2, d3, logd2, ierr
      double complex x(d1,d2,d3)
      double complex xout(d1,d2,d3)
      double complex y(fftblockpad, d2, 2)
      integer i, j, k, ii, tid

      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer(8) region_id
      integer(8) num_it
      integer(8) chunk_size
      integer num_thread

      logd2 = ilog2(d2)

!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel
      num_it = d3
      chunk_size = num_it / num_thread
      if (chunk_size .eq. 0) then
          chunk_size = 1
      end if
      ierr = geopm_prof_region(c_char_'AT_PHASE_FFT_2'//c_null_char, &
                              GEOPM_REGION_HINT_COMPUTE, region_id)
      ierr = geopm_prof_enter(region_id)

!$omp parallel default(shared) private(i,j,k,ii,y,tid)
      tid = omp_get_thread_num()
      ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
      do k = 1, d3
        do ii = 0, d1 - fftblock, fftblock
           if (timers_enabled) call timer_start(T_fftcopy)
           do j = 1, d2
              do i = 1, fftblock
                 y(i,j,1) = x(i+ii,j,k)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)

           if (timers_enabled) call timer_start(T_fftlow)
           call cfftz (is, logd2, d2, y, y(1, 1, 2))
           if (timers_enabled) call timer_stop(T_fftlow)

           if (timers_enabled) call timer_start(T_fftcopy)
           do j = 1, d2
              do i = 1, fftblock
                 xout(i+ii,j,k) = y(i,j,1)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)
        enddo
        ierr = geopm_tprof_post()
      enddo
!$omp end do
!$omp end parallel
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine cffts3(is, d1, d2, d3, x, xout, y)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none

      include 'global.fi'
      include 'mpif.h'
      integer is, d1, d2, d3, logd3
      double complex x(d1,d2,d3)
      double complex xout(d1,d2,d3)
      double complex y(fftblockpad, d3, 2)
      integer i, j, k, ii, tid, ierr

      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer(8) region_id
      integer(8) num_it
      integer(8) chunk_size
      integer num_thread

      logd3 = ilog2(d3)

!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel
      num_it = d2
      chunk_size = num_it / num_thread
      if (chunk_size .eq. 0) then
          chunk_size = 1
      end if
      ierr = geopm_prof_region(c_char_'AT_PHASE_FFT_3'//c_null_char, &
                              GEOPM_REGION_HINT_COMPUTE, region_id)
      ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,k,ii,y, tid)
      tid = omp_get_thread_num()
      ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
      do j = 1, d2
        do ii = 0, d1 - fftblock, fftblock
           if (timers_enabled) call timer_start(T_fftcopy)
           do k = 1, d3
              do i = 1, fftblock
                 y(i,k,1) = x(i+ii,j,k)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)

           if (timers_enabled) call timer_start(T_fftlow)
           call cfftz (is, logd3, d3, y, y(1, 1, 2))
           if (timers_enabled) call timer_stop(T_fftlow)

           if (timers_enabled) call timer_start(T_fftcopy)
           do k = 1, d3
              do i = 1, fftblock
                 xout(i+ii,j,k) = y(i,k,1)
              enddo
           enddo
           if (timers_enabled) call timer_stop(T_fftcopy)
        enddo
        ierr = geopm_tprof_post()
      enddo
!$omp end do
!$omp end parallel
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine fft_init (n)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
! compute the roots-of-unity array that will be used for subsequent FFTs.
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'

      integer m,n,nu,ku,i,j,ln
      double precision t, ti

!---------------------------------------------------------------------
!   Initialize the U array with sines and cosines in a manner that permits
!   stride one access at each FFT iteration.
!---------------------------------------------------------------------
      nu = n
      m = ilog2(n)
      u(1) = m
      ku = 2
      ln = 1

      do j = 1, m
         t = pi / ln

         do i = 0, ln - 1
            ti = i * t
            u(i+ku) = dcmplx (cos (ti), sin(ti))
         enddo

         ku = ku + ln
         ln = 2 * ln
      enddo

      return
      end

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine cfftz (is, m, n, x, y)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
!   Computes NY N-point complex-to-complex FFTs of X using an algorithm due
!   to Swarztrauber.  X is both the input and the output array, while Y is a
!   scratch array.  It is assumed that N = 2^M.  Before calling CFFTZ to
!   perform FFTs, the array U must be initialized by calling CFFTZ with IS
!   set to 0 and M set to MX, where MX is the maximum value of M for any
!   subsequent call.
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'

      integer is,m,n,i,j,l,mx
      double complex x, y

      dimension x(fftblockpad,n), y(fftblockpad,n)

!---------------------------------------------------------------------
!   Check if input parameters are invalid.
!---------------------------------------------------------------------
      mx = u(1)
      if ((is .ne. 1 .and. is .ne. -1) .or. m .lt. 1 .or. m .gt. mx) &
       then
        write (*, 1)  is, m, mx
 1      format ('CFFTZ: Either U has not been initialized, or else'/ &
         'one of the input parameters is invalid', 3I5)
        stop
      endif

!---------------------------------------------------------------------
!   Perform one variant of the Stockham FFT.
!---------------------------------------------------------------------
      do l = 1, m, 2
        call fftz2 (is, l, m, n, fftblock, fftblockpad, u, x, y)
        if (l .eq. m) goto 160
        call fftz2 (is, l + 1, m, n, fftblock, fftblockpad, u, y, x)
      enddo

      goto 180

!---------------------------------------------------------------------
!   Copy Y to X.
!---------------------------------------------------------------------
 160  do j = 1, n
        do i = 1, fftblock
          x(i,j) = y(i,j)
        enddo
      enddo

 180  continue

      return
      end

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine fftz2 (is, l, m, n, ny, ny1, u, x, y)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

!---------------------------------------------------------------------
!   Performs the L-th iteration of the second variant of the Stockham FFT.
!---------------------------------------------------------------------

      implicit none

      integer is,k,l,m,n,ny,ny1,n1,li,lj,lk,ku,i,j,i11,i12,i21,i22
      double complex u,x,y,u1,x11,x21
      dimension u(n), x(ny1,n), y(ny1,n)


!---------------------------------------------------------------------
!   Set initial parameters.
!---------------------------------------------------------------------

      n1 = n / 2
      lk = 2 ** (l - 1)
      li = 2 ** (m - l)
      lj = 2 * lk
      ku = li + 1

      do i = 0, li - 1
        i11 = i * lk + 1
        i12 = i11 + n1
        i21 = i * lj + 1
        i22 = i21 + lk
        if (is .ge. 1) then
          u1 = u(ku+i)
        else
          u1 = dconjg (u(ku+i))
        endif

!---------------------------------------------------------------------
!   This loop is vectorizable.
!---------------------------------------------------------------------
        do k = 0, lk - 1
          do j = 1, ny
            x11 = x(j,i11+k)
            x21 = x(j,i12+k)
            y(j,i21+k) = x11 + x21
            y(j,i22+k) = u1 * (x11 - x21)
          enddo
        enddo
      enddo

      return
      end

!---------------------------------------------------------------------


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      integer function ilog2(n)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      integer n, nn, lg
      if (n .eq. 1) then
         ilog2=0
         return
      endif
      lg = 1
      nn = 2
      do while (nn .lt. n)
         nn = nn*2
         lg = lg+1
      end do
      ilog2 = lg
      return
      end

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_yz(l1, l2, xin, xout, ntdivnp_i)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer l1, l2
      integer ntdivnp_i
      double complex xin(ntdivnp_i), xout(ntdivnp_i)

      call transpose2_local(dims(1,l1),dims(2, l1)*dims(3, l1), &
                               xin, xout)

      call transpose2_global(xout, xin, ntdivnp_i)

      call transpose2_finish(dims(1,l1),dims(2, l1)*dims(3, l1), &
                               xin, xout)

      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_xy_z(l1, l2, xin, xout, ntdivnp_i)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer l1, l2
      integer ntdivnp_i
      double complex xin(ntdivnp_i), xout(ntdivnp_i)

      call transpose2_local(dims(1,l1)*dims(2, l1),dims(3, l1), &
                               xin, xout)
      call transpose2_global(xout, xin, ntdivnp_i)
      call transpose2_finish(dims(1,l1)*dims(2, l1),dims(3, l1), &
                               xin, xout)

      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose2_local(n1, n2, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------
      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'global.fi'
      include 'mpif.h'
      integer n1, n2, ierr
      double complex xin(n1, n2), xout(n2, n1)

      double complex z(transblockpad, transblock)

      integer i, j, ii, jj, tid

      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer(8) region_id
      integer(8) num_it
      integer(8) chunk_size
      integer num_thread
!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel

      if (timers_enabled) call timer_start(T_transxzloc)

!---------------------------------------------------------------------
! If possible, block the transpose for cache memory systems.
! How much does this help? Example: R8000 Power Challenge (90 MHz)
! Blocked version decreases time spend in this routine
! from 14 seconds to 5.2 seconds on 8 nodes class A.
!---------------------------------------------------------------------

      if (n1 .lt. transblock .or. n2 .lt. transblock) then
         if (n1 .ge. n2) then
            do j = 1, n2
               do i = 1, n1
                  xout(j, i) = xin(i, j)
               end do
            end do
         else
            do i = 1, n1
               do j = 1, n2
                  xout(j, i) = xin(i, j)
               end do
            end do
         endif
      else

      num_it = n2 / transblock
      chunk_size = num_it / num_thread
      if (chunk_size .eq. 0) then
          chunk_size = 1
      end if

      ierr = geopm_prof_region(c_char_'AT_PHASE_TRANS_1'//c_null_char, &
                               GEOPM_REGION_HINT_MEMORY, region_id)
         ierr = geopm_prof_enter(region_id)
!$omp parallel default(shared) private(i,j,ii,jj,z,tid)
         tid = omp_get_thread_num()
         ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
         do j = 0, n2-1, transblock
            do i = 0, n1-1, transblock

!---------------------------------------------------------------------
! Note: compiler should be able to take j+jj out of inner loop
!---------------------------------------------------------------------
               do jj = 1, transblock
                  do ii = 1, transblock
                     z(jj,ii) = xin(i+ii, j+jj)
                  end do
               end do

               do ii = 1, transblock
                  do jj = 1, transblock
                     xout(j+jj, i+ii) = z(jj,ii)
                  end do
               end do
            end do
            ierr = geopm_tprof_post()
         end do
!$omp end do
!$omp end parallel
         ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      endif
      if (timers_enabled) call timer_stop(T_transxzloc)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose2_global(xin, xout, ntdivnp_i)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'global.fi'
      include 'mpinpb.fi'
      integer ntdivnp_i
      double complex xin(ntdivnp_i)
      double complex xout(ntdivnp_i)
      integer ierr, tid

!      if (timers_enabled) call synchup()

      integer(8) region_id

      ierr = geopm_prof_region(c_char_'AT_PHASE_MPI_ATA_1'//c_null_char, &
                              GEOPM_REGION_HINT_NETWORK, region_id)
      ierr = geopm_prof_enter(region_id)
      if (timers_enabled) call timer_start(T_transxzglo)
      call mpi_alltoall(xin, ntdivnp_i/np, dc_type, &
                       xout, ntdivnp_i/np, dc_type, &
                       commslice1, ierr)
      if (timers_enabled) call timer_stop(T_transxzglo)
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      return
      end



!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose2_finish(n1, n2, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'global.fi'
      include 'mpif.h'
      integer n1, n2, ioff
      double complex xin(n2, n1/np2, 0:np2-1), xout(n2*np2, n1/np2)
      integer i, j, p, tid, ierr

      integer, external :: omp_get_thread_num, omp_get_num_threads
      integer(8) region_id
      integer(8) num_it
      integer(8) chunk_size
      integer num_thread
!$omp parallel
      num_thread = omp_get_num_threads()
!$omp end parallel
      num_it = np2
      chunk_size = num_it / num_thread
      if (chunk_size .eq. 0) then
          chunk_size = 1
      end if
      ierr = geopm_prof_region(c_char_'AT_PHASE_TRANS_2'//c_null_char, &
                              GEOPM_REGION_HINT_MEMORY, region_id)
      ierr = geopm_prof_enter(region_id)

      if (timers_enabled) call timer_start(T_transxzfin)
!$omp parallel default(shared) private(i,j,p,ioff,tid)
      tid = omp_get_thread_num()
      ierr = geopm_tprof_init_loop(num_thread, tid, num_it, chunk_size)
!$omp do schedule(static, chunk_size)
      do p = 0, np2-1
         ioff = p*n2
         do j = 1, n1/np2
            do i = 1, n2
               xout(i+ioff, j) = xin(i, j, p)
            end do
         end do
         ierr = geopm_tprof_post()
      end do
!$omp end do
!$omp end parallel
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      if (timers_enabled) call timer_stop(T_transxzfin)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_z(l1, l2, xin, xout, ntdivnp_i)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer l1, l2
      integer ntdivnp_i
      double complex xin(ntdivnp_i), xout(ntdivnp_i)

      call transpose_x_z_local(dims(1,l1),dims(2,l1),dims(3,l1), &
                              xin, xout)
      call transpose_x_z_global(dims(1,l1),dims(2,l1),dims(3,l1), &
                               xout, xin)
      call transpose_x_z_finish(dims(1,l2),dims(2,l2),dims(3,l2), &
                               xin, xout)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_z_local(d1, d2, d3, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer d1, d2, d3
      double complex xin(d1,d2,d3)
      double complex xout(d3,d2,d1)
      integer block1, block3
      integer i, j, k, kk, ii, i1, k1

      double complex buf(transblockpad, maxdim)
      if (timers_enabled) call timer_start(T_transxzloc)
      if (d1 .lt. 32) goto 100
      block3 = d3
      if (block3 .eq. 1)  goto 100
      if (block3 .gt. transblock) block3 = transblock
      block1 = d1
      if (block1*block3 .gt. transblock*transblock) &
               block1 = transblock*transblock/block3
!---------------------------------------------------------------------
! blocked transpose
!---------------------------------------------------------------------
      do j = 1, d2
         do kk = 0, d3-block3, block3
            do ii = 0, d1-block1, block1

               do k = 1, block3
                  k1 = k + kk
                  do i = 1, block1
                     buf(k, i) = xin(i+ii, j, k1)
                  end do
               end do

               do i = 1, block1
                  i1 = i + ii
                  do k = 1, block3
                     xout(k+kk, j, i1) = buf(k, i)
                  end do
               end do

            end do
         end do
      end do
      goto 200


!---------------------------------------------------------------------
! basic transpose
!---------------------------------------------------------------------
 100  continue

      do j = 1, d2
         do k = 1, d3
            do i = 1, d1
               xout(k, j, i) = xin(i, j, k)
            end do
         end do
      end do

!---------------------------------------------------------------------
! all done
!---------------------------------------------------------------------
 200  continue

      if (timers_enabled) call timer_stop(T_transxzloc)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_z_global(d1, d2, d3, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'global.fi'
      include 'mpinpb.fi'
      integer d1, d2, d3
      double complex xin(d3,d2,d1)
      double complex xout(d3,d2,d1) ! not real layout, but right size
      integer ierr, tid
      integer(8) region_id

!      if (timers_enabled) call synchup()

!---------------------------------------------------------------------
! do transpose among all  processes with same 1-coord (me1)
!---------------------------------------------------------------------
      ierr = geopm_prof_region(c_char_'AT_PHASE_MPI_ATA_2'//c_null_char, &
                               GEOPM_REGION_HINT_NETWORK, region_id)
      ierr = geopm_prof_enter(region_id)
      if (timers_enabled)call timer_start(T_transxzglo)
      call mpi_alltoall(xin, d1*d2*d3/np2, dc_type, &
                       xout, d1*d2*d3/np2, dc_type, &
                       commslice1, ierr)
      if (timers_enabled) call timer_stop(T_transxzglo)
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_z_finish(d1, d2, d3, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer d1, d2, d3
      double complex xin(d1/np2, d2, d3, 0:np2-1)
      double complex xout(d1,d2,d3)
      integer i, j, k, p, ioff
      if (timers_enabled) call timer_start(T_transxzfin)
!---------------------------------------------------------------------
! this is the most straightforward way of doing it. the
! calculation in the inner loop doesn't help.
!      do i = 1, d1/np2
!         do j = 1, d2
!            do k = 1, d3
!               do p = 0, np2-1
!                  ii = i + p*d1/np2
!                  xout(ii, j, k) = xin(i, j, k, p)
!               end do
!            end do
!         end do
!      end do
!---------------------------------------------------------------------

      do p = 0, np2-1
         ioff = p*d1/np2
         do k = 1, d3
            do j = 1, d2
               do i = 1, d1/np2
                  xout(i+ioff, j, k) = xin(i, j, k, p)
               end do
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxzfin)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_y(l1, l2, xin, xout, ntdivnp_i)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer l1, l2
      integer ntdivnp_i
      double complex xin(ntdivnp_i), xout(ntdivnp_i)

!---------------------------------------------------------------------
! xy transpose is a little tricky, since we don't want
! to touch 3rd axis. But alltoall must involve 3rd axis (most
! slowly varying) to be efficient. So we do
! (nx, ny/np1, nz/np2) -> (ny/np1, nz/np2, nx) (local)
! (ny/np1, nz/np2, nx) -> ((ny/np1*nz/np2)*np1, nx/np1) (global)
! then local finish.
!---------------------------------------------------------------------


      call transpose_x_y_local(dims(1,l1),dims(2,l1),dims(3,l1), &
                              xin, xout)
      call transpose_x_y_global(dims(1,l1),dims(2,l1),dims(3,l1), &
                               xout, xin)
      call transpose_x_y_finish(dims(1,l2),dims(2,l2),dims(3,l2), &
                               xin, xout)

      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_y_local(d1, d2, d3, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer d1, d2, d3
      double complex xin(d1, d2, d3)
      double complex xout(d2, d3, d1)
      integer i, j, k
      if (timers_enabled) call timer_start(T_transxyloc)

      do k = 1, d3
         do i = 1, d1
            do j = 1, d2
               xout(j,k,i)=xin(i,j,k)
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxyloc)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_y_global(d1, d2, d3, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      use geopm
      use, intrinsic :: ISO_C_BINDING
      implicit none
      include 'global.fi'
      include 'mpinpb.fi'
      integer d1, d2, d3
!---------------------------------------------------------------------
! array is in form (ny/np1, nz/np2, nx)
!---------------------------------------------------------------------
      double complex xin(d2,d3,d1)
      double complex xout(d2,d3,d1) ! not real layout but right size
      integer ierr, tid
      integer(8) region_id

!      if (timers_enabled) call synchup()

!---------------------------------------------------------------------
! do transpose among all processes with same 1-coord (me1)
!---------------------------------------------------------------------
      ierr = geopm_prof_region(c_char_'AT_PHASE_MPI_ATA_3'//c_null_char, &
                              GEOPM_REGION_HINT_NETWORK, region_id)
      ierr = geopm_prof_enter(region_id)
      if (timers_enabled) call timer_start(T_transxyglo)
      call mpi_alltoall(xin, d1*d2*d3/np1, dc_type, &
                       xout, d1*d2*d3/np1, dc_type, &
                       commslice2, ierr)
      if (timers_enabled) call timer_stop(T_transxyglo)
      ierr = geopm_prof_exit(region_id)
!      call MPI_Barrier(node_comm, ierr)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine transpose_x_y_finish(d1, d2, d3, xin, xout)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      integer d1, d2, d3
      double complex xin(d1/np1, d3, d2, 0:np1-1)
      double complex xout(d1,d2,d3)
      integer i, j, k, p, ioff
      if (timers_enabled) call timer_start(T_transxyfin)
!---------------------------------------------------------------------
! this is the most straightforward way of doing it. the
! calculation in the inner loop doesn't help.
!      do i = 1, d1/np1
!         do j = 1, d2
!            do k = 1, d3
!               do p = 0, np1-1
!                  ii = i + p*d1/np1
! note order is screwy bcz we have (ny/np1, nz/np2, nx) -> (ny, nx/np1, nz/np2)
!                  xout(ii, j, k) = xin(i, k, j, p)
!               end do
!            end do
!         end do
!      end do
!---------------------------------------------------------------------

      do p = 0, np1-1
         ioff = p*d1/np1
         do k = 1, d3
            do j = 1, d2
               do i = 1, d1/np1
                  xout(i+ioff, j, k) = xin(i, k, j, p)
               end do
            end do
         end do
      end do
      if (timers_enabled) call timer_stop(T_transxyfin)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine checksum(i, u1, d1, d2, d3)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      include 'mpinpb.fi'
      integer i, d1, d2, d3
      double complex u1(d1, d2, d3)
      integer j, q,r,s, ierr
      double complex chk,allchk
      chk = (0.0,0.0)

      do j=1,1024
         q = mod(j, nx)+1
         if (q .ge. xstart(1) .and. q .le. xend(1)) then
            r = mod(3*j,ny)+1
            if (r .ge. ystart(1) .and. r .le. yend(1)) then
               s = mod(5*j,nz)+1
               if (s .ge. zstart(1) .and. s .le. zend(1)) then
                  chk=chk+u1(q-xstart(1)+1,r-ystart(1)+1,s-zstart(1)+1)
               end if
            end if
         end if
      end do
      chk = chk/ntotal_f

      if (timers_enabled) call timer_start(T_synch)
      call MPI_Reduce(chk, allchk, 1, dc_type, MPI_SUM, &
                     0, MPI_COMM_WORLD, ierr)
      if (timers_enabled) call timer_stop(T_synch)
      if (me .eq. 0) then
            write (*, 30) i, allchk
 30         format (' T =',I5,5X,'Checksum =',1P2D22.12)
      endif

!      sums(i) = allchk
!     If we compute the checksum for diagnostic purposes, we let i be
!     negative, so the result will not be stored in an array
      if (i .gt. 0) sums(i) = allchk

      return
      end

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine synchup

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      include 'mpinpb.fi'
      integer ierr
      call timer_start(T_synch)
      call mpi_barrier(MPI_COMM_WORLD, ierr)
      call timer_stop(T_synch)
      return
      end


!---------------------------------------------------------------------
!---------------------------------------------------------------------

      subroutine verify (d1, d2, d3, nt, verified, class)

!---------------------------------------------------------------------
!---------------------------------------------------------------------

      implicit none
      include 'global.fi'
      include 'mpinpb.fi'
      integer d1, d2, d3, nt
      character class
      logical verified
      integer ierr, size, i
      double precision err, epsilon

!---------------------------------------------------------------------
!   Reference checksums
!---------------------------------------------------------------------
      double complex csum_ref(25)


      class = 'U'

      if (me .ne. 0) return

      epsilon = 1.0d-12
      verified = .FALSE.

      if (d1 .eq. 64 .and. &
         d2 .eq. 64 .and. &
         d3 .eq. 64 .and. &
         nt .eq. 6) then
!---------------------------------------------------------------------
!   Sample size reference checksums
!---------------------------------------------------------------------
         class = 'S'
         csum_ref(1) = dcmplx(5.546087004964D+02, 4.845363331978D+02)
         csum_ref(2) = dcmplx(5.546385409189D+02, 4.865304269511D+02)
         csum_ref(3) = dcmplx(5.546148406171D+02, 4.883910722336D+02)
         csum_ref(4) = dcmplx(5.545423607415D+02, 4.901273169046D+02)
         csum_ref(5) = dcmplx(5.544255039624D+02, 4.917475857993D+02)
         csum_ref(6) = dcmplx(5.542683411902D+02, 4.932597244941D+02)

      else if (d1 .eq. 128 .and. &
         d2 .eq. 128 .and. &
         d3 .eq. 32 .and. &
         nt .eq. 6) then
!---------------------------------------------------------------------
!   Class W size reference checksums
!---------------------------------------------------------------------
         class = 'W'
         csum_ref(1) = dcmplx(5.673612178944D+02, 5.293246849175D+02)
         csum_ref(2) = dcmplx(5.631436885271D+02, 5.282149986629D+02)
         csum_ref(3) = dcmplx(5.594024089970D+02, 5.270996558037D+02)
         csum_ref(4) = dcmplx(5.560698047020D+02, 5.260027904925D+02)
         csum_ref(5) = dcmplx(5.530898991250D+02, 5.249400845633D+02)
         csum_ref(6) = dcmplx(5.504159734538D+02, 5.239212247086D+02)

      else if (d1 .eq. 256 .and. &
         d2 .eq. 256 .and. &
         d3 .eq. 128 .and. &
         nt .eq. 6) then
!---------------------------------------------------------------------
!   Class A size reference checksums
!---------------------------------------------------------------------
         class = 'A'
         csum_ref(1) = dcmplx(5.046735008193D+02, 5.114047905510D+02)
         csum_ref(2) = dcmplx(5.059412319734D+02, 5.098809666433D+02)
         csum_ref(3) = dcmplx(5.069376896287D+02, 5.098144042213D+02)
         csum_ref(4) = dcmplx(5.077892868474D+02, 5.101336130759D+02)
         csum_ref(5) = dcmplx(5.085233095391D+02, 5.104914655194D+02)
         csum_ref(6) = dcmplx(5.091487099959D+02, 5.107917842803D+02)

      else if (d1 .eq. 512 .and. &
         d2 .eq. 256 .and. &
         d3 .eq. 256 .and. &
         nt .eq. 20) then
!---------------------------------------------------------------------
!   Class B size reference checksums
!---------------------------------------------------------------------
         class = 'B'
         csum_ref(1)  = dcmplx(5.177643571579D+02, 5.077803458597D+02)
         csum_ref(2)  = dcmplx(5.154521291263D+02, 5.088249431599D+02)
         csum_ref(3)  = dcmplx(5.146409228649D+02, 5.096208912659D+02)
         csum_ref(4)  = dcmplx(5.142378756213D+02, 5.101023387619D+02)
         csum_ref(5)  = dcmplx(5.139626667737D+02, 5.103976610617D+02)
         csum_ref(6)  = dcmplx(5.137423460082D+02, 5.105948019802D+02)
         csum_ref(7)  = dcmplx(5.135547056878D+02, 5.107404165783D+02)
         csum_ref(8)  = dcmplx(5.133910925466D+02, 5.108576573661D+02)
         csum_ref(9)  = dcmplx(5.132470705390D+02, 5.109577278523D+02)
         csum_ref(10) = dcmplx(5.131197729984D+02, 5.110460304483D+02)
         csum_ref(11) = dcmplx(5.130070319283D+02, 5.111252433800D+02)
         csum_ref(12) = dcmplx(5.129070537032D+02, 5.111968077718D+02)
         csum_ref(13) = dcmplx(5.128182883502D+02, 5.112616233064D+02)
         csum_ref(14) = dcmplx(5.127393733383D+02, 5.113203605551D+02)
         csum_ref(15) = dcmplx(5.126691062020D+02, 5.113735928093D+02)
         csum_ref(16) = dcmplx(5.126064276004D+02, 5.114218460548D+02)
         csum_ref(17) = dcmplx(5.125504076570D+02, 5.114656139760D+02)
         csum_ref(18) = dcmplx(5.125002331720D+02, 5.115053595966D+02)
         csum_ref(19) = dcmplx(5.124551951846D+02, 5.115415130407D+02)
         csum_ref(20) = dcmplx(5.124146770029D+02, 5.115744692211D+02)

      else if (d1 .eq. 512 .and. &
         d2 .eq. 512 .and. &
         d3 .eq. 512 .and. &
         nt .eq. 20) then
!---------------------------------------------------------------------
!   Class C size reference checksums
!---------------------------------------------------------------------
         class = 'C'
         csum_ref(1)  = dcmplx(5.195078707457D+02, 5.149019699238D+02)
         csum_ref(2)  = dcmplx(5.155422171134D+02, 5.127578201997D+02)
         csum_ref(3)  = dcmplx(5.144678022222D+02, 5.122251847514D+02)
         csum_ref(4)  = dcmplx(5.140150594328D+02, 5.121090289018D+02)
         csum_ref(5)  = dcmplx(5.137550426810D+02, 5.121143685824D+02)
         csum_ref(6)  = dcmplx(5.135811056728D+02, 5.121496764568D+02)
         csum_ref(7)  = dcmplx(5.134569343165D+02, 5.121870921893D+02)
         csum_ref(8)  = dcmplx(5.133651975661D+02, 5.122193250322D+02)
         csum_ref(9)  = dcmplx(5.132955192805D+02, 5.122454735794D+02)
         csum_ref(10) = dcmplx(5.132410471738D+02, 5.122663649603D+02)
         csum_ref(11) = dcmplx(5.131971141679D+02, 5.122830879827D+02)
         csum_ref(12) = dcmplx(5.131605205716D+02, 5.122965869718D+02)
         csum_ref(13) = dcmplx(5.131290734194D+02, 5.123075927445D+02)
         csum_ref(14) = dcmplx(5.131012720314D+02, 5.123166486553D+02)
         csum_ref(15) = dcmplx(5.130760908195D+02, 5.123241541685D+02)
         csum_ref(16) = dcmplx(5.130528295923D+02, 5.123304037599D+02)
         csum_ref(17) = dcmplx(5.130310107773D+02, 5.123356167976D+02)
         csum_ref(18) = dcmplx(5.130103090133D+02, 5.123399592211D+02)
         csum_ref(19) = dcmplx(5.129905029333D+02, 5.123435588985D+02)
         csum_ref(20) = dcmplx(5.129714421109D+02, 5.123465164008D+02)

      else if (d1 .eq. 2048 .and. &
         d2 .eq. 1024 .and. &
         d3 .eq. 1024 .and. &
         nt .eq. 25) then
!---------------------------------------------------------------------
!   Class D size reference checksums
!---------------------------------------------------------------------
         class = 'D'
         csum_ref(1)  = dcmplx(5.122230065252D+02, 5.118534037109D+02)
         csum_ref(2)  = dcmplx(5.120463975765D+02, 5.117061181082D+02)
         csum_ref(3)  = dcmplx(5.119865766760D+02, 5.117096364601D+02)
         csum_ref(4)  = dcmplx(5.119518799488D+02, 5.117373863950D+02)
         csum_ref(5)  = dcmplx(5.119269088223D+02, 5.117680347632D+02)
         csum_ref(6)  = dcmplx(5.119082416858D+02, 5.117967875532D+02)
         csum_ref(7)  = dcmplx(5.118943814638D+02, 5.118225281841D+02)
         csum_ref(8)  = dcmplx(5.118842385057D+02, 5.118451629348D+02)
         csum_ref(9)  = dcmplx(5.118769435632D+02, 5.118649119387D+02)
         csum_ref(10) = dcmplx(5.118718203448D+02, 5.118820803844D+02)
         csum_ref(11) = dcmplx(5.118683569061D+02, 5.118969781011D+02)
         csum_ref(12) = dcmplx(5.118661708593D+02, 5.119098918835D+02)
         csum_ref(13) = dcmplx(5.118649768950D+02, 5.119210777066D+02)
         csum_ref(14) = dcmplx(5.118645605626D+02, 5.119307604484D+02)
         csum_ref(15) = dcmplx(5.118647586618D+02, 5.119391362671D+02)
         csum_ref(16) = dcmplx(5.118654451572D+02, 5.119463757241D+02)
         csum_ref(17) = dcmplx(5.118665212451D+02, 5.119526269238D+02)
         csum_ref(18) = dcmplx(5.118679083821D+02, 5.119580184108D+02)
         csum_ref(19) = dcmplx(5.118695433664D+02, 5.119626617538D+02)
         csum_ref(20) = dcmplx(5.118713748264D+02, 5.119666538138D+02)
         csum_ref(21) = dcmplx(5.118733606701D+02, 5.119700787219D+02)
         csum_ref(22) = dcmplx(5.118754661974D+02, 5.119730095953D+02)
         csum_ref(23) = dcmplx(5.118776626738D+02, 5.119755100241D+02)
         csum_ref(24) = dcmplx(5.118799262314D+02, 5.119776353561D+02)
         csum_ref(25) = dcmplx(5.118822370068D+02, 5.119794338060D+02)

      else if (d1 .eq. 4096 .and. &
         d2 .eq. 2048 .and. &
         d3 .eq. 2048 .and. &
         nt .eq. 25) then
!---------------------------------------------------------------------
!   Class E size reference checksums
!---------------------------------------------------------------------
         class = 'E'
         csum_ref(1)  = dcmplx(5.121601045346D+02, 5.117395998266D+02)
         csum_ref(2)  = dcmplx(5.120905403678D+02, 5.118614716182D+02)
         csum_ref(3)  = dcmplx(5.120623229306D+02, 5.119074203747D+02)
         csum_ref(4)  = dcmplx(5.120438418997D+02, 5.119345900733D+02)
         csum_ref(5)  = dcmplx(5.120311521872D+02, 5.119551325550D+02)
         csum_ref(6)  = dcmplx(5.120226088809D+02, 5.119720179919D+02)
         csum_ref(7)  = dcmplx(5.120169296534D+02, 5.119861371665D+02)
         csum_ref(8)  = dcmplx(5.120131225172D+02, 5.119979364402D+02)
         csum_ref(9)  = dcmplx(5.120104767108D+02, 5.120077674092D+02)
         csum_ref(10) = dcmplx(5.120085127969D+02, 5.120159443121D+02)
         csum_ref(11) = dcmplx(5.120069224127D+02, 5.120227453670D+02)
         csum_ref(12) = dcmplx(5.120055158164D+02, 5.120284096041D+02)
         csum_ref(13) = dcmplx(5.120041820159D+02, 5.120331373793D+02)
         csum_ref(14) = dcmplx(5.120028605402D+02, 5.120370938679D+02)
         csum_ref(15) = dcmplx(5.120015223011D+02, 5.120404138831D+02)
         csum_ref(16) = dcmplx(5.120001570022D+02, 5.120432068837D+02)
         csum_ref(17) = dcmplx(5.119987650555D+02, 5.120455615860D+02)
         csum_ref(18) = dcmplx(5.119973525091D+02, 5.120475499442D+02)
         csum_ref(19) = dcmplx(5.119959279472D+02, 5.120492304629D+02)
         csum_ref(20) = dcmplx(5.119945006558D+02, 5.120506508902D+02)
         csum_ref(21) = dcmplx(5.119930795911D+02, 5.120518503782D+02)
         csum_ref(22) = dcmplx(5.119916728462D+02, 5.120528612016D+02)
         csum_ref(23) = dcmplx(5.119902874185D+02, 5.120537101195D+02)
         csum_ref(24) = dcmplx(5.119889291565D+02, 5.120544194514D+02)
         csum_ref(25) = dcmplx(5.119876028049D+02, 5.120550079284D+02)

      endif


      if (class .ne. 'U') then

         do i = 1, nt
            err = abs( (sums(i) - csum_ref(i)) / csum_ref(i) )
            if (.not.(err .le. epsilon)) goto 100
         end do
         verified = .TRUE.
 100     continue

      endif

      call MPI_COMM_SIZE(MPI_COMM_WORLD, size, ierr)
      if (size .ne. np) then
         write(*, 4010) np
         write(*, 4011)
         write(*, 4012)
!---------------------------------------------------------------------
! multiple statements because some Fortran compilers have
! problems with long strings.
!---------------------------------------------------------------------
 4010    format( ' Warning: benchmark was compiled for ', i5, &
                'processors')
 4011    format( ' Must be run on this many processors for official', &
                ' verification')
 4012    format( ' so memory access is repeatable')
         verified = .false.
      endif

!#ifdef CHECKSUM
!      if (class .ne. 'U') then
!         if (verified) then
!            write(*,2000)
! 2000       format(' Result verification successful')
!         else
!            write(*,2001)
! 2001       format(' Result verification failed')
!         endif
!      endif
!#endif
      print *, 'class = ', class

      return
      end

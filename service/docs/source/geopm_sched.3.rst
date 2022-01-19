.. role:: raw-html-m2r(raw)
   :format: html


geopm_sched.h(3) -- interface with Linux scheduler
==================================================






SYNOPSIS
--------

#include `<geopm_sched.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_sched.h>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``int geopm_sched_num_cpu(``\ :
  ``void);``

* 
  ``int geopm_sched_get_cpu(``\ :
  ``void);``

* 
  ``int geopm_sched_proc_cpuset(``\ :
  ``int`` _num\ *cpu*\ , :raw-html-m2r:`<br>`
  `cpu_set_t *`_proc\ *cpuset*\ ``);``

* 
  ``int geopm_sched_woomp(``\ :
  ``int`` _num\ *cpu*\ , :raw-html-m2r:`<br>`
  `cpu_set_t *`_woomp_\ ``);``

* 
  ``int geopm_sched_popen(``\ :
  `const char *`_cmd_, :raw-html-m2r:`<br>`
  `FILE **`_fid_\ ``);``

DESCRIPTION
-----------

The _geopm\ *sched.h* header defines GEOPM interfaces for interacting with
the linux scheduler.


* 
  ``geopm_sched_num_cpu``\ ():
  Returns the number of online Linux logical CPUs on the system.

* 
  ``geopm_sched_get_cpu``\ ():
  Returns the Linux logical CPU index that the calling thread is executing on.

* 
  ``geopm_sched_proc_cpuset``\ ():
  Provides a `CPU_SET(3) <http://man7.org/linux/man-pages/man3/CPU_SET.3.html>`_ bit mask identifying the Linux logical CPUs on
  which the calling process is allowed to run. The user must allocate the bit
  arrary _proc_cpu\ *set* prior to calling this function, and the number of bits
  allocated is given by the _num\ *cpu* parameter. All of the bits in _proc_cpu\ *set*
  will be zeroed except for the offsets corresponding to CPUs that the process has
  access to which will be set to one. Returns zero on success and an error
  code on failure.

* 
  ``geopm_sched_woomp``\ ():
  Sets the `CPU_SET(3) <http://man7.org/linux/man-pages/man3/CPU_SET.3.html>`_ given by *woomp* such that it includes all
  CPUs not used in an OpenMP parallel region but available to the
  calling thread.  If there are no CPU's that are part of the mask
  returned by `sched_getaffinity(2) <http://man7.org/linux/man-pages/man2/sched_getaffinity.2.html>`_ but not affinitized by an
  OpenMP thread then the returned mask will have all bits set,
  allowing the Linux scheduler to dynamically affinitize the thread.
  The cpu mask *woomp* that is created by this function can be used
  with `pthread_attr_setaffinity_np(3) <http://man7.org/linux/man-pages/man3/pthread_attr_setaffinity_np.3.html>`_ to modify the attributes
  passed to ``geopm_ctl_pthread``\ () so that the pthread created is
  affinitized to CPUs that do not have an OpenMP thread affinity.
  The mask generated when OpenMP threads are not statically
  affinitized is unreliable (i.e. use OMP_PROC_BIND environment
  variable).  The _num\ *cpu* parameter specifies size of the CPU_SET
  in terms of number of CPUs.  If an error occurs a non-zero error
  number is returned. See `geopm_error(3) <geopm_error.3.html>`_ for a full description
  of the error numbers and how to convert them to strings.

* 
  ``geopm_sched_popen``\ ():
  Enables calls to **popen(3)** within a process that is running the GEOPM
  Controller without triggering GEOPM's signal handling when the subprocess
  completes. This may be useful for built-in and plugin-loaded actors of the
  GEOPM runtime.  Unlike **popen(3)**\ , this function blocks until the opened process
  has completed, and it also suppresses the SIGCHILD signal which is raised
  when the opened process completes. The *cmd* is the shell command that is
  executed, and the *fid* is a file descriptor that provides the standard output
  from the opened process. Returns zero upon success and an error code on failure.
  Note that opening a child process will result in the MPI runtime killing the job
  with MPICH and possibly other implementations of MPI.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_ctl_c(3) <geopm_ctl_c.3.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
`CPU_SET(3) <http://man7.org/linux/man-pages/man3/CPU_SET.3.html>`_\ ,
**popen(3)**\ ,
`pthread_setaffinity_np(3) <http://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html>`_\ ,
`pthread_attr_setaffinity_np(3) <http://man7.org/linux/man-pages/man3/pthread_attr_setaffinity_np.3.html>`_\ ,
`sched_getaffinity(2) <http://man7.org/linux/man-pages/man2/sched_getaffinity.2.html>`_\ ,
`sched_getcpu(3) <http://man7.org/linux/man-pages/man3/sched_getcpu.3.html>`_

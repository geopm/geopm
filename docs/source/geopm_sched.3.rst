.. role:: raw-html-m2r(raw)
   :format: html


geopm_sched(3) -- interface with Linux scheduler
==================================================






Synopsis
--------

#include `<geopm_sched.h> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_sched.h>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c

       int geopm_sched_num_cpu(void);

       int geopm_sched_get_cpu(void);

       int geopm_sched_proc_cpuset(int num_cpu,
                                   cpu_set_t *proc_cpuset);

       int geopm_sched_woomp(int num_cpu, cpu_set_t *woomp);

Description
-----------

The `geopm_sched.h <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_sched.h>`_ header defines GEOPM interfaces for interacting with
the Linux scheduler.


``geopm_sched_num_cpu()``
  Returns the number of online Linux logical CPUs on the system.

``geopm_sched_get_cpu()``
  Returns the Linux logical CPU index that the calling thread is executing on.

``geopm_sched_proc_cpuset()``
  Provides a `CPU_SET(3) <https://man7.org/linux/man-pages/man3/CPU_SET.3.html>`_ bit mask identifying the Linux logical CPUs on
  which the calling process is allowed to run. The user must allocate the bit
  array *proc_cpuset* prior to calling this function, and the number of bits
  allocated is given by the *num_cpu* parameter. All of the bits in *proc_cpuset*
  will be zeroed except for the offsets corresponding to CPUs that the process has
  access to which will be set to one. Returns zero on success and an error
  code on failure.

``geopm_sched_woomp()``
  Sets the `CPU_SET(3) <https://man7.org/linux/man-pages/man3/CPU_SET.3.html>`_ given by *woomp* such that it includes all
  CPUs not used in an OpenMP parallel region but available to the
  calling thread.  If there are no CPU's that are part of the mask
  returned by `sched_getaffinity(2) <https://man7.org/linux/man-pages/man2/sched_getaffinity.2.html>`_ but not affinitized by an
  OpenMP thread then the returned mask will have all bits set,
  allowing the Linux scheduler to dynamically affinitize the thread.
  The CPU mask *woomp* that is created by this function can be used
  with `pthread_attr_setaffinity_np(3) <https://man7.org/linux/man-pages/man3/pthread_attr_setaffinity_np.3.html>`_ to modify the attributes
  passed to ``geopm_ctl_pthread()`` so that the pthread created is
  affinitized to CPUs that do not have an OpenMP thread affinity.
  The mask generated when OpenMP threads are not statically
  affinitized is unreliable (i.e. use ``OMP_PROC_BIND`` environment
  variable).  The *num_cpu* parameter specifies size of the ``CPU_SET``
  in terms of number of CPUs.  If an error occurs a non-zero error
  number is returned. See :doc:`geopm_error(3) <geopm_error.3>` for a full description
  of the error numbers and how to convert them to strings.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_ctl(3) <geopm_ctl.3>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`\ ,
`CPU_SET(3) <https://man7.org/linux/man-pages/man3/CPU_SET.3.html>`_\ ,
`pthread_setaffinity_np(3) <https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html>`_\ ,
`pthread_attr_setaffinity_np(3) <https://man7.org/linux/man-pages/man3/pthread_attr_setaffinity_np.3.html>`_\ ,
`sched_getaffinity(2) <https://man7.org/linux/man-pages/man2/sched_getaffinity.2.html>`_\ ,
`sched_getcpu(3) <https://man7.org/linux/man-pages/man3/sched_getcpu.3.html>`_

.. role:: raw-html-m2r(raw)
   :format: html


geopm::PlatformTopo(3) -- platform topology information
=======================================================






NAMESPACES
----------

The ``PlatformTopo`` class and ``platform_topo()`` singleton accessor
function are members of ``namespace geopm``\ , but the full names,
``geopm::PlatformTopo`` and ``geopm::platform_topo()``\ , have been
abbreviated in this manual.  Similarly, the ``std::`` namespace
specifier has been omitted from the interface definitions for the
following standard types: ``std::vector``\ , ``std::string``\ , and
``std::set``\ , to enable better rendering of this manual.

Note that ``PlatformTopo`` class is an abstract base class that the
user interacts with.  The concrete implementation, ``PlatformTopoImp``\ , is
hidden by the singleton accessor.

SYNOPSIS
--------

#include `<geopm/PlatformTopo.hpp> <https://github.com/geopm/geopm/blob/dev/src/PlatformTopo.hpp>`_\ 

Link with ``-lgeopmd``


.. code-block:: c++

       PlatformTopo &platform_topo(void);

       int PlatformTopo::num_domain(int domain_type) const = 0;

       int PlatformTopo::domain_idx(int domain_type,
                                    int cpu_idx) const = 0;

       bool PlatformTopo::is_nested_domain(int inner_domain,
                                           int outer_domain) const = 0;

       set<int> PlatformTopo::domain_nested(int inner_domain,
                                            int outer_domain,
                                            int outer_idx) const = 0;

       static string PlatformTopo::domain_type_to_name(int domain_type);

       static int PlatformTopo::domain_name_to_type(const string &domain_name);

       static void PlatformTopo::create_cache(void);

DESCRIPTION
-----------

This class describes the number and arrangement of cores, sockets,
logical CPUs, memory banks, and other components.  This information is
used when calling methods of the `geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_ interface.  The
topology of the current platform is available using the singleton
``geopm::platform_topo()``.  The remaining methods are accessed through
this singleton.

Most methods in the ``PlatformTopo`` interface return or require as an
argument an integer domain type, used to refer to different parts of
the system topology where signals and controls are applicable.  Each
domain is defined by a type and an index.  The domain type is a value
from the ``geopm_domain_e`` enum, and the domain index enumerates the
devices of that type available on the system.  Refer to the list of
domain types in `geopm_topo_c(3) <geopm_topo_c.3.html>`_.  The domains are effectively
hierarchical and the ``PlatformTopo::domain_nested()`` method can be
used to explore which domains are nested within a specified outer
domain.  Each domain, specified by pairing a domain type and a domain
index, is related to a specific set of Linux logical CPUs which reside
at the leaves of the hierarchy.  These are the set of CPUs that can
most efficiently issue instructions to read signals from or write
controls to the domain.

SINGLETON ACCESSOR
------------------


* ``platform_topo()``:
  Returns the singleton accessor for the ``PlatformTopo`` interface.

CLASS METHODS
-------------


* 
  ``num_domain()``:
  Number of domains on the platform of a particular *domain_type*.
  Refer to the list of domain types in `platform_topo_c(3) <platform_topo_c.3.html>`_.

* 
  ``domain_idx()``:
  Get the domain index for a particular *domain_type* that contains
  the given Linux logical CPU with index *cpu_idx*.

* 
  ``is_nested_domain()``:
  Check if *inner_domain* is contained within *outer_domain*.
  ``GEOPM_DOMAIN_BOARD`` is the outermost domain representing the entire
  node.  All other domains are contained within *board*.
  ``GEOPM_DOMAIN_CORE``, ``GEOPM_DOMAIN_CPU``, ``GEOPM_DOMAIN_PACKAGE_MEMORY``, and
  ``GEOPM_DOMAIN_PACKAGE_ACCELERATOR`` are contained within package.
  ``GEOPM_DOMAIN_CPU`` is contained within ``GEOPM_DOMAIN_CORE``.  The following
  outline summarizes the hierarchy of containing domains, where each
  domain is also contained in parents of its parent domain.

.. code-block::

       `GEOPM_DOMAIN_BOARD`
        +---`GEOPM_DOMAIN_PACKAGE`
             +---`GEOPM_DOMAIN_CORE`
                  +---`GEOPM_DOMAIN_CPU`
             +---`GEOPM_DOMAIN_PACKAGE_MEMORY`
             +---`GEOPM_DOMAIN_PACKAGE_NIC`
             +---`GEOPM_DOMAIN_PACKAGE_ACCELERATOR`
        +---`GEOPM_DOMAIN_BOARD_MEMORY`
        +---`GEOPM_DOMAIN_BOARD_NIC`
        +---`GEOPM_DOMAIN_BOARD_ACCELERATOR`


* 
  ``domain_nested()``:
  Returns the set of smaller domains of type *inner_domain*
  contained with a larger domain of type *outer_domain* at
  *outer_idx*.  If the inner domain is not the same as or contained
  within the outer domain, it throws an exception.

* 
  ``domain_type_to_name()``:
  Convert a *domain_type* integer to a string.  These strings are
  used by the `geopmread(1) <geopmread.1.html>`_ and `geopmwrite(1) <geopmwrite.1.html>`_ tools.

* 
  ``domain_name_to_type()``:
  Convert a *domain_name* string to the corresponding integer domain type.
  This method is the inverse of ``domain_type_to_name()``.

* 
  ``create_cache()``:
  Create cache file in ``tmpfs`` that can be read instead of ``popen()`` call.

EXAMPLES
--------

The following example program queries the ``PlatformTopo`` to calculate various
information of interest about the platform.

.. code-block:: c++

       #include <iostream>

       #include <geopm/PlatformTopo.hpp>

       using geopm::PlatformTopo;

       int main() {
           const PlatformTopo &topo = geopm::platform_topo();

           int num_cores = topo.num_domain(GEOPM_DOMAIN_CORE);
           int num_cpus = topo.num_domain(GEOPM_DOMAIN_CPU);
           int num_pkgs = topo.num_domain(GEOPM_DOMAIN_PACKAGE);

           // Print counts of various domains
           std::cout << "Domain      Count      " << std::endl;
           std::cout << "-----------------------" << std::endl;
           std::cout << "cores       " << num_cores << std::endl;
           std::cout << "packages    " << num_pkgs << std::endl;
           std::cout << "core/pkg    " << num_cores / num_pkgs << std::endl;
           std::cout << "cpu/core    " << num_cpus / num_cores << std::endl;
           std::cout << "cpu/pkg     " << num_cpus / num_pkgs << std::endl;
       }

For example, when run on a system with 2 sockets, 4 cores per socket,
and 3 hyperthreads per core, the following would be printed to
standard output:

.. code-block::

       Domain      Count
       -----------------------
       cores       8
       packages    2
       core/pkg    4
       cpu/core    3
       cpu/pkg     12

This loop, inserted into the above program, prints the Linux CPUs on each package:

.. code-block:: c++

       for (int pkg_idx = 0; pkg_idx < num_pkgs; ++pkg_idx) {
           std::cout << "CPUs on package " << pkg_idx << ": ";
           std::set<int> cpus = topo.domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_PACKAGE, pkg_idx);
           for(auto pcpu : cpus) {
               std::cout << pcpu << " ";
           }
           std::cout << std::endl;
       }

The output for the same system would be:

.. code-block::

   CPUs on package 0: 0 1 2 3 8 9 10 11 16 17 18 19
   CPUs on package 1: 4 5 6 7 12 13 14 15 20 21 22 23


To check which logical CPUs are on the same core as CPU 1:

.. code-block:: c++

       int my_cpu = 8;
       int cpu_core = topo.domain_idx(GEOPM_DOMAIN_CORE, my_cpu);
       std::set<int> core_cpu_set = topo.domain_nested(GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CORE, cpu_core);
       for (auto cpu : core_cpu_set) {
           if (cpu != my_cpu) {
               std::cout << cpu << " ";
           }
       }
       std::cout << std::endl;

The output for the same system would be:

.. code-block::

   0 16

The number of domains can also be use to check if a hardware feature, such as
on-package memory, is present or absent:

.. code-block:: c++

       if (topo.num_domain(GEOPM_DOMAIN_PACKAGE_MEMORY) > 0) {
           std::cout << "On-package memory is present." << std::endl;
       }
       else {
           std::cout << "No on-package memory." << std::endl;
       }

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_pio_c(3) <geopm_pio_c.3.html>`_\ ,
`geopm_topo_c(3) <geopm_topo_c.3.html>`_\ ,
`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_

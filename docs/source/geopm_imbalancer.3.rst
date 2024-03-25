
geopm_imbalancer(3) -- set artificial runtime imbalance
=========================================================


Synopsis
--------

#include `<geopm_imbalancer.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_imbalancer.h>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c

       int geopm_imbalancer_frac(double frac);

       int geopm_imbalancer_enter(void);

       int geopm_imbalancer_exit(void);

Description
-----------

The `geopm_imbalancer.h <https://github.com/geopm/geopm/blob/dev/src/geopm_imbalancer.h>`_ header defines interfaces for accessing the
imbalancer singleton, which is an object used to add artificial
imbalance to regions as a fraction of the total region runtime.  In
particular, it is used by the ``ModelRegion`` of the ``geopmbench``
application to simulate imbalance between nodes.  If the environment
variable ``IMBALANCER_CONFIG`` is set to a file path, the file will be
searched for the hostname of the current node and the imbalance
fraction will be set to the given value.  This file should be
formatted with a hostname and double fraction on each line, separated
by a space.


``geopm_imbalancer_frac()``
  Used to set a delay *frac* that will sleep for the given fraction
  of the region runtime.

``geopm_imbalancer_enter()``
  Sets the entry time for the imbalanced region.

``geopm_imbalancer_exit()``
  Spins until the region has been extended by the previously specified delay.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopmbench(1) <geopmbench.1>`

geopmbench(1) -- synthetic benchmark application
================================================

Synopsis
--------

.. code-block:: bash

       geopmbench [--verbose] [CONFIG_FILE]

Description
-----------


The command line tool ``geopmbench``, an example hybrid MPI/OpenMP application,
contains several selectable regions with different runtime characteristics.
``geopmbench`` can be used as a synthetic benchmark to test the behavior of
plugins to GEOPM, or as an integration test of the installation. It is also
used in the tutorials. The optional configuration passed as ``CONFIG_FILE`` can
be used to adjust the number and type of regions, problem size of each region
(referred to as ``big-o``), and number of iterations to run the entire
application. The ``big-o`` value for each of the regions is designed such that
a ``big-o`` of ``1.0`` will run on the order of one second, and the runtime
will scale linearly with ``big-o``. The correct base value for one second of
runtime varies between platforms, so the ``big-o`` should be tuned to achieve
the desired runtime on a platform. The regions can also be configured to run
with imbalance on different nodes.

Unless otherwise noted, the hint used is ``GEOPM_REGION_HINT_UNKNOWN``.

Region names can be one of the following options:

* *sleep*: Executes ``clock_nanosleep()`` for ``big-o`` seconds.

* *spin*: Executes a spin loop for ``big-o`` seconds.

* *dgemm*: Dense matrix-matrix multiply with floating point operations
  proportional to ``big-o``.  ``GEOPM_REGION_HINT_COMPUTE`` is set for this
  region.

* *stream*: Executes stream "triad" on a vector with length proportional to
  ``big-o``.  ``GEOPM_REGION_HINT_MEMORY`` is set for this region.

* *all2all*: All processes send buffers to all other processes. The time of
  this operation is proportional to ``big-o``.

* *ignore*: Sleeps for a number of seconds equal to the ``big-o``.
  ``GEOPM_REGION_HINT_IGNORE`` is set for this region.

* *scaling*: Executes an arithmetic operation where ``big-o``
  is a linear function of CPU frequency.  Designed to fill the LLC.
  ``GEOPM_REGION_HINT_MEMORY`` is set for this region.

* *barrier*: Executes MPI_Barrier(MPI_COMM_WORLD).  ``big-o`` has no effect.

* *reduce*: Executes MPI_Allreduce on a buffer sized by ``big-o``.  A ``big-o``
  of 1 will produce a 64MiB buffer.

* *timed_scaling*: Like the *scaling* region but termintates based on elapsed
  time as opposed to finishing the calculation.

Of these regions, *dgemm* exhibits the most compute-intensive behavior and will be
sensitive to frequency, while *stream* is memory-intensive and is less sensitive
to CPU frequency. *all2all* represents a network-intensive region.

The JSON config file must follow this schema:

.. literalinclude:: ../../../json_schemas/geopmbench_config.schema.json
    :language: json

Examples
--------

Use :doc:`geopmlaunch(1) <geopmlaunch.1>` to launch ``geopmbench`` with a given
configuration provided as a command line argument:

.. code-block:: bash

   geopmlaunch srun -N 2 -n 32 -c 4 --geopm-ctl=process \
                                    --geopm-report=bench.report \
                                    -- geopmbench config.json


The config file is a JSON file containing the loop count and sequence of
regions in each loop.

Example configuration JSON string:

.. code-block:: json

   {
     "loop-count": 10,
     "region": ["sleep", "stream", "dgemm", "stream", "all2all"],
     "big-o": [1.0, 1.0, 1.0, 1.0, 1.0]
   }

The ``"loop-count"`` value is an integer that sets the number of loops
executed.  Each time through the loop the regions listed in the ``"region"``
array are executed.  The ``"big-o"`` array gives double precision values for
each region.

Example configuration JSON string with imbalance:

.. code-block:: json

   {
     "loop-count": 10,
     "region": ["sleep", "stream", "dgemm-imbalance", "stream", "all2all"],
     "big-o": [1.0, 1.0, 1.0, 1.0, 1.0],
     "hostname": ["compute-node-3", "compute-node-15"],
     "imbalance": [0.05, 0.15]
   }

If ``"-imbalance"`` is appended to any region name in the configuration file
and the ``"hostname"`` and ``"imbalance"`` fields are provided then those
regions will have an injected delay on the hosts listed.  In the above example
a 5% delay on ``"my-compute-node-3"`` and a 15% delay on
``"my-compute-node-15"`` are injected when executing the *dgemm* region.

If ``"-progress"`` is appended to any region name in the configuration, then
progress for the region will be reported through the ``geopm_tprof_*()`` API.

If ``"-unmarked"`` is appended to any region name in the configuration, then
the region will **not** be marked with the ``geopm_prof_*()`` API calls for
``enter()`` and ``exit()`` calls.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_prof(3) <geopm_prof.3>`,
:doc:`geopmlaunch(1) <geopmlaunch.1>`

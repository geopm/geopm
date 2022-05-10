.. role:: raw-html-m2r(raw)
   :format: html


geopmbench(1) -- synthetic benchmark application
================================================






SYNOPSIS
--------

``geopmbench`` [_CONFIG\ *FILE*\ ]

DESCRIPTION
-----------

The GEOPM model application is provided as an example application
containing several marked regions with different runtime characteristics.
It can be used as a synthetic benchmark to test the behavior of plugins to GEOPM,
or as an integration test of the installation. It is also used in the
tutorials. The optional configuration passed as _CONFIG\ *FILE* can be used to adjust the
number and type of regions, problem size of each region (referred to as big-o),
and number of iterations to run the entire application. The big-o value for
each of the regions is designed such that a big-o of 1.0 will run on the order of one second,
and the runtime will scale linearly with big-o. The correct base value for one second
of runtime varies between platforms, so the big-o should be tuned to achieve the
desired runtime on a platform. The regions can also be configured to run with imbalance
on different nodes.

Region names can be one of the following options:


* 
  *sleep*\ :
  Executes clock_nanosleep() for big-o seconds.

* 
  *spin*\ :
  Executes a spin loop for big-o seconds.

* 
  *dgemm*\ :
  Dense matrix-matrix multiply with floating point operations proportional to big-o.

* 
  *stream*\ :
  Executes stream "triad" on a vector with length proportional to big-o.

* 
  *all2all*\ :
  All processes send buffers to all other processes. The time of this operation is
  proportional to big-o.

* 
  *nested*\ :
  Executes *spin*\ , *all2all*\ , and *spin* within a single region.

* 
  *ignore*\ :
  Sleeps for a number of seconds equal to the big-o.

Of these regions, *dgemm* exhibits the most compute-intensive behavior and will be
sensitive to frequency, while *stream* is memory-intensive and is less sensitive
to CPU frequency. *all2all* represents a network-intensive region.

The JSON config file must follow this schema:

.. literalinclude:: ../../../json_schemas/geopmbench_config.schema.json
    :language: json

EXAMPLES
--------

Use `geopmlaunch(1) <geopmlaunch.1.html>`_ to launch geopmbench with a given configuration provided as
a command line argument:

.. code-block::

   geopmlaunch srun -N 2 -n 32 -c 4 --geopm-ctl=process \
                                    --geopm-report=bench.report \
                                    -- geopmbench config.json


The config file is a JSON file containing the loop count and sequence of regions in each loop.

Example configuration JSON string:

 {"loop-count": 10,\ :raw-html-m2r:`<br>`
 "region": ["sleep", "stream", "dgemm", "stream", "all2all"],\ :raw-html-m2r:`<br>`
 "big-o": [1.0, 1.0, 1.0, 1.0, 1.0]}

The "loop-count" value is an integer that sets the
number of loops executed.  Each time through the loop
the regions listed in the "region" array are
executed.  The "big-o" array gives double precision
values for each region.

Example configuration JSON string with imbalance:

 {"loop-count": 10,\ :raw-html-m2r:`<br>`
 "region": ["sleep", "stream", "dgemm-imbalance", "stream", "all2all"],\ :raw-html-m2r:`<br>`
 "big-o": [1.0, 1.0, 1.0, 1.0, 1.0],\ :raw-html-m2r:`<br>`
 "hostname": ["compute-node-3", "compute-node-15"],\ :raw-html-m2r:`<br>`
 "imbalance": [0.05, 0.15]}

If "-imbalance" is appended to any region name in
the configuration file and the "hostname" and
"imbalance" fields are provided then those
regions will have an injected delay on the hosts
listed.  In the above example a 5% delay on
"my-compute-node-3" and a 15% delay on
"my-compute-node-15" are injected when executing
the dgemm region.

If "-progress" is appended to any region name in the
configuration, then progress for the region will be
reported through the geopm\ *tprof*\ * API.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_\ ,

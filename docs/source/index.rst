.. GEOPM documentation main file


GEOPM
=====

**Fine-grained low-latency batch access to power metrics and control knobs on Linux**

Key Features
------------

The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes of
computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.

----

Source Code
-----------

To get the latest source code clone the git repository:

.. code-block:: bash

    $ git clone https://github.com/geopm/geopm.git

.. image:: https://geopm.github.io/images/github-button.png
   :target: https://github.com/geopm/geopm
   :width: 200
   :alt: View on Github

----

With GEOPM, a system administrator can:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

|:closed_lock_with_key:| Provide secure access to hardware telemetry and controls
  - Grant per-user or per-group access to individual metrics and controls, even
    when their underlying interfaces do not offer fine-grained access control
|:safety_vest:| Provide safe access to hardware settings
  - Ensure that user-driven changes to hardware settings are reverted when the
    user's process session terminates
|:building_construction:| Develop new plugins
  - Develop your own platform-specific monitor and control interfaces through
    the extensible plugin architecture

With GEOPM, an end user can:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

|:microscope:| Gather hardware telemetry and set hardware configuration
  - Interact with hardware settings and sensors (e.g., set a CPU power limit or
    read a GPU's current power consumption) using a platform-agnostic interface
|:stopwatch:| Gather data for analysis
  - Generate summarized reports of power and energy behavior during execution
    of an application
  - Automatically detect MPI and OpenMP phases in an application, generating
    per-phase summaries within application reports
|:checkered_flag:| Implement optimization objectives
  - Optimize applications to improve energy efficiency or reduce the
    effects of work imbalance, system jitter, and manufacturing variation
    through built-in control algorithms
  - Develop your own runtime control algorithms through the extensible
    plugin architecture
|:race_car:| Efficiently gather large amounts of data
  - Gather large groups of signal-reads or control-writes into batch
    operations, often reducing total latency to complete the operations.

----

Documentation
-------------

.. toctree::
   :maxdepth: 1

   overview
   publications
   service
   runtime
   contrib
   devel
   reference

License: BSD 3-Clause
---------------------

.. literalinclude:: ../LICENSE-BSD-3-Clause
  :language: none

`SPDX Details for BSD 3-Clause License <https://spdx.org/licenses/BSD-3-Clause.html>`_

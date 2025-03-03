.. GEOPM documentation main file


GEOPM
=====

**Fine-grained low-latency batch access to power metrics and control knobs on Linux**

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

|:closed_lock_with_key:| Utilize fine-grained access management
  - Grant per-user or per-group access to individual metrics and controls
  - Gain more granular management of features than the underlying OS access
    mechanisms provide
|:safety_vest:| Provide safe access to hardware settings
  - Ensure that user-driven changes to hardware settings are reverted when the
    user's process session terminates
|:building_construction:| Extend the open-core framework
  - Develop your own platform-specific monitor and control interfaces through
    the extensible plugin architecture

With GEOPM, an end user can:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

|:microscope:| Interact with hardware settings and sensors
  - Platform-agnostic interface
  - Support for a wide range of Linux device drivers
  - Examples: set a CPU power limit, read a GPU's current power consumption,
    limit the frequency range of system sub-components
|:stopwatch:| Measure application performance
  - Generate summarized reports of power and energy behavior during execution of
    an application
  - Automatically detect MPI and OpenMP phases in an application, generating
    per-phase summaries within application reports
|:checkered_flag:| Implement optimization objectives
  - Optimize applications to improve energy efficiency or reduce the
    effects of work imbalance, system jitter, and manufacturing variation
    through built-in control algorithms
  - Develop your own runtime control algorithms through the extensible
    plugin architecture
|:race_car:| Build fast efficient software
  - Gather large groups of signal-reads or control-writes into batch
    operations
  - Batch interface often reduces total latency to complete the operations

----

Documentation
-------------

.. toctree::
   :maxdepth: 1

   overview
   install
   user_guides
   contrib
   devel
   publications
   reference
   releases

----

License: BSD 3-Clause
---------------------

.. literalinclude:: ../LICENSE-BSD-3-Clause
  :language: none

`SPDX Details for BSD 3-Clause License <https://spdx.org/licenses/BSD-3-Clause.html>`_

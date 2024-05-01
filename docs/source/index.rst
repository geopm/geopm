.. GEOPM Service documentation main file


GEOPM: Global Extensible Open Power Manager
===========================================

**Fine-grained low-latency batch access to power metrics and control knobs on Linux**

Key Features
------------

The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes of
computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.

**With GEOPM, a system administrator can:**

  - Grant per-user or per-group access to individual metrics and controls, even
    when their underlying interfaces do not offer fine-grained access control
  - Ensure that user-driven changes to hardware settings are reverted when the
    user's process session terminates
  - Develop your own platform-specific monitor and control interfaces through
    the extensible plugin architecture

**With GEOPM, an end user can:**

  - Interact with hardware settings and sensors (e.g., set a CPU power limit or
    read a GPU's current power consumption) using a platform-agnostic interface
  - Generate summarized reports of power and energy behavior during execution
    of an application
  - Automatically detect MPI and OpenMP phases in an application, generating
    per-phase summaries within application reports
  - Optimize applications to improve energy efficiency or reduce the
    effects of work imbalance, system jitter, and manufacturing variation
    through built-in control algorithms
  - Develop your own runtime control algorithms through the extensible
    plugin architecture
  - Gather large groups of signal-reads or control-writes into batch
    operations, often reducing total latency to complete the operations.

Source Code
-----------

To get the latest source code clone the git repository:

.. code-block:: bash

    $ git clone https://github.com/geopm/geopm.git

.. image:: https://geopm.github.io/images/github-button.png
   :target: https://github.com/geopm/geopm
   :width: 200
   :alt: View on Github

Documentation
-------------

.. toctree::
   :maxdepth: 1

   overview
   service
   runtime
   contrib
   devel
   reference

License
-------

.. literalinclude:: ../COPYING
  :language: none

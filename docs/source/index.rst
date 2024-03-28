.. GEOPM Service documentation main file

Welcome to GEOPM
================

The Global Extensible Open Power Manager (GEOPM) serves as a framework for
investigating energy and power optimizations geared towards heterogeneous
platforms. It offers a whole range of built-in functions. A basic use case
could be reading hardware counters and configuring hardware controls using a
command line tool on a specific compute node with platform independent syntax.
On the other hand, an advanced example includes dynamically synchronizing
hardware settings across all compute nodes of a distributed application in
response to the application's behavior and requests from the resource manager.

.. image:: https://geopm.github.io/images/github-button.png
   :target: https://github.com/geopm/geopm
   :width: 200
   :alt: View on Github

To get the latest source code clone the git repository:

.. code-block:: bash

    $ git clone https://github.com/geopm/geopm.git

.. toctree::
   :maxdepth: 1

   overview
   service
   runtime
   contrib
   devel
   reference

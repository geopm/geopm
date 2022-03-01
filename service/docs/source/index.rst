.. GEOPM Service documentation main file

Welcome to GEOPM
================


The Global Extensible Open Power Manager (GEOPM) is a framework for exploring
power and energy optimizations targeting heterogenous platforms. The GEOPM
package provides many built-in features. A simple use case is reading hardware
counters and setting hardware controls with platform independent syntax using
a command line tool on a particular compute node. An advanced use case is
dynamically coordinating hardware settings across all compute nodes used by a
distributed application is response to the application's behavior and requests
from the resource manager.


.. image:: https://geopm.github.io/images/github-button.png
   :target: https://github.com/geopm/geopm
   :width: 200
   :alt: View on Github

For access to the latest source code clone the git repo:

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

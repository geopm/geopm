.. role:: raw-html-m2r(raw)
   :format: html


geopmpy(7) -- global extensible open power manager python package
=================================================================






DESCRIPTION
-----------

An extension to the Global Extensible Open Power Manager (GEOPM), the
geopmpy package provides several command line tools and other
infrastructure modules for both setting up and launching a job
utilizing GEOPM and post-processing the profiling data that is output
from GEOPM.  Presently the following command line tools are provided:

**geopmlaunch**\ :raw-html-m2r:`<br>`
This script will invoke the GEOPM launcher for either the ALPS or SLURM
resource managers.  See `geopmlaunch(1) <geopmlaunch.1.html>`_ for more information.

In addition, there is 1 infrastructure module provided:

**io.py**\ :raw-html-m2r:`<br>`
This module provides tools for parsing and encapsulating report and trace data
into either simple structures or Pandas DataFrames.  It can be used to parse
any number of files, and houses structures that can be queried for said data.
This module also houses certain analysis functions in the Trace class for
extracting specific data.  See the in-file docstrings for more info.

INSTALLATION
------------


* 
  ``Source Builds``\ :
  Building GEOPM with the build instructions posted on the GitHub site will put
  the Python scripts in either the system path for Python, or in a subdirectory
  of the "--prefix" path.  See the [ENVIRONMENT][] section for more information on
  how to setup this configuration.

* 
  ``Via PyPI``\ :
  The geopmpy package can be installed via pip from PyPI with:

    sudo pip install geopmpy\ :raw-html-m2r:`<br>`\ :raw-html-m2r:`<br>`

    OR for an individual user install (not system wide)\ :raw-html-m2r:`<br>`\ :raw-html-m2r:`<br>`

    pip install --user geopmpy

ENVIRONMENT
-----------


* 
  ``PYTHONPATH`` and ``PATH``\ :
  If you are installing GEOPM into the system's default paths for Python, etc.
  then there is nothing to be done here.  Otherwise, if you are using
  "--prefix=\ :raw-html-m2r:`<PREFIX_PATH>`\ " when you run configure then you must set your
  ``PYTHONPATH`` to the location of the built site-packages directory. For
  example, the PYTHONPATH for a python 3.6 build may look like:

    export PYTHONPATH=\ :raw-html-m2r:`<PREFIX_PATH>`\ /lib/python3.6/site-packages:${PYTHONPATH}

  You must also set your ``PATH`` variable to:

    export PATH=\ :raw-html-m2r:`<PREFIX_PATH>`\ /bin:${PATH}

  It is recommended to do this in your login script (e.g. .bashrc).

TROUBLESHOOTING
---------------

If you have an existing clone of the GEOPM GitHub repo and are experiencing
a pkg_resources.DistributionNotFound error when attempting to run the Python
scripts, please remove the VERSION file at the root of your repo and re-run
autogen.sh.

The version file will be removed if the ``dist-clean`` Makefile target is invoked.
This is also remedied by rerunning autogen.sh.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_\ ,

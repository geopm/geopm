geopmpy
========
This directory contains source code for the `geopmpy` Python package, which
provides python bindings for `libgeopm`, as well as the `geopmlaunch` tool.

Subdirectories include:

* [debian](debian): Configuration files for debian packaging scripts
* [geopmpy](geopmpy): Source code for modules in this package
* [test](test): Test code for this package

Set Up a Development Environment
--------------------------------
Run `pip install -e .` to install this directory in editable mode (so you don't need to reinstall between updating python source code and re-running tests). That will also install the python execution-time dependencies of this package. Also run `pip install -f requirements.txt` to install additional development dependencies. If you follow these steps, then you do not need to modify your `PYTHONPATH` variable when executing tests.

The `geopmpy` package depends on `geopmdpy`, so also be sure to install that package (e.g., in editable mode as done for this package).

The `geopmpy` package wraps `libgeopm`, which also depends on `libgeopmd`. Be sure to follow the build instructions in the directories for those libraries as part of setting up a geopmpy development environment. Either install those builds somewhere on your path, or manually add them to your `LD_LIBRARY_PATH` (examples in the next section).

Building Against Non-System-Installed Libraries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you want to build geopmpy against non-system-installed headers and ``libgeopm``
binaries, you need to set your compiler's environment variables to tell it
where to search for GEOPM. For example, if you built and installed ``libgeopm``
with ``--prefix=$HOME/build/geopm`` and your python extensions are compiled
with gcc, then run:

    CC=gcc LIBRARY_PATH=$HOME/build/geopm/lib C_INCLUDE_PATH=$HOME/build/geopm/include pip install ./

to build and install this package.

Executing Tests
---------------
Run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 test` from this directory to launch the entire test suite. Some of the tests depend on `libgeopm`, so it should be built before running tests.
Alternatively, run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 -m unittest discover -p 'Test*.py'`

Execute a single test case with `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 -m unittest <one.or.more.test.modules.or.classes.or.functions>`. For example:
`LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 -m unittest test.TestAgent.TestAgent.test_policy_names`

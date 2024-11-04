# geopmdpy

This directory contains source code for the `geopmdpy` Python package, which
provides python bindings for `libgeopmd`, as well as the `geopmd` daemon,
`geopmaccess` tool for administrators, and the `geopmsession` tool for end
users.

Subdirectories include:

* [debian](debian): Configuration files for debian packaging scripts
* [geopmdpy](geopmdpy): Source code for modules in this package
* [test](test): Test code for this package

## Set Up a Development Environment

Run `pip install .` to install this package. Optionally use the `-e`
installation option to install in editable mode (so you don't need to reinstall
between updating python source code and re-running tests). The install command
will also install the python execution-time dependencies of this package.

Also run `pip install -f requirements.txt` to install additional development
dependencies. If you follow these steps, then you do not need to modify your
`PYTHONPATH` variable when executing tests.

The `geopmdpy` package wraps `libgeopmd`. Be sure to follow the build
instructions in the directory for that library as part of setting up a geopmdpy
development environment. Either install that builds somewhere on your path, or
manually add it to your `LD_LIBRARY_PATH` (examples in the next section).

### Building Against Non-System-Installed Libraries

If you want to build geopmdpy against non-system-installed headers and ``libgeopmd``
binaries, you need to set your compiler's environment variables to tell it
where to search for GEOPM. For example, if you built and installed ``libgeopmd``
with ``--prefix=$HOME/build/geopm`` and your python extensions are compiled
with gcc, then run:

    CC=gcc LIBRARY_PATH=$HOME/build/geopm/lib C_INCLUDE_PATH=$HOME/build/geopm/include pip install ./

to build and install this package.

### Helper Script to Build Latest Development Snapshot

A script called `install_user.sh` is provided which builds and installs
libgeopmd and geopmdpy based on the latest commit to the `dev` branch in the
upstream geopm Git repository.  This script will make a new clone of the geopm
git repository in your current working directory, and so this script is a
stand-alone solution:

```
    wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
    chmod a+x install_user.sh
    ./install_user.sh --prefix=$HOME/geopm-build --enable-levelzero --disable-io-uring --disable-libcap

```

See the output from `./install_user.sh --help` for more information.

## Executing Tests

Run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" python3 test`
from this directory to launch the entire test suite. Some of the tests depend
on `libgeopmd`, so it should be built before running tests.  Alternatively, run
`LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" python3 -m unittest discover -p 'Test*.py'`

Execute a single test case with
`LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python -m unittest <one.or.more.test.modules.or.classes.or.functions>`.

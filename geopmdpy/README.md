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

The `geopmdpy` package wraps `libgeopmd`. Be sure to follow the build
instructions in the directory for that library as part of setting up a `geopmdpy`
development environment. Either install `libgeopmd` somewhere on your path, or
manually add it to your `LD_LIBRARY_PATH` (examples in the next section).

### Building Against Non-System-Installed Libraries

If you want to build `geopmdpy` against non-system-installed headers and ``libgeopmd``
binaries, you need to set your compiler's environment variables to tell it
where to search for GEOPM. For example, if you built and installed ``libgeopmd``
with ``--prefix=$HOME/build/geopm`` and your python extensions are compiled
with gcc, then run:

```bash
CC=gcc \
LIBRARY_PATH=$HOME/build/geopm/lib \
C_INCLUDE_PATH=$HOME/build/geopm/include \
python3 -m pip install ./

```

to build and install this package.

### User Build/Install Helper Script for geopmdpy and libgeopmd

A script called `install_user.sh` is provided which builds and installs
`libgeopmd` and `geopmdpy` for a single user.  This script may be a simple helper for
running the build of the two directories `geopm/geopmdpy` and `geopm/libgeopmd`, but
it may also be used as a stand-alone solution in place of:

```bash
python3 -m pip install "geopmdpy @ git+https://github.com/geopm/geopm.git#subdirectory=geopmdpy"

```

The above command will install the `geopmdpy` package from the dev branch on
GitHub, but it will not install a development snapshot of `libgeopmd` which may
also be required.  To combine these two installation steps, run the following
commands instead:

```bash
wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
chmod a+x install_user.sh
export GEOPM_GIT_PATH=$(mktemp -d)/geopm
./install_user.sh --prefix=$HOME/geopm-build --enable-levelzero
rm -rf $(dirname $GEOPM_GIT_PATH)
export LD_LIBRARY_PATH=$HOME/geopm-build/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

```

Note that the options provided to `install_user.sh` above are one example.  See
the output from `./install_user.sh --help` for more information about how to use
the script.

## Executing Tests

Run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" python3 test`
from this directory to launch the entire test suite. Some of the tests depend
on `libgeopmd`, so it should be built before running tests.  Alternatively, run
`LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" python3 -m unittest discover -p 'Test*.py'`

Execute a single test case with
`LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 -m unittest <one.or.more.test.modules.or.classes.or.functions>`.

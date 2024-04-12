libgeopmd Directory
-------------------
C and C++ source files for libgeopmd and associated command line tools.
Directories include:

* [contrib](contrib): Third-party contributed code
* [debian](debian): Configuration files for debian packaging scripts
* [fuzz_test](fuzz_test): Corpus and test automation for fuzz tests
* [include](include): Public headers installed for use with `libgeopmd`
* [src](src): Source code, including headers that do not get installed
* [test](test): Test code for `libgeopmd`

Building
========
Steps to build `libgeopmd` are as follows:

1. This library is built with autotools, and may be distributed with an
   autoconf script. If you haven't been provided a `./configure` script (e.g.,
   if you cloned the repository yourself), then generate one by running
   `./autogen.sh`. 
2. Run `./configure --prefix=$HOME/build/geopm` (or modify the prefix to
   wherever you want to install GEOPM). Additional options are shown when you
   run `./configure --help`.
3. Run `make -j` to build libgeopm and other build outputs. The library will be
   written to the `./.libs` directory.
4. Run `make install` to copy the build outputs to their installation
   destinations.

Testing
-------
Run ``make checkprogs -j`` to build the test suite.

Run ``make check`` to run the full test suite. Basic pass/fail information is printed to the screen. Detailed test logs are written to ``test-suite.log``.

Run a subset of tests by using [gtest filters](https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests). For example, to run only ``HelperTest`` test cases, run ``GTEST_FILTER='HelperTest*' make check``.

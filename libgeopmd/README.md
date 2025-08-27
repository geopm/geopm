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

Tip: configure with ``--enable-debug`` to speed up incremental builds and enable more verbose logging during development.

PMTIOGroup (Intel PMT telemetry)
================================
libgeopmd includes an IOGroup that exposes read-only signals from the Linux ``intel_pmt`` telemetry driver. At runtime it discovers the PMT telem device via sysfs (``/sys/class/intel_pmt/telem*``), prefers the Sapphire Rapids PM GUID, and loads the corresponding Intel-PMT XML to enumerate counters.

- XML location: set ``GEOPM_PMT_XML_ROOT`` to the root of the Intel-PMT XML tree (the directory containing ``pmt.xml``). If unset, a minimal fallback for Sapphire Rapids IDI bandwidth is used when available.
- Signals: exposes per-CPU IDI bandwidth counters (C2U and U2C) for Ports 0–3 when defined by the XML, plus composite totals and explicit rate (derivative) forms:
  - Raw counters: ``PMT::IDI_PORT{0..3}_{C2U|U2C}_BW``
  - Port rates: ``PMT::IDI_PORT{0..3}_{C2U|U2C}_BW_RATE``
  - Totals across ports: ``PMT::IDI_{C2U|U2C}_BW_TOTAL`` and ``PMT::IDI_{C2U|U2C}_BW_TOTAL_RATE``
- Domains: all PMT signals are exposed at the CPU domain.
- Behaviors and aggregation:
  - Raw counters and totals: behavior MONOTONE; aggregate with sum.
  - Rates: behavior VARIABLE; aggregate with average.

Rollover and direct-rate behavior
---------------------------------
- Counter rollover: raw IDI counters are extended across wrap using a per-CPU rollover accumulator (width-driven; 64-bit fields are treated as no-op). Totals and rate signals are computed from these extended values.
- Direct rate reads: when not using batch, rate signals perform a short two-read finite difference. The sleep can be tuned with ``GEOPM_PMT_DIRECT_RATE_SLEEP_NS`` (default 5ms).

Note: This IOGroup provides signals only (no controls) and is always registered alongside other IOGroups.

Parallel Region ID Repeatability Test
=====================================

This directory contains a test for the ompt parallel id feature of the
parallel region callback.  The test determines if calling into the
same OpenMP parallel region results in the same parallel region ID.
This test fails with LLVM version 4.0.0.

To run the test have icc and icpc in your path and execute the
command:

    make check

This will download LLVM 4.0.0, build the OpenMP runtime with OMPT
enabled and run the test.

TODO: Stub for libgeopmd directory README

Testing
-------
Run ``make checkprogs`` to build the test suite.

Run ``make check`` to run the full test suite. Basic pass/fail information is printed to the screen. Detailed test logs are written to ``test-suite.log``.

Run a subset of tests by using `gtest filters <https://google.github.io/googletest/advanced.html#running-a-subset-of-the-tests>`_. For example, to run only ``HelperTest`` test cases, run ``GTEST_FILTER='HelperTest*' make check``.

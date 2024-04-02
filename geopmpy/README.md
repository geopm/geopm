GEOPM - Global Extensible Open Power Manager Runtime Tools

# Testing
Run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 test` from this directory to launch the entire test suite. Some of the tests depend on `libgeopm`, so it should be built before running tests.
Alternatively, run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python3 -m unittest discover -p 'Test*.py'`

Execute a single test case with `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python -m unittest <one.or.more.test.modules.or.classes.or.functions>`. For example:
`LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python -m unittest test.TestAgent.TestAgent.test_policy_names`

GEOPM - Global Extensible Open Power Manager Daemon

# Set Up a Development Environment
Run `pip install -e .` to install this directory in editable mode (so you don't need to reinstall between updating python source code and re-running tests). That will also install the python execution-time dependencies of this package. Also run `pip install -f requirements.txt` to install additional development dependencies. If you follow these steps, then you do not need to modify your `PYTHONPATH` variable when executing tests.

The `geopmdpy` package wraps `libgeopmd`. Be sure to follow the build instructions in the directory for that library as part of setting up a geopmdpy development environment. Either install that builds somewhere on your path, or manually add it to your `LD_LIBRARY_PATH` (examples in the next section).

# Executing Tests
Run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" python3 test` from this directory to launch the entire test suite. Some of the tests depend on `libgeopmd`, so it should be built before running tests.
Alternatively, run `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopmd/.libs" python3 -m unittest discover -p 'Test*.py'`

Execute a single test case with `LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/../libgeopm/.libs" python -m unittest <one.or.more.test.modules.or.classes.or.functions>`.

GEOPM Integration Tests
=======================

This directory contains the integration tests for the GEOPM package.
These tests are implemented by wrapping the ModelApplication class
introduced in tutorial 6 with a Python unittest framework.  The
geopm_test_integration.py wrapper generates input configuration files,
executes the geopm_test_integration model application one or more
times and checks that the GEOPM report and trace files demonstrate
features of the GEOPM runtime.

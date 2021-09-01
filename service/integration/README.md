Testing for the GEOPM Service
=============================

This directory provides a set of integration test scripts showing
examples of interacting with the GEOPM service using the `geopmaccess`
and `geopmsession` command line tools.  These tests are written in
bash and serve the purpose of testing a fully installed GEOPM service
using only software from the service subdirectory of the GEOPM
reposistory.  The tests in this directory also show examples of how to
use each of the features provided by the service.  The tests each
begin with a long comment describing the feature under test, and
provide a form of tutorial for an end user learning how to interact
with the GEOPM service.


Where to find other tests
-------------------------

The tests in this directory do not use any of the tools provided by
`libgeopm` or `libgeopmpolicy`.  Integration tests for `libgeopm` and
`libgeopmpolicy` derived features are located in
`geopm/integration/test`.  Some of these tests may use the GEOPM
service on a system where it is required.

The unit tests for the C++ files in `geopm/service/src` are located in
`geopm/test` along with the unit tests for the files in `geopm/src`.
In the future it may make sense to split the unit tests in to two
directories so that the service subdirectory is fully independent.

The unit tests for the geopmdpy module are located in
`geopm/service/geopmdpy_test`.
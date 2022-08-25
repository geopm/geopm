Running the GEOPM Service tests
===============================

There are several different integration tests that are available for testing the service.
This readme file provides instructions on how to run the integration tests.

**Old Tests:**

 - test_sst_priority.sh
 - test_su_give_access.sh
 - test_su_restart.sh
 - test_su_term_batch_helper.sh
 - test_su_term_batch.sh

**New Tests:**

 - test_kill_batch_client.sh
 - test_kill_batch_server.sh
 - test_kill_geopmd_batch_run.sh
 - test_kill_geopmd_serial_run.sh
 - test_systemctl_stop_geopm.sh

How to Run the **New Tests**
------------------------

We need to first build the service directory and it's associated rpm package.

```bash
cd geopm/service
./autogen.sh
./configure --prefix=$HOME/build/geopm-service --disable-fortran
make -j20
make checkprogs -j20
make rpm
make install
```

Next we need to export the environment variables.

```bash
export PATH=$GEOPM_INSTALL/bin:$PATH
export LD_LIBRARY_PATH=$GEOPM_INSTALL/lib:$LD_LIBRARY_PATH
```

Then we need to install the service.

```bash
cd geopm/service
sudo /usr/sbin/install_service.sh `cat VERSION` $USER
```

The `VERSION` file is located in the `geopm/service` directory.

Then we have to set up the allow list using `geopmaccess`,
for both the signals and the controls.

```bash
geopmaccess -a | sudo geopmaccess -w
geopmaccess -ac | sudo geopmaccess -wc
```

After that we can run the tests directly on the command line.

```bash
cd geopm/service/integration/test
./test_kill_batch_client.sh
./test_kill_batch_server.sh
./test_kill_geopmd_batch_run.sh
./test_kill_geopmd_serial_run.sh
./test_systemctl_stop_geopm.sh
```

If the test finishes with an exit code of `0`, and it prints `SUCCESS` at the end,
then the test has succeeded.

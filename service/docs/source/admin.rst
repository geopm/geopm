
Guide for Service Administrators
================================

The ``geopmaccess`` command line tool is used by a system
administrator to manage access to the features provided by the GEOPM
Service.  The GEOPM Service does not allow read or write access for
any non-root user until the system administrator explicitly configures
the service using the ``geopmaccess`` command line tool.  This command
line interface allows the administrator to set access permissions for
all users, and may extend these default privileges for specific Unix
groups.


Example: Enabling User Access
-----------------------------

This test configures the GEOPM service to enable and disable any
user to read and write to bits 8-15 of the MSR_PERF_CTL register
which controls the maximum frequency of the core.  The test saves
the existing settings so they can be restored at the end of the
test.  Access to the control is removed from the list, and it is
shown that the test user cannot succesfully run the
test_write_session.sh script.  Access to the control is granted
and it is shown that the test user can succefully change the
value.


.. code-block:: bash

    # PARAMETERS
    CONTROL=MSR::PERF_CTL:FREQ
    TEST_DIR=$(dirname $(readlink -f $0))
    TEST_USER=test
    TEST_SCRIPT=${TEST_DIR}/test_write_session.sh
    SAVE_SIGNALS=$(mktemp)
    SAVE_CONTROLS=$(mktemp)
    TEST_SIGNALS=$(mktemp)
    TEST_CONTROLS=$(mktemp)

    test_error() {
        echo "Error: $1" 1>&2
        exit -1
    }

    # MAKE SURE TEST IS RUN AS ROOT USER
    test ${USER} == "root" ||
        test_error "Must be run as the root user, this is an administrator test"

    # SAVE INITIAL ACCESS SETTINGS
    geopmaccess > ${SAVE_SIGNALS}
    geopmaccess -c > ${SAVE_CONTROLS}

    # REMOVE CONTROL FROM ACCESS LIST FOR READING AND WRITING
    grep -v ${CONTROL} ${SAVE_SIGNALS} > ${TEST_SIGNALS}
    grep -v ${CONTROL} ${SAVE_CONTROLS} > ${TEST_CONTROLS}
    geopmaccess -w < ${TEST_SIGNALS}
    geopmaccess -w -c < ${TEST_CONTROLS}

    # RUN WRITE SESSION TEST AND MAKE SURE IT FAILS
    su ${TEST_USER} ${TEST_SCRIPT} &&
        test_error "Access to $CONTROL was disabled, but write session passed"

    # ADD THE CONTROL INTO ACCESS LIST FOR READING AND WRITING
    echo ${CONTROL} >> ${TEST_SIGNALS}
    echo ${CONTROL} >> ${TEST_CONTROLS}
    geopmaccess -w < ${TEST_SIGNALS}
    geopmaccess -w -c < ${TEST_CONTROLS}

    # RUN WRITE SESSION TEST AND MAKE SURE IT PASSES
    su ${TEST_USER} ${TEST_SCRIPT} ||
        test_error "Access to $CONTROL was enabled, but write session failed"

    # RESTORE INITIAL ACCESS SETTINGS
    geopmaccess -w < ${SAVE_SIGNALS}
    geopmaccess -w -c < ${SAVE_CONTROLS}

    # CLEAN UP TEMPORARY FILES
    rm ${SAVE_SIGNALS} ${SAVE_CONTROLS} ${TEST_SIGNALS} ${TEST_CONTROLS}

    echo "SUCCESS"
    exit 0

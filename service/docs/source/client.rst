
Guide for Service Clients
=========================

The ``geopmsession`` command line tool is one way to interface with
the GEOPM DBus APIs.


Example: Creating a write session
---------------------------------

This test shows the GEOPM service being used to set the maximum
CPU frequency of core zero to 2 GHz.  The test uses the
geopmsession command line tool to read and write from/to the
MSR_PERF_CTL register to control the maximum frequency of the
core.  The test first reads the value of the register, then opens
a write session to set it to 2 GHz and reads the register again.
The test then kills the write session with signal 9 and reads the
control register a third time.  The test asserts that the control
value was changed by the write sesssion, and that this change was
reverted to the value it started with after the session is
killed.


.. code-block:: bash

    # PARAMETERS
    CONTROL=MSR::PERF_CTL:FREQ
    DOMAIN=core
    DOMAIN_IDX=0
    SETTING=2000000000.0
    REQUEST="${CONTROL} ${DOMAIN} ${DOMAIN_IDX}"

    test_error() {
        echo "Error: $1" 1>&2
        exit -1
    }

    # READ START VALUE OF CONTROL REGISTER
    START_VALUE=$(echo ${REQUEST} | geopmsession) ||
        test_error "Call to read ${CONTROL} through geopmsession failed"

    # CHECK THAT IT IS DIFFERENT THAN THE TEST VALUE
    test ${SETTING} != ${START_VALUE} ||
        test_error "Start value for the control is the same as the test value"

    # START A SESSION THAT WRITES THE CONTROL VALUE
    echo "${REQUEST} ${SETTING}" | geopmsession -w -t 10 &
    SESSION_ID=$!
    sleep 1

    # READ THE CONTROLLED REGISTER
    SESSION_VALUE=$(echo ${REQUEST} | geopmsession)

    # END THE SESSION
    kill -9 ${SESSION_ID} ||
        test_error "Failed to kill session"
    sleep 1

    # READ THE RESTORED REGISTER
    END_VALUE=$(echo ${REQUEST} | geopmsession)

    # CHECK THAT THE REGISTER WAS CHANGED DURING THE SESSION
    test ${SETTING} == ${SESSION_VALUE} ||
        test_error "Control is not set during the session"

    # CHECK THAT SAVE/RESTORE WORKED
    test ${START_VALUE} == ${END_VALUE} ||
        test_error "Control is not restored after the session"

    echo "SUCCESS"
    exit 0

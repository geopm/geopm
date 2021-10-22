#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""The pio module provides python bindings for the geopm_pio_c(3) C
interfaces.  This interface provides an abstraction for reading
signals and writing controls from system components.

"""

from __future__ import absolute_import

import cffi
import sys
from . import topo
from . import error


_ffi = cffi.FFI()
_ffi.cdef("""

struct geopm_request_s {
    int domain;
    int domain_idx;
    char name[255];
};

int geopm_pio_num_signal_name(void);

int geopm_pio_signal_name(int name_idx,
                          size_t result_max,
                          char *result);

int geopm_pio_num_control_name(void);

int geopm_pio_control_name(int name_index,
                           size_t result_max,
                           char *result);

int geopm_pio_signal_domain_type(const char *signal_name);

int geopm_pio_control_domain_type(const char *control_name);

int geopm_pio_read_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx,
                          double *result);

int geopm_pio_write_control(const char *control_name,
                            int domain_type,
                            int domain_idx,
                            double setting);

int geopm_pio_push_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx);

int geopm_pio_push_control(const char *control_name,
                           int domain_type,
                           int domain_idx);

int geopm_pio_sample(int signal_idx,
                     double *result);

int geopm_pio_adjust(int control_idx,
                     double setting);

int geopm_pio_read_batch(void);

int geopm_pio_write_batch(void);

int geopm_pio_save_control(void);

int geopm_pio_save_control_dir(const char *save_dir);

int geopm_pio_restore_control(void);

int geopm_pio_restore_control_dir(const char *save_dir);

int geopm_pio_signal_description(const char *signal_name,
                                 size_t description_max,
                                 char *description);

int geopm_pio_control_description(const char *control_name,
                                  size_t description_max,
                                  char *description);

int geopm_pio_signal_info(const char *signal_name,
                          int *aggregation_type,
                          int *format_type,
                          int *behavior_type);

int geopm_pio_start_batch_server(int client_pid,
                                 int num_signal,
                                 const struct geopm_request_s *signal_config,
                                 int num_control,
                                 const struct geopm_request_s *control_config,
                                 int *server_pid,
                                 int key_size,
                                 char *server_key);

int geopm_pio_stop_batch_server(int server_pid);

int geopm_pio_format_signal(double signal,
                            int format_type,
                            size_t result_max,
                            char *result);


""")
_dl = _ffi.dlopen('libgeopmd.so', _ffi.RTLD_GLOBAL|_ffi.RTLD_LAZY)

def signal_names():
    """Get all available signals.

    A user can choose from any of the signal names returned to
    pass as a parameter to other pio module functions that take a
    signal_name as input.

    Returns:
        list(str): All available signal names.

    """
    global _ffi
    global _dl

    result = []
    num_signal = _dl.geopm_pio_num_signal_name()
    if num_signal < 0:
        raise RuntimeError('geopm_pio_num_signal_name() failed: {}'.format(error.message(num_signal)))
    name_max = 255
    signal_name_cstr = _ffi.new("char[]", name_max)
    for signal_idx in range(num_signal):
        err = _dl.geopm_pio_signal_name(signal_idx, name_max, signal_name_cstr)
        if err < 0:
            raise RuntimeError('geopm_pio_signal_name() failed: {}'.format(error.message(err)))
        result.append(_ffi.string(signal_name_cstr).decode())
    return result

def control_names():
    """Get all available controls.

    A user can choose from any of the control names returned to
    pass as a parameter to other pio module functions that take a
    control_name as input.

    Returns:
        list(str): All available control names.

    """
    global _ffi
    global _dl
    result = []
    num_control = _dl.geopm_pio_num_control_name()
    if num_control < 0:
        raise RuntimeError('geopm_pio_num_control_name() failed: {}'.format(error.message(num_control)))
    name_max = 1024
    control_name_cstr = _ffi.new("char[]", name_max)
    for control_idx in range(num_control):
        err = _dl.geopm_pio_control_name(control_idx, name_max, control_name_cstr)
        if err < 0:
            raise RuntimeError('geopm_pio_control_name() failed: {}'.format(error.message(err)))
        result.append(_ffi.string(control_name_cstr).decode())
    return result

def signal_domain_type(signal_name):
    """Get the domain type that is native for a signal.

    Signals cannot be sampled at a finer granularity than the native
    domain, and such requests will result in a raised exception.  When
    a request is made to read a signal at a courser domain than the
    native domain, the signal will be read at the native granularity
    for all sub-domains of the requested domain and the returned result
    will be aggregated from the values read.  The result can be
    converted into a domain name with the topo.domain_name() function.

    Args:
        signal_name (str): name of signal to query

    Returns:
        int: One of the geopmdpy.topo.DOMAIN_* integers corresponding
            to a domain type.

    """
    global _ffi
    global _dl
    signal_name_cstr = _ffi.new("char[]", signal_name.encode())
    result = _dl.geopm_pio_signal_domain_type(signal_name_cstr)
    if result < 0:
        raise RuntimeError('geopm_pio_signal_domain_type() failed: {}'.format(error.message(result)))
    return result

def control_domain_type(control_name):
    """Get the domain type that is native for a control.

    Controls cannot be set at a finer granularity than the native
    domain, and such requests will result in a raised exception.  When
    a request is made to write a control at a courser domain than the
    native domain, the same control value will be set for all
    sub-domains of the requested domain.

    Args:
        control_name (str): name of control to query

    Returns:
        int: One of the geopmdpy.topo.DOMAIN_* integers corresponding
            to a domain type.  This can be converted into a domain
            name with the geopmdpy.topo.domain_name() function.

    """
    global _ffi
    global _dl
    control_name_cstr = _ffi.new("char[]", control_name.encode())
    result = _dl.geopm_pio_control_domain_type(control_name_cstr)
    if result < 0:
        raise RuntimeError('geopm_pio_control_domain_type() failed: {}'.format(error.message(result)))
    return result

def read_signal(signal_name, domain_type, domain_idx):
    """Read a signal value from platform.

    Select a signal by name and read the current value from the
    domain type and index specified.  The read signal is returned
    as a floating point number in SI units.  The signal name can
    be any returned by the signal_names() function.

    Args:
        signal_name (str): Name of the signal to be read.
        domain_type (int or str): One of the domain types provided
            by the geopmdpy.topo module or the name associated
            with a domain type (e.g. 'cpu' or 'board').
        domain_idx (int): Index of domain to read from.

    Returns:
        float: The value of the signal read in SI units.

    """
    global _ffi
    global _dl
    result_cdbl = _ffi.new("double*")
    signal_name_cstr = _ffi.new("char[]", signal_name.encode())
    domain_type = topo.domain_type(domain_type)
    err = _dl.geopm_pio_read_signal(signal_name_cstr, domain_type, domain_idx, result_cdbl)
    if err < 0:
        raise RuntimeError('geopm_pio_read_signal() failed: {}'.format(error.message(err)))
    return result_cdbl[0]

def write_control(control_name, domain_type, domain_idx, setting):
    """Write a control value to the platform.

    Select a control by name and write a new setting for the
    domain type and index specified.  The written control is a
    floating point number in SI units.  The control name can be any
    returned by the control_names() function.

    Args:
        control_name (str): Name of the control to be written.
        domain_type (int or str): One of the domain types provided
            by the geopmdpy.topo module or the name associated
            with a domain type (e.g. 'cpu' or 'board').
        domain_idx (int): Index of domain to write to.
        setting (float): Value of the control to be written.

    """
    global _ffi
    global _dl
    control_name_cstr = _ffi.new("char[]", control_name.encode())
    domain_type = topo.domain_type(domain_type)
    err = _dl.geopm_pio_write_control(control_name_cstr, domain_type, domain_idx, setting)
    if err < 0:
        raise RuntimeError('geopm_pio_write_control() failed: {}'.format(error.message(err)))

def push_signal(signal_name, domain_type, domain_idx):
    """Push a signal onto the stack of batch access signals.

    Subsequent calls to the read_batch() function will read the signal
    and update the internal state used to store batch signals.  The
    function returns an index that can be passed to the sample()
    function to access the signal value stored in the internal state
    from the last update.  A distinct signal index will be returned
    for each unique combination of input parameters.  All signals must
    be pushed onto the stack prior to the fist call to sample() or
    read_batch().  Attempts to push a signal onto the stack after the
    first call to sample() or read_batch() or attempts to push a
    signal_name that is not provided by signal_names() will result in
    a raised exception.

    Args:
        signal_name (str): Name of the signal to be read.
        domain_type (int or str): One of the domain types provided
            by the geopmdpy.topo module or the name associated
            with a domain type (e.g. 'cpu' or 'board').
        domain_idx (int): Index of domain to read from.

    Returns:
        int: Signal index that can be passed to the sample() function
            after the read_batch() function has been called.

    """
    global _ffi
    global _dl
    signal_name_cstr = _ffi.new("char[]", signal_name.encode())
    domain_type = topo.domain_type(domain_type)
    result = _dl.geopm_pio_push_signal(signal_name_cstr, domain_type, domain_idx)
    if result < 0:
        raise RuntimeError('geopm_pio_push_signal() failed: {}'.format(error.message(result)))
    return result

def push_control(control_name, domain_type, domain_idx):
    """Push a control onto the stack of batch access controls.

    The return value is an index that can be passed to the adjust()
    function which will update the internal state used to store batch
    controls.  Subsequent calls to the write_batch() function access
    the control values in the internal state and write the values to
    the hardware.  A distinct control index will be returned for each
    unique combination of input parameters.  All controls must be
    pushed onto the stack prior to the first call to the adjust() or
    write_batch() functions.  Attempts to push a controls onto the
    stack after the first call to adjust() or write_batch() or
    attempts to push a control_name that is not a value provided by
    the control_names() function will result in a raised exception.

    Args:
        control_name (str): Name of the control to be written.
        domain_type (int or str): One of the domain types provided
            by the geopmdpy.topo module or the name associated
            with a domain type (e.g. 'cpu' or 'board').
        domain_idx (int): Index of domain to write to.

    Returns:
        int: Control index that can be passed to the adjust()
        function prior to a call to the write_batch() function.

    """
    global _ffi
    global _dl
    control_name_cstr = _ffi.new("char[]", control_name.encode())
    domain_type = topo.domain_type(domain_type)
    result = _dl.geopm_pio_push_control(control_name_cstr, domain_type, domain_idx)
    if result < 0:
        raise RuntimeError('geopm_pio_push_control() failed: {}'.format(error.message(result)))
    return result

def sample(signal_idx):
    """Samples cached value of a single signal.

    This function returns one of the values that was read in the last
    call to read_batch().  The particular signal returned is
    determined by the signal index provided.  This signal index is the
    value that was returned when push_signal() was called.

    Args:
        signal_idx (int): Index returned by a previous call to the
            push_signal() function.

    Returns:
        float: Value of signal read when read_batch() function was
            last called.

    """
    global _ffi
    global _dl
    result_cdbl = _ffi.new("double*")
    err = _dl.geopm_pio_sample(signal_idx, result_cdbl)
    if err < 0:
        raise RuntimeError('geopm_pio_sample() failed: {}'.format(error.message(err)))
    return result_cdbl[0]

def adjust(control_idx, setting):
    """Updates the cached value of a single control.

    The particular control that is modified is determined by the
    control index provided.  The control index is the value that was
    returned when push_control() was called.  The value for this
    control is updated so that the next call to write_batch() will
    write the setting to the platform.

    Args:
        control_idx (int): Index returned by a previous call to the
                           push_control() function.

        setting (float): Value for control to be set on next call to
                         write_batch().

    """
    global _dl
    err = _dl.geopm_pio_adjust(control_idx, setting)
    if err < 0:
        raise RuntimeError('geopm_pio_adjust() failed: {}'.format(error.message(err)))

def read_batch():
    """Read all pushed signals from the platform.

    The next calls to sample() will reflect the updated data.

    """
    global _dl
    err = _dl.geopm_pio_read_batch()
    if err < 0:
        raise RuntimeError('geopm_pio_read_batch() failed: {}'.format(error.message(err)))

def write_batch():
    """Write all pushed controls to the platform.

    The values provided to previous calls to adjust() are written to
    the platform.

    """
    global _dl
    err = _dl.geopm_pio_write_batch()
    if err < 0:
        raise RuntimeError('geopm_pio_write_batch() failed: {}'.format(error.message(err)))

def save_control():
    """Save the state of all controls.

    Any subsequent changes made through write_control() or
    write_batch() will be reverted with a call to restore_control().
    The control settings are stored in memory managed by GEOPM.

    Raises:
        RuntimeError:  Failure to save all control values.

    """
    global _dl
    err = _dl.geopm_pio_save_control()
    if err < 0:
        raise RuntimeError('geopm_pio_save_control() failed: {}'.format(error.message(err)))

def restore_control():
    """Restore the state recorded by the last call to save_control().

    All previous changes made through write_control() or write_batch()
    are reverted to their previous settings.

    Raises:
        RuntimeError:  Failure to restore all control values.

    """

    global _dl
    err = _dl.geopm_pio_restore_control()
    if err < 0:
        raise RuntimeError('geopm_pio_restore_control() failed: {}'.format(error.message(err)))

def signal_description(signal_name):
    """Get a description of a signal.

    A description should include the units of the signal and the
    aggregation function along with other descriptive text.  An
    exception is raised if any error occurs.

    Args:
        signal_name (str): Name of signal.

    Returns:
        str: Signal description string.

    """
    global _ffi
    global _dl
    name_max = 1024
    signal_name_cstr = _ffi.new("char[]", signal_name.encode())
    result_cstr = _ffi.new("char[]", name_max)
    err = _dl.geopm_pio_signal_description(signal_name_cstr, name_max, result_cstr)
    if err < 0:
        raise RuntimeError('geopm_pio_signal_description() failed: {}'.format(error.message(err)))
    return _ffi.string(result_cstr).decode()

def control_description(control_name):
    """Get a description of a control.

    A description should include the units of the control along with
    other descriptive text.  An exception is raised if any error
    occurs.

    Args:
        control_name (str): Name of control.

    Returns:
        str: Control description string.

    """
    global _ffi
    global _dl
    name_max = 1024
    control_name_cstr = _ffi.new("char[]", control_name.encode())
    result_cstr = _ffi.new("char[]", name_max)
    err = _dl.geopm_pio_control_description(control_name_cstr, name_max, result_cstr)
    if err < 0:
        raise RuntimeError('geopm_pio_control_description() failed: {}'.format(error.message(err)))
    return _ffi.string(result_cstr).decode()

def save_control_dir(save_dir):
    """Save the state of all controls to files in the save directory.

    Any subsequent changes made through write_control() or
    write_batch() will be reverted with a call to
    restore_control_dir(save_dir).  The control settings are stored in
    files named after each IOGroup that is loaded.

    Args:
        save_dir (str): Output directory where each IOGroup creates a
                        save file.

    Raises:
        RuntimeError:  Failure to save all control values.

    """
    global _ffi
    global _dl
    name_max = 1024
    save_dir_cstr = _ffi.new("char[]", save_dir.encode())
    err = _dl.geopm_pio_save_control_dir(save_dir_cstr)
    if err < 0:
        raise RuntimeError('geopm_pio_save_control_dir() failed: {}'.format(error.message(err)))

def restore_control_dir(save_dir):
    """Restore the state recorded to the save directory

    The save directory will have been created by the last call to
    save_control_dir(save_dir) so that all subsequent changes made
    through write_control() or write_batch() are reverted to their
    previous settings.

    Args:
        save_dir (str): Output directory where each IOGroup has
                        created a save file.

    Raises:
        RuntimeError:  Failure to restore all control values.

    """
    global _ffi
    global _dl
    name_max = 1024
    save_dir_cstr = _ffi.new("char[]", save_dir.encode())
    err = _dl.geopm_pio_restore_control_dir(save_dir_cstr)
    if err < 0:
        raise RuntimeError('geopm_pio_restore_control() failed: {}'.format(error.message(err)))

def signal_info(signal_name):
    """Get information about a signal

    Query the integer enum values that give information about the
    named signal.  There are three integers returned which correspond
    to the the aggregation type, the format method, and the behavior
    of the signal.

    The aggregation type is one of the the geopm::Agg::m_type_e enum
    values defined in Agg.hpp.  The format method is one of the
    geopm::string_format_e enum values defined in Helper.hpp.  The
    behavior is one of the geopm::IOGroup::m_signal_behavior_e enum
    values defined in IOGroup.hpp.

    Args:
        signal_name (str): Name of the signal to query.

    Returns:
        tuple(int, int, int): The aggregation type, format type, and
                              behavior type enum values associated
                              with the signal.

    Raises:
        RuntimeError: Query of signal name failed.

    """
    global _ffi
    global _dl
    name_max = 1024
    signal_name_cstr = _ffi.new("char[]", signal_name.encode())
    aggregation_type = _ffi.new("int*")
    format_type = _ffi.new("int*")
    behavior_type = _ffi.new("int*")
    err = _dl.geopm_pio_signal_info(signal_name_cstr, aggregation_type, format_type, behavior_type)
    if err < 0:
        raise RuntimeError('geopm_pio_signal_info() failed: {}'.format(error.message(err)))
    return (aggregation_type[0], format_type[0], behavior_type[0])

def start_batch_server(client_pid, signal_config, control_config):
    """Start a batch server to interface with a client thread

    Create a new process to interact with the client thread using the
    GEOPM batch server protocol.  The created server will be enabled
    to read the configured signals and write the configured controls
    based on requests made by the client thread after the server is
    running.  Each configuration is a list of tuples that specify the
    name of the signal or control, the domain type, and domain index.
    This call to start the batch server will block until the server is
    up and ready to respond to client requests through the GEOPM batch
    server protocol.

    Args:
        client_pid (int): Linux PID of the thread that will interact
                          with the batch server.

        signal_config (list((str, int, int))): List of requested
            signals where each tuple represents (signal_name,
            domain_type, domain_idx).

        control_config (list((str, int, int))): List of requested
            controlss where each tuple represents (control_name,
            domain_type, domain_idx).

    Returns:
        tuple(int, str): The server PID and the string key used by the
                         client thread to initiate the GEOPM batch
                         server protocol.

    Raises:
        NotImplementedError: This function has not yet been
                             implemented.

    """

    sys.stderr.write(f"pio.start_batch_server({client_pid}, {signal_config}, {control_config}): begin\n")
    num_signal = len(signal_config)
    num_control = len(control_config)
    if num_signal != 0:
        signal_config_carr = _ffi.new(f'struct geopm_request_s[{num_signal}]')
    else:
        signal_config_carr = _ffi.NULL
    if num_control != 0:
        control_config_carr = _ffi.new(f'struct geopm_request_s[{num_control}]')
    else:
        control_config_carr = _ffi.NULL

    for idx, req in enumerate(signal_config):
        signal_config_carr[idx].domain = req[0]
        signal_config_carr[idx].domain_idx = req[1]
        signal_config_carr[idx].name = req[2].encode()

    for idx, req in enumerate(control_config):
        control_config_carr[idx].domain = req[0]
        control_config_carr[idx].domain_idx = req[1]
        control_config_carr[idx].name = req[2].encode()

    server_pid_c = _ffi.new('int *')
    server_key_cstr = _ffi.new('char [255]')
    sys.stderr.write("pio.start_batch_server(): call enter\n")
    err = _dl.geopm_pio_start_batch_server(client_pid,
                                           num_signal,
                                           signal_config_carr,
                                           num_control,
                                           control_config_carr,
                                           server_pid_c,
                                           255,
                                           server_key_cstr)
    sys.stderr.write("pio.start_batch_server(): call exit\n")
    if err < 0:
        raise RuntimeError('geopm_pio_start_batch_server() failed: {}'.format(error.message(err)))
    server_pid = server_pid_c[0]
    server_key = _ffi.string(server_key_cstr).decode()
    sys.stderr.write(f"pio.start_batch_server(): return {server_pid} {server_key}\n")
    return server_pid, server_key

def stop_batch_server(server_pid):
    """Stop a currently running batch server

    End a batch server process that was created with a call to
    start_batch_server().  This will end the server process, and the
    client will not be able to use the GEOPM batch server protocol
    unless it starts a new server.

    Args:
        server_pid (int): Linux PID of the created batch server.  This
                          is the value of the first element of the
                          tuple returned by the start_batch_server()
                          function.

    Raises:
        NotImplementedError: This function has not yet been
                             implemented.

    """
    _dl.geopm_pio_stop_batch_server(server_pid)

def format_signal(signal, format_type):
    """Convert a signal into a string representation

    All signals have a format enum that describes how to format a
    value that was read.  This format enum can be queried with the
    signal_info() function and passed as the format type parameter.

    Args:
        signal (float): The value of a signal that was read.

        format_type (int): The geopm::string_format_e enum value
                           defined in Helper.hpp that describes how to
                           format the value.

    Returns:
        str: String representation of the signal value.

    Raises:
        RuntimeError: Unable to format the value.

    """
    global _ffi
    global _dl

    result = ''
    name_max = 1024
    result_cstr = _ffi.new("char[]", name_max)
    err = _dl.geopm_pio_format_signal(signal, format_type, name_max, result_cstr)
    if err < 0:
        raise RuntimeError('geopm_pio_format_signal() failed: {}'.format(error.message(err)))
    return _ffi.string(result_cstr).decode()

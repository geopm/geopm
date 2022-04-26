#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Tools to support runtime implementations that use the GEOPM service

"""

import math
import time
import subprocess # nosec
import sys
import shlex
from . import pio


class TimedLoop:
    """Object that can be iterated over to run a timed loop

    Use in a for loop to execute a fixed number of timed delays.  The
    overhead time for executing what is inside of the loop is
    accounted for.  Calls to time.sleep() are made to delay until the
    targeted end time for each iteration.

    Example:

        >>> from time import time
        >>> time_0 = time()
        >>> for index in TimedLoop(0.1, 10):
        ...     print(f'{index}: {time() - time_0}')
        ...
        0: 0.0008680820465087891
        1: 0.10126090049743652
        2: 0.20174455642700195
        3: 0.30123186111450195
        4: 0.4010961055755615
        5: 0.5020360946655273
        6: 0.6011238098144531
        7: 0.7011349201202393
        8: 0.8020164966583252
        9: 0.9015650749206543
        10: 1.0021190643310547

    """

    def __init__(self, period, num_period=None):
        """Constructor for timed loop object

        The number of loops executed is one greater than the number of
        time intervals requested, and that the first iteration is not
        delayed.  The total amount of time spanned by the loop is the
        product of the two input parameters.

        To create an infinite loop, specify num_period is None.

        Args:
            period (float): Target interval for the loop execution in
                            units of seconds.

            num_period (int): Number of time periods spanned by the
                              loop.  The total loop time is num_periods * period,
                              but since there is no delay in the first loop, there
                              will be num_period + 1 loop iterations.

        """

        if period < 0.0:
            raise RuntimeError('Specified period is invalid.  Must be >= 0.')
        if num_period is not None:
            if num_period < 0:
                raise RuntimeError('Specified num_period is invalid.  Must be > 0.')
            if not isinstance(num_period, int):
                raise ValueError('num_period must be a whole number.')
        self._period = period
        self._num_loop = num_period
        # Add one to ensure:
        #     total_time == num_loop * period
        # because we do not delay the start iteration
        if self._num_loop is not None:
            self._num_loop += 1

    def __iter__(self):
        """Set up a timed loop

        Iteration method for timed loop.  This iterator can be used in
        a for statement to execute the loop periodically.

        """
        self._loop_idx = 0
        self._target_time = time.time()
        return self

    def __next__(self):
        """Sleep until next targeted time for loop and update counter

        """
        result = self._loop_idx
        if self._loop_idx == self._num_loop:
            raise StopIteration
        if self._loop_idx != 0:
            sleep_time = self._target_time - time.time()
            if sleep_time > 0:
                time.sleep(sleep_time)
        self._target_time += self._period
        self._loop_idx += 1
        return result


class Agent:
    """Base class that documents the interfaces required by an agent

    Agent objects are used to initialize a Controller object and
    define the control algorithm.

    """
    def __init__(self):
        raise NotImplementedError('Agent is an abstract base class')

    def get_signals(self):
        """Get list of read requests

        The returned signals will be sampled from the platform by the
        Controller and passed into Agent.update().

        Returns:
            list((str, int, int)): List of request tuples. Each
                                   request comprises a signal name,
                                   domain type, and domain index.

        """
        raise NotImplementedError('Agent is an abstract base class')

    def get_controls(self):
        """Get list of control requests

        The returned controls will be set in the platform by the Controller
        based on the return value from Agent.update().

        Returns:
            list((str, int, int)): List of request tuples. Each
                                   request comprises a control name,
                                   domain type, and domain index.

        """
        raise NotImplementedError('Agent is an abstract base class')

    def run_begin(self, policy):
        """Called by Controller at the start of each run

        The policy for the run is passed through to the agent from the
        Controller.run() input.  For some agents, the policy may always be
        None.

        Args:
            policy (object): The Agent specific policy provided to the
                             Controller.run() method.

        """
        raise NotImplementedError('Agent is an abstract base class')

    def policy_repr(self, policy):
        """Create a string representation of a policy suitable for printing

        """
        return policy.__repr__()

    def run_end(self):
        """Called by the Controller at the end of each run

        The result of calling the get_report() method after run_end() should
        reflect the same report until the next call to run_end().  This report
        will document the measurements made between the last calls to
        run_begin() and run_end().  Each call to run_end() will follow a
        previous call to run_begin().  The run_end() method will be called by
        the Controller even if the run resulted in an error that raises an
        exception.  In this way resources associated with a single run can be
        released when the run_end() method is called.

        """
        raise NotImplementedError('Agent is an abstract base class')

    def update(self, signals):
        """Called periodically by the Controller

        The signals that specified by get_signals() will be passed as inputs
        to the method by the Controller.  The update() method will be called
        periodically and the interval is set by the value returned by
        Agent.get_period().

        Args:
            signals (list(float)): Recently read signal values

        Returns:
            list(float): Control values for next control interval

        """
        raise NotImplementedError('Agent is an abstract base class')

    def get_period(self):
        """Get the target time interval for the control loop

        Returns:
            float: Time interval in seconds

        """
        raise NotImplementedError('Agent is an abstract base class')

    def get_report(self, profile):
        """Summary of all data collected by calls to update()

        The report covers the interval of time between the last two calls to
        Agent.begin_run() / Agent.end_run().  Until the next call to
        Agent.begin_run(), the same report will be returned by this method.
        The Controller.run() method will return this report upon completion of
        the run.

        Args:
            profile (str): Profile name to associate with the report

        Returns:
            str: Human readable report

        """
        raise NotImplementedError('Agent is an abstract base class')


class Controller:
    """Class that supports a runtime control algorithm

    """
    def __init__(self, agent, timeout=0):
        """Controller constructor

        Args:
            agent (Agent): Object that conforms to the Agent class
                           interface

            timeout (float): The agent algorithm will run for the full
                             duration of the application execution if timeout
                             is 0.  Setting the timeout to a non-zero value
                             will end the agent algorithm after the specified
                             period of time or when the application ends,
                             whichever occurs first.

        """
        if not isinstance(agent, Agent):
            raise ValueError('agent must be a subclass of Agent.')
        if timeout < 0:
            raise ValueError('timeout must be >= 0')
        self._agent = agent
        self._signals = agent.get_signals()
        self._controls = agent.get_controls()
        self._signals_idx = []
        self._controls_idx = []
        self._update_period = agent.get_period()
        self._num_update = None
        if timeout != 0:
            self._num_update = math.ceil(timeout / self._update_period)
        self._returncode = None

    def push_all(self):
        self._signals_idx = [pio.push_signal(*ss) for ss in self._signals]
        self._controls_idx = [pio.push_control(*cc) for cc in self._controls]

    def read_all_signals(self):
        """Sample for all signals pushed with pio

        Returns:
            list(float): Sampled values for each signal

        """
        return [pio.sample(signal_idx)
                for signal_idx in self._signals_idx]

    def returncode(self):
        """Get the return code of the application process

        Returns:
            int: Return code of app process
        """
        if self._returncode is None:
            raise RuntimeError('App process is still running')
        return self._returncode

    def run(self, argv, policy=None, profile=None):
        """Execute control loop defined by agent

        Interfaces with PlatformIO directly through the geopmdpy.pio module.

        Args:
            argv (list(str)): Arguments for application that is executed

            policy (object): Policy for Agent to use during the run

        Returns:
            str: Human readable report created by agent

        """
        sys.stderr.write('<geopmdpy> RUN BEGIN\n')
        try:
            pio.save_control()
            self.push_all()
            pid = subprocess.Popen(argv)
            self._agent.run_begin(policy)
            for loop_idx in TimedLoop(self._update_period, self._num_update):
                if pid.poll() is not None:
                    break
                pio.read_batch()
                signals = self.read_all_signals()
                new_settings = self._agent.update(signals)
                for control_idx, new_setting in zip(self._controls_idx, new_settings):
                    pio.adjust(control_idx, new_setting)
                pio.write_batch()
        except:
            raise
        finally:
            pio.restore_control()
            self._agent.run_end()
        self._returncode = pid.returncode
        sys.stderr.write(f'<geopmdpy> RUN END, return: {self._returncode}\n')
        if profile is None:
            profile = ' '.join([shlex.quote(arg) for arg in argv])
        return self._agent.get_report(profile)

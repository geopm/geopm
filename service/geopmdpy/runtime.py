#!/usr/bin/env python3

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

"""Tools to support runtime implementations that use the GEOPM service

"""

import math
import time
import subprocess
import sys
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

        To create an infinte loop, specify num_period is None.

        Args:
            period (float): Target interval for the loop execution in
                            units of seconds.

            num_period (int): Number of time periods spanned by the
                              loop.  The total loop time is num_periods * period,
                              but since there is no delay in the first loop, there
                              will be num_period + 1 loop iterations.

        """

        if period <= 0.0:
            raise RuntimeError('Specified period is invalid.  Must be > 0.')
        if num_period is not None:
            if num_period <= 0:
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

        Returns:
            list((str, int, int)): List of request tuples. Each
                                   request comprises a signal name,
                                   domain type, and domain index.

        """
        raise NotImplementedError('Agent is an abstract base class')

    def get_controls(self):
        """Get list of control requests

        Returns:
            list((str, int, int)): List of request tuples. Each
                                   request comprises a control name,
                                   domain type, and domain index.

        """
        raise NotImplementedError('Agent is an abstract base class')

    def update(self, signals):
        """Called periodically by the Controller

        Args:
            signals (list(float)): Recently read signal values

        Returns:
            list(float): Control values for next control interval

        """
        raise NotImplementedError('Agent is an abstract base class')

    def get_period(self):
        """Get the target time interal for the control loop

        Returns:
            float: Time interval in seconds

        """
        raise NotImplementedError('Agent is an abstract base class')

    def get_report(self):
        """Summary of all data collected by calls to update()

        Returns:
            str: Human readable report

        """
        raise NotImplementedError('Agent is an abstract base class')


class Controller:
    """Class that supports a runtime control agorithm

    """
    def __init__(self, agent, argv, timeout=0):
        """Controller constructor

        Args:
            agent (Agent): Object that conforms to the Agent class
                           interface

            argv (list(str)): Arguments for application that is executed

            timeout (float): Maximum runtime before controller ends, the agent
                             will run indefinitely (until killed by another
                             process) if timeout == 0.

        """
        if not isinstance(agent, Agent):
            raise ValueError('agent must be a subclass of Agent.')
        if timeout < 0:
            raise ValueError('timeout must be >= 0')
        self._agent = agent
        self._signals = agent.get_signals()
        self._controls = agent.get_controls()
        self._signals_idx = [pio.push_signal(*ss) for ss in self._signals]
        self._controls_idx = [pio.push_control(*cc) for cc in self._controls]
        self._update_period = agent.get_period()
        self._num_update = None
        if timeout != 0:
            self._num_update = math.ceil(timeout / self._update_period)
        self._argv = argv
        self._returncode = None

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

    def run(self):
        """Execute control loop defined by agent

        Interfaces with PlatformIO directly through the geopmdpy.pio module.

        Returns:
            str: Human readable report created by agent

        """
        pio.save_control()
        try:
            pid = subprocess.Popen(self._argv)
            for loop_idx in TimedLoop(self._update_period, self._num_update):
                if pid.poll() is not None:
                    sys.stderr.write('Ths app process has ended.  return code = {}'.format(pid.returncode))
                    break
                pio.read_batch()
                signals = self.read_all_signals()
                controls = self._agent.update(signals)
                for control_idx in self._controls_idx:
                    pio.adjust(control_idx, controls[control_idx])
                pio.write_batch()
        except:
            raise
        finally:
            pio.restore_control()
        self._returncode = pid.returncode
        return self._agent.get_report()

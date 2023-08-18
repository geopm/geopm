#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Tools to support runtime implementations that use the GEOPM service

"""

import math
import time
import subprocess # nosec
import sys
import os
import shlex
from . import pio_rt
from . import reporter
from geopmdpy import loop

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

    def run_begin(self, policy, profile):
        """Called by Controller at the start of each run

        The policy for the run is passed through to the agent from the
        Controller.run() input.  For some agents, the policy may always be
        None.

        Args:
            policy (object): The Agent specific policy provided to the
                             Controller.run() method.

            profile (str): Profile name to associate with the report

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

    def get_report(self):
        """Summary of all data collected by calls to update()

        The report covers the interval of time between the last two calls to
        Agent.begin_run() / Agent.end_run().  Until the next call to
        Agent.begin_run(), the same report will be returned by this method.
        The Controller.run() method will return this report upon completion of
        the run.

        Returns:
            str: Human readable report

        """
        raise NotImplementedError('Agent is an abstract base class')

class Environment:
    def __init__(self):
        self._env = {kk:vv for (kk, vv) in
                     os.environ.items()
                     if kk.startswith('GEOPM_')}
    def profile(self):
        return self._env.get('GEOPM_PROFILE', 'default')

    def get(self):
        return dict(self._env)

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
        self._env = Environment()

    def push_all(self):
        self._signals_idx = [pio_rt.push_signal(*ss) for ss in self._signals]
        self._controls_idx = [pio_rt.push_control(*cc) for cc in self._controls]

    def read_all_signals(self):
        """Sample for all signals pushed with pio_rt

        Returns:
            list(float): Sampled values for each signal

        """
        return [pio_rt.sample(signal_idx)
                for signal_idx in self._signals_idx]

    def returncode(self):
        """Get the return code of the application process

        Returns:
            int: Return code of app process
        """
        if self._returncode is None:
            raise RuntimeError('App process is still running')
        return self._returncode

    def run(self, argv, policy=None):
        """Execute control loop defined by agent

        Interfaces with PlatformIO directly through the geopmpy.pio_rt module.

        Args:
            argv (list(str)): Arguments for application that is executed

            policy (object): Policy for Agent to use during the run

        Returns:
            str: Human readable report created by agent

        """
        sys.stderr.write('<geopmpy> RUN BEGIN\n')
        profile = self._env.profile();
        try:
            pio_rt.save_control()
            self.push_all()
            reporter.init()
            pid = subprocess.Popen(argv)
            pio_rt.update()

            self._agent.run_begin(policy, profile)
            for loop_idx in loop.PIDTimedLoop(pid, self._update_period, self._num_update):
                pio_rt.update()
                pio_rt.read_batch()
                signals = self.read_all_signals()
                new_settings = self._agent.update(signals)
                reporter.update()
                if pid.poll() is not None:
                    break
                for control_idx, new_setting in zip(self._controls_idx, new_settings):
                    pio_rt.adjust(control_idx, new_setting)
                pio_rt.write_batch()
            self._agent.run_end()
        except:
            raise
        finally:
            pio_rt.restore_control()
        self._returncode = pid.returncode
        sys.stderr.write(f'<geopmpy> RUN END, return: {self._returncode}\n')
        return self._agent.get_report()

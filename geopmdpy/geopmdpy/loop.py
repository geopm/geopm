#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Classes to support timer based loops

"""

import time
from subprocess import TimeoutExpired # nosec

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
                              loop.  The total loop time is
                              num_periods * period, but since there is
                              no delay in the first loop, there will
                              be num_period + 1 loop iterations.

        """

        if period < 0.0:
            raise RuntimeError('Specified period is invalid.  Must be >= 0.')
        if num_period is not None:
            if num_period < 0:
                raise RuntimeError('Specified num_period is invalid.  Must be > 0.')
            if not isinstance(num_period, int):
                raise ValueError('num_period must be a whole number.')
        self._period = period
        self._max_time = None
        if num_period is not None:
            self._max_time = time.time() + period * num_period
        self._stop_iteration = False
        self._one_shot = (num_period == 0)

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
        curr_time = time.time()
        if self._stop_iteration:
            raise StopIteration
        if self._loop_idx != 0:
            sleep_time = self._target_time - curr_time
            if sleep_time > 0:
                self.wait(sleep_time)
            if self._max_time is not None and time.time() >= self._max_time:
                self._stop_iteration = True

        self._target_time += self._period
        self._loop_idx += 1
        if self._one_shot:
            self._stop_iteration = True
        return result

    def wait(self, timeout):
        """Pass-through to time.sleep()

        Args:
            timeout (float): Target interval for the loop execution in
                             units of seconds.
        """
        time.sleep(timeout)


class PIDTimedLoop(TimedLoop):
    def __init__(self, pid, period, num_period=None):
        """Similar to the TimedLoop but stop when subprocess ends

        The number of loops executed is one greater than the number of
        time intervals requested, and that the first iteration is not
        delayed.  The total amount of time spanned by the loop is the
        product of the two input parameters.

        To create an infinite loop, specify num_period is None.

        Loop will always terminate when the subprocess pid terminates.

        Args:
            pid (Popen): Object returned by subprocess.Popen() constructor.

            period (float): Target interval for the loop execution in
                            units of seconds.

            num_period (int): Number of time periods spanned by the
                              loop.  The total loop time is
                              num_periods * period, but since there is
                              no delay in the first loop, there will
                              be num_period + 1 loop iterations.

        """

        super(PIDTimedLoop, self).__init__(period, num_period)
        self._pid = pid
        self._is_active = pid.poll() is None

    def wait(self, timeout):
        """Wait for timeout seconds or until pid ends

        Args:
            timeout (float): Target interval for the loop execution in
                             units of seconds.

        Raises:
            StopIteration: When last call to wait termintated due to
                           the process ending

        """
        if not self._is_active:
            raise StopIteration
        try:
            self._pid.wait(timeout=timeout)
            self._is_active = False
        except TimeoutExpired:
            pass


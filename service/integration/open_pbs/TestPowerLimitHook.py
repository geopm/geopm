#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
from unittest import mock
import json
import os
import copy

mock.patch.dict("sys.modules", pbs=mock.MagicMock()).start()
mock.patch("cffi.FFI.dlopen").start()
import geopm_power_limit as hook


SAVED_CONTROLS_FILE = "saved_controls.json"

CURRENT_SETTINGS = [
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0.0},
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_CLAMP_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0},
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 340000000},
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_LIMIT_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0}
    ]

CURRENT_SETTINGS_REF = {
        d["name"]: {k: v for (k, v) in d.items() if k != "name"}
        for d in CURRENT_SETTINGS
    }


def read_signal(name, domain_type, domain_idx):
    if name not in CURRENT_SETTINGS_REF:
        raise RuntimeError(f"Signal {name} not found!")
    if domain_type != CURRENT_SETTINGS_REF[name]["domain_type"]:
        raise RuntimeError(f"Invalid domain for signal {name}")
    if domain_idx != CURRENT_SETTINGS_REF[name]["domain_idx"]:
        raise RuntimeError(f"Invalid domain index for signal {name}")

    return CURRENT_SETTINGS_REF[name]["setting"]


def read_settings_from_file(file_name):
    with open(file_name) as f:
        settings_json = f.read()
    return json.loads(settings_json)


def write_settings_to_file(file_name, settings):
    with open(file_name, "w") as f:
        f.write(json.dumps(settings))


def setUpModule():
    hook._SAVED_CONTROLS_FILE = SAVED_CONTROLS_FILE

class MockAttr():
    def __init__(self, value):
        self._value = value

    def __str__(self):
        return str(self._value)

    def __repr__(self):
        # PBS hook documentation recommends using repr() in places you would
        # usually use str(). The default implementation of repr() for PBS
        # attributes behaves like str().
        return str(self._value)

@mock.patch("geopmdpy.pio.read_signal", side_effect=read_signal)
@mock.patch("geopmdpy.pio.write_control")
@mock.patch("geopmdpy.system_files.secure_make_dirs")
class TestPowerLimitPrologue(unittest.TestCase):
    JOB_ID = 1

    def setUp(self):
        self._mock_pbs = hook.pbs
        self._mock_event = mock.Mock()
        self._mock_server = mock.Mock()
        self._mock_server_job = mock.Mock()

        self._mock_pbs.event.return_value = self._mock_event
        self._mock_pbs.server.return_value = self._mock_server
        self._mock_server.job.return_value = self._mock_server_job

        self._mock_event.job.id = self.JOB_ID

    def tearDown(self):
        try:
            os.unlink(SAVED_CONTROLS_FILE)
        except:
            pass

    def test_power_limit_request(self, mock_secure_make_dirs,
                                 mock_write_control, mock_read_signal):
        REQUESTED_POWER_LIMIT = 240000000

        self._mock_server_job.Resource_List = {
            hook._POWER_LIMIT_RESOURCE: REQUESTED_POWER_LIMIT,
        }

        # Prepare expectations
        new_settings = copy.deepcopy(hook._controls)
        power_limit_setting = hook._power_limit_control.copy()
        power_limit_setting["setting"] = REQUESTED_POWER_LIMIT
        expected_calls = [
            mock.call(d["name"], d["domain_type"], d["domain_idx"],
                      d["setting"])
            for d in new_settings if d["name"] != power_limit_setting["name"]]
        expected_calls.append(mock.call(power_limit_setting["name"],
                                        power_limit_setting["domain_type"],
                                        power_limit_setting["domain_idx"],
                                        power_limit_setting["setting"]))

        hook.do_power_limit_prologue()

        # Assert prologue gets the right server job
        self._mock_server.job.assert_called_with(self.JOB_ID)
        # Assert saved controls path is created securely
        mock_secure_make_dirs.assert_called_with(hook._SAVED_CONTROLS_PATH)
        # Assert settings are saved to file
        saved_settings = read_settings_from_file(SAVED_CONTROLS_FILE)
        self.assertEqual(saved_settings, CURRENT_SETTINGS)
        # Assert new settings (including requested power limit) are written
        # correctly
        mock_write_control.assert_has_calls(expected_calls, any_order=True)
        # Assert hook accepts event
        self._mock_event.accept.assert_called_once()
        self._mock_event.reject.assert_not_called()

    def test_invalid_power_limit(self, mock_secure_make_dirs,
                                 mock_write_control, mock_read_signal):
        REQUESTED_POWER_LIMIT = "not a number"

        self._mock_server_job.Resource_List = {
                hook._POWER_LIMIT_RESOURCE: REQUESTED_POWER_LIMIT}
        self._mock_pbs.EXECJOB_PROLOGUE = 'execjob_prologue_event_type'
        self._mock_event.type = self._mock_pbs.EXECJOB_PROLOGUE
        self._mock_event.reject.side_effect = RuntimeError

        with self.assertRaises(RuntimeError):
            hook.do_power_limit_prologue()

        # Assert job gets removed from queue upon failure
        self._mock_event.job.delete.assert_called_once()
        # Assert hook rejects the event
        self._mock_event.reject.assert_called_once()
        # Assert hook doesn't attempt setting any controls
        mock_read_signal.assert_not_called()
        mock_write_control.assert_not_called()
        mock_secure_make_dirs.assert_not_called()

    def test_event_without_power_limit(self, mock_secure_make_dirs,
                                       mock_write_control, mock_read_signal):
        self._mock_server_job.Resource_List = {}

        hook.do_power_limit_prologue()

        # Assert hook accepts the event
        self._mock_event.accept.assert_called_once()
        # Assert hook doesn't attempt setting any controls
        mock_read_signal.assert_not_called()
        mock_write_control.assert_not_called()
        mock_secure_make_dirs.assert_not_called()

    def test_power_settings_restored(self, mock_secure_make_dirs,
                                     mock_write_control, mock_read_signal):
        self._mock_server_job.Resource_List = {}

        # Simulate epilogue not being able to run and settings from prior job
        # still in place.
        write_settings_to_file(SAVED_CONTROLS_FILE, CURRENT_SETTINGS)

        # pbs.event().accept() doesn't actually throw, but does end hook
        # execution. Using an exception here to simulate the same side effect.
        self._mock_event.accept.side_effect = RuntimeError
        with self.assertRaises(RuntimeError):
            hook.do_power_limit_prologue()

        # Assert hook accepts the event
        self._mock_event.accept.assert_called_once()
        # Hook shouldn't need to read or store any settings
        mock_read_signal.assert_not_called()
        mock_secure_make_dirs.assert_not_called()
        # Assert hook restores settings from file
        expected_calls = [
            mock.call(d["name"], d["domain_type"], d["domain_idx"],
                      d["setting"])
            for d in CURRENT_SETTINGS]
        mock_write_control.assert_has_calls(expected_calls, any_order=True)
        # Assert hook removes saved controls file
        self.assertFalse(os.path.exists(SAVED_CONTROLS_FILE))

    def test_uniform_power_limit(self, mock_secure_make_dirs,
                                 mock_write_control, mock_read_signal):
        JOB_POWER_LIMIT = 300
        MAX_NODE_POWER_LIMIT = 200
        self._mock_server_job.Resource_List = {
            hook._JOB_POWER_LIMIT_RESOURCE: JOB_POWER_LIMIT,
        }

        A1 = 1
        A2 = A1  # Same model for each node --> Same power limit for each node
        mock_file_contents = dict(
            max_power=MAX_NODE_POWER_LIMIT,
            profiles={'test-type': dict(hosts={
                'test-host-1': dict(model={
                    # Min slowdown at max power, increasing at a rate of 3x^2
                    # with x percent power below max.
                    # We requested 75% slowdown, which should be (3 * .5^2), 50% power in this case
                    'x0': 1.0,
                    'A': A1,
                    'B': 0,
                    'C': 0,
                }),
                'test-host-2': dict(model={
                    # Min slowdown at max power, increasing at a rate of 3x^2
                    # with x percent power below max.
                    # We requested 75% slowdown, which should be (3 * .5^2), 50% power in this case
                    'x0': 1.0,
                    'A': A2,
                    'B': 0,
                    'C': 0,
                }),
            })}
        )
        node1 = mock.Mock()
        node1.name = 'test-host-1'
        node2 = mock.Mock()
        node2.name = 'test-host-2'
        self._mock_event.vnode_list.values.return_value = [node1, node2]
        power_limit_setting = hook._power_limit_control.copy()

        self._mock_pbs.get_local_nodename.return_value = node1.name
        with mock.patch("builtins.open", mock.mock_open(read_data=json.dumps(mock_file_contents))):
            hook.do_power_limit_prologue()
        node_1_power_write = next(
            call[0][3] for call in mock_write_control.call_args_list
            if call[0][0] == power_limit_setting["name"] and
            call[0][1] == power_limit_setting["domain_type"] and
            call[0][2] == power_limit_setting["domain_idx"])
        mock_write_control.reset_mock()

        self._mock_pbs.get_local_nodename.return_value = node2.name
        with mock.patch("builtins.open", mock.mock_open(read_data=json.dumps(mock_file_contents))):
            hook.do_power_limit_prologue()
        node_2_power_write = next(
            call[0][3] for call in mock_write_control.call_args_list
            if call[0][0] == power_limit_setting["name"] and
            call[0][1] == power_limit_setting["domain_type"] and
            call[0][2] == power_limit_setting["domain_idx"])
        node_1_slowdown = A1 * (1 - node_1_power_write/MAX_NODE_POWER_LIMIT)**2
        node_2_slowdown = A2 * (1 - node_2_power_write/MAX_NODE_POWER_LIMIT)**2

        # Test that both nodes have the same power cap, and it is within bounds
        self.assertLessEqual(node_1_power_write, MAX_NODE_POWER_LIMIT)
        self.assertLessEqual(node_2_power_write, MAX_NODE_POWER_LIMIT)
        self.assertGreaterEqual(node_1_power_write, 0)
        self.assertGreaterEqual(node_2_power_write, 0)
        self.assertEqual(node_1_power_write, node_2_power_write)

        # Test that the cluster power budget is balanced
        self.assertEqual(JOB_POWER_LIMIT, node_1_power_write + node_2_power_write)

        # Test that both nodes have the same expected slowdown
        self.assertAlmostEqual(node_1_slowdown, node_2_slowdown)

    def test_non_uniform_power_limit(self, mock_secure_make_dirs,
                                     mock_write_control, mock_read_signal):
        JOB_POWER_LIMIT = 300
        MAX_NODE_POWER_LIMIT = 200
        self._mock_server_job.Resource_List = {
            hook._JOB_POWER_LIMIT_RESOURCE: JOB_POWER_LIMIT,
        }

        A1 = 1
        A2 = 1.1  # A1 != A2 --> Expect non-uniform power caps
        mock_file_contents = dict(
            max_power=MAX_NODE_POWER_LIMIT,
            node_profile_name='test-type',
            profiles={'test-type': dict(hosts={
                'test-host-1': dict(model={
                    # Min slowdown at max power, increasing at a rate of 3x^2
                    # with x percent power below max.
                    # We requested 75% slowdown, which should be (3 * .5^2), 50% power in this case
                    'x0': 1.0,
                    'A': A1,
                    'B': 0,
                    'C': 0,
                }),
                'test-host-2': dict(model={
                    # Min slowdown at max power, increasing at a rate of 3x^2
                    # with x percent power below max.
                    # We requested 75% slowdown, which should be (3 * .5^2), 50% power in this case
                    'x0': 1.0,
                    'A': A2,
                    'B': 0,
                    'C': 0,
                }),
            })}
        )
        node1 = mock.Mock()
        node1.name = 'test-host-1'
        node2 = mock.Mock()
        node2.name = 'test-host-2'
        self._mock_event.vnode_list.values.return_value = [node1, node2]
        power_limit_setting = hook._power_limit_control.copy()

        # Simulate each of node 1 and node 2 getting their prologues called
        self._mock_pbs.get_local_nodename.return_value = node1.name
        with mock.patch("builtins.open", mock.mock_open(read_data=json.dumps(mock_file_contents))):
            hook.do_power_limit_prologue()
        node_1_power_write = next(
            call[0][3] for call in mock_write_control.call_args_list
            if call[0][0] == power_limit_setting["name"] and
            call[0][1] == power_limit_setting["domain_type"] and
            call[0][2] == power_limit_setting["domain_idx"])
        mock_write_control.reset_mock()

        self._mock_pbs.get_local_nodename.return_value = node2.name
        with mock.patch("builtins.open", mock.mock_open(read_data=json.dumps(mock_file_contents))):
            hook.do_power_limit_prologue()
        node_2_power_write = next(
            call[0][3] for call in mock_write_control.call_args_list
            if call[0][0] == power_limit_setting["name"] and
            call[0][1] == power_limit_setting["domain_type"] and
            call[0][2] == power_limit_setting["domain_idx"])

        node_1_slowdown = A1 * (1 - node_1_power_write/MAX_NODE_POWER_LIMIT)**2
        node_2_slowdown = A2 * (1 - node_2_power_write/MAX_NODE_POWER_LIMIT)**2

        self.assertLessEqual(node_1_power_write, MAX_NODE_POWER_LIMIT)
        self.assertLessEqual(node_2_power_write, MAX_NODE_POWER_LIMIT)
        self.assertGreaterEqual(node_1_power_write, 0)
        self.assertGreaterEqual(node_2_power_write, 0)
        self.assertNotEqual(node_1_power_write, node_2_power_write)

        # Test that the cluster power budget is balanced
        self.assertEqual(JOB_POWER_LIMIT, node_1_power_write + node_2_power_write)

        # Test that both nodes have the same expected slowdown
        self.assertAlmostEqual(node_1_slowdown, node_2_slowdown)


@mock.patch("geopmdpy.pio.write_control")
class TestPowerLimitEpilogue(unittest.TestCase):
    def setUp(self):
        self._mock_pbs = hook.pbs
        self._mock_event = mock.Mock()
        self._mock_pbs.event.return_value = self._mock_event

    def tearDown(self):
        try:
            os.unlink(SAVED_CONTROLS_FILE)
        except:
            pass

    def test_no_settings_to_restore(self, mock_write_control):
        # Ensure there is no file prior to running the test
        self.assertFalse(os.path.exists(SAVED_CONTROLS_FILE))

        hook.do_power_limit_epilogue()

        # Assert hook accepts event
        self._mock_event.accept.assert_called_once()
        # Hook shouldn't need to write any settings
        mock_write_control.assert_not_called()

    def test_settings_restored(self, mock_write_control):
        # Create some settings to restore
        write_settings_to_file(SAVED_CONTROLS_FILE, CURRENT_SETTINGS)

        hook.do_power_limit_epilogue()

        # Assert hook accepts the event
        self._mock_event.accept.assert_called_once()
        # Assert hook restores settings from file
        expected_calls = [
            mock.call(d["name"], d["domain_type"], d["domain_idx"],
                      d["setting"])
            for d in CURRENT_SETTINGS]
        mock_write_control.assert_has_calls(expected_calls, any_order=True)
        # Assert hook removes saved controls file
        self.assertFalse(os.path.exists(SAVED_CONTROLS_FILE))

    def test_invalid_saved_controls_file(self, mock_write_control):
        # Create some spurious settings
        settings = [
            {"name": "SELF_DESTRUCT_CONTROL",
             "nuclear_option": True}]
        write_settings_to_file(SAVED_CONTROLS_FILE, settings)
        self._mock_event.reject.side_effect = RuntimeError

        with self.assertRaises(RuntimeError):
            hook.do_power_limit_epilogue()

        # Assert hook rejects the event
        self._mock_event.reject.assert_called_once()
        self._mock_event.accept.assert_not_called()
        # Hook shouldn't write any settings
        mock_write_control.assert_not_called()


@mock.patch("geopm_power_limit.do_power_limit_prologue")
@mock.patch("geopm_power_limit.do_power_limit_epilogue")
class TestPowerLimitMain(unittest.TestCase):
    def setUp(self):
        self._mock_pbs = hook.pbs
        self._mock_event = mock.Mock()
        self._mock_pbs.event.return_value = self._mock_event
        self._mock_pbs.EXECJOB_PROLOGUE = 1
        self._mock_pbs.EXECJOB_EPILOGUE = 2

    def test_prologue(self, mock_epilogue, mock_prologue):
        # Simulate the prologue event
        self._mock_event.type = self._mock_pbs.EXECJOB_PROLOGUE

        hook.hook_main()

        # Assert only the prologue is executed
        mock_epilogue.assert_not_called()
        mock_prologue.assert_called_once()
        # The hook main should not reject the event
        self._mock_event.reject.assert_not_called()

    def test_epilogue(self, mock_epilogue, mock_prologue):
        # Simulate the epilogue event
        self._mock_event.type = self._mock_pbs.EXECJOB_EPILOGUE

        hook.hook_main()

        # Assert only the epilogue is executed
        mock_epilogue.assert_called_once()
        mock_prologue.assert_not_called()
        # The hook main should not reject the event
        self._mock_event.reject.assert_not_called()

    def test_invalid_event(self, mock_epilogue, mock_prologue):
        SOME_OTHER_EVENT = self._mock_pbs.EXECJOB_PROLOGUE + \
                           self._mock_pbs.EXECJOB_EPILOGUE
        self._mock_event.type = SOME_OTHER_EVENT

        hook.hook_main()

        # Neither prologue or epilogue should be called and event should be
        # rejected
        mock_epilogue.assert_not_called()
        mock_prologue.assert_not_called()
        self._mock_event.reject.assert_called_once()
        self._mock_event.accept.assert_not_called()


class TestPowerLimitQueuejob(unittest.TestCase):
    JOB_ID = 1

    def setUp(self):
        self._mock_pbs = hook.pbs
        self._mock_server = mock.Mock()
        self._mock_event = mock.Mock()
        self._mock_event_job = mock.Mock()

        self._mock_pbs.server.return_value = self._mock_server
        self._mock_pbs.event.return_value = self._mock_event
        self._mock_event.job = self._mock_event_job

        self._mock_pbs.hook_config_filename = None

        self._mock_event_job.Resource_List = {
            hook._JOB_POWER_LIMIT_RESOURCE: None,
            hook._POWER_LIMIT_RESOURCE: None,
            hook._DEFAULT_SLOWDOWN_RESOURCE: None,
            hook._JOB_TYPE_RESOURCE: None,
        }

        self._MIN_NODE_POWER_LIMIT = 500
        self._MAX_NODE_POWER_LIMIT = 1000

        self._mock_event.job.id = self.JOB_ID

    def test_requested_node_power_limit(self):
        REQUESTED_NODE_POWER_LIMIT = 1000
        NODE_COUNT = 2

        self._mock_event_job.Resource_List[hook._POWER_LIMIT_RESOURCE] = REQUESTED_NODE_POWER_LIMIT
        self._mock_event_job.Resource_List['select'] = MockAttr(NODE_COUNT)

        self._mock_server.resources_available = {
            # Available job power can be anything except None for this case (if None,
            # then there's no need to auto-set job power)
            hook._JOB_POWER_LIMIT_RESOURCE: REQUESTED_NODE_POWER_LIMIT * NODE_COUNT,
            hook._MAX_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT,
            hook._MIN_POWER_LIMIT_RESOURCE: self._MIN_NODE_POWER_LIMIT,
        }

        hook.do_power_limit_queuejob()

        self.assertEqual(self._mock_event_job.Resource_List[hook._POWER_LIMIT_RESOURCE],
                         REQUESTED_NODE_POWER_LIMIT)

        # The assigned job limit should be consistent with the requested node limit
        self.assertEqual(self._mock_event_job.Resource_List[hook._JOB_POWER_LIMIT_RESOURCE],
                         NODE_COUNT * REQUESTED_NODE_POWER_LIMIT)

    def test_no_user_power_limit_with_high_admin_limit(self):
        NODE_COUNT = 2

        self._mock_event_job.Resource_List['select'] = MockAttr(NODE_COUNT)
        self._mock_server.resources_available = {
            hook._JOB_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT * NODE_COUNT,
            hook._MAX_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT,
            hook._MIN_POWER_LIMIT_RESOURCE: self._MIN_NODE_POWER_LIMIT,
        }

        hook.do_power_limit_queuejob()

        # Node power limit is only auto-set in the prologue. The queue hook only sets job power.
        self.assertIsNone(self._mock_event_job.Resource_List[hook._POWER_LIMIT_RESOURCE])
        self.assertEqual(self._mock_event_job.Resource_List[hook._JOB_POWER_LIMIT_RESOURCE],
                         self._MAX_NODE_POWER_LIMIT * NODE_COUNT)

    def test_no_user_power_limit_with_low_admin_limit(self):
        NODE_COUNT = 2

        self._mock_event_job.Resource_List['select'] = MockAttr(NODE_COUNT)
        self._mock_server.resources_available = {
            hook._JOB_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT * NODE_COUNT / 2,
            hook._MAX_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT,
            hook._MIN_POWER_LIMIT_RESOURCE: self._MIN_NODE_POWER_LIMIT,
        }

        hook.do_power_limit_queuejob()

        # Node power limit is only auto-set in the prologue. The queue hook only sets job power.
        self.assertIsNone(self._mock_event_job.Resource_List[hook._POWER_LIMIT_RESOURCE])
        # Don't require more power than the whole PBS server has available
        self.assertEqual(self._mock_event_job.Resource_List[hook._JOB_POWER_LIMIT_RESOURCE],
                         self._MAX_NODE_POWER_LIMIT * NODE_COUNT / 2)

    def test_no_user_power_limit_without_admin_limit(self):
        NODE_COUNT = 2

        self._mock_event_job.Resource_List['select'] = MockAttr(NODE_COUNT)
        self._mock_server.resources_available = {
            hook._JOB_POWER_LIMIT_RESOURCE: None,
            hook._MAX_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT,
            hook._MIN_POWER_LIMIT_RESOURCE: self._MIN_NODE_POWER_LIMIT,
        }

        hook.do_power_limit_queuejob()

        # Node limit should be untouched. Job limit should be set.
        self.assertEqual(self._mock_event_job.Resource_List[hook._POWER_LIMIT_RESOURCE], None)
        self.assertIsNone(self._mock_event_job.Resource_List[hook._JOB_POWER_LIMIT_RESOURCE])

    def test_user_slowdown(self):
        NODE_COUNT = 1

        self._mock_event_job.Resource_List['select'] = MockAttr(NODE_COUNT)
        self._mock_event_job.Resource_List[hook._JOB_TYPE_RESOURCE] = 'test-type'
        self._mock_event_job.Resource_List[hook._DEFAULT_SLOWDOWN_RESOURCE] = 0.75
        self._mock_server.resources_available = {
            hook._JOB_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT * NODE_COUNT,
            hook._MAX_POWER_LIMIT_RESOURCE: self._MAX_NODE_POWER_LIMIT,
            hook._MIN_POWER_LIMIT_RESOURCE: self._MIN_NODE_POWER_LIMIT,
        }
        self._mock_pbs.hook_config_filename = 'fake/file/path'
        mock_file_contents = dict(max_power=self._MAX_NODE_POWER_LIMIT, profiles={'test-type': dict(model={
            # Min slowdown at max power, increasing at a rate of 3x^2
            # with x percent power below max.
            # We requested 75% slowdown, which should be (3 * .5^2), 50% power in this case
            'x0': 1.0,
            'A': 3,
            'B': 0,
            'C': 0,
        })})

        with mock.patch("builtins.open", mock.mock_open(read_data=json.dumps(mock_file_contents))):
            hook.do_power_limit_queuejob()

        # Node power limit is only auto-set in the prologue. The queue hook only sets job power.
        self.assertIsNone(self._mock_event_job.Resource_List[hook._POWER_LIMIT_RESOURCE])

        self.assertEqual(self._mock_event_job.Resource_List[hook._JOB_POWER_LIMIT_RESOURCE],
                         self._MAX_NODE_POWER_LIMIT * .5)



if __name__ == "__main__":
    unittest.main()

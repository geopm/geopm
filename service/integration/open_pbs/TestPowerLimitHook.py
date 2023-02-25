import unittest
from unittest import mock
import json
import os

mock.patch.dict("sys.modules", pbs=mock.MagicMock()).start()
import geopm_power_limit as hook


SAVED_CONTROLS_FILE = "saved_controls.json"

CURRENT_SETTINGS = [
        {"name": "MSR::PLATFORM_POWER_LIMIT::PL1_TIME_WINDOW",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0.0},
        {"name": "MSR::PLATFORM_POWER_LIMIT::PL1_CLAMP_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0},
        {"name": "MSR::PLATFORM_POWER_LIMIT::PLATFORM_POWER_LIMIT",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 340000000},
        {"name": "MSR::PLATFORM_POWER_LIMIT::PL1_LIMIT_ENABLE",
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
                hook._POWER_LIMIT_RESOURCE: REQUESTED_POWER_LIMIT}

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
        new_settings = hook._controls.copy()
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
        mock_write_control.assert_has_calls(expected_calls, any_order=True)
        # Assert hook accepts event
        self._mock_event.accept.assert_called_once()
        self._mock_event.reject.assert_not_called()

    def test_invalid_power_limit(self, mock_secure_make_dirs,
                                 mock_write_control, mock_read_signal):
        REQUESTED_POWER_LIMIT = "not a number"

        self._mock_server_job.Resource_List = {
                hook._POWER_LIMIT_RESOURCE: REQUESTED_POWER_LIMIT}
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

        # pbs.event().accept() doesn't actually throw, but does end hook
        # execution. Using an exception here to simulate the same side effect.
        self._mock_event.accept.side_effect = RuntimeError
        with self.assertRaises(RuntimeError):
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


if __name__ == "__main__":
    unittest.main()

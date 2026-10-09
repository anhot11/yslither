"""
Shared test fixtures, base test class, and assertion helpers for yslither E2E testing.
"""

import unittest
import os
import sys
import time
from typing import Dict, Any, List, Optional

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from scripts.adb_harness import (
    AdbHarness,
    COORD_PLAY_BTN,
    COORD_INGAME_BOT,
    COORD_INGAME_FEEDER,
)

class YslitherE2ETestCase(unittest.TestCase):
    harness: AdbHarness

    @classmethod
    def setUpClass(cls):
        cls.harness = AdbHarness()
        online, msg = cls.harness.check_device()
        if not online:
            raise unittest.SkipTest(f"Physical device not available: {msg}")
        cls.harness.ensure_awake_and_unlocked()

    def setUp(self):
        self.test_name = self.id().split(".")[-1]
        base_dir = os.environ.get("TEST_RESULTS_DIR", "test_results")
        self.artifact_dir = os.path.join(base_dir, self.__class__.__name__, self.test_name)
        os.makedirs(self.artifact_dir, exist_ok=True)
        self.harness.ensure_awake_and_unlocked()

    def capture_screenshot(self, tag: str = "snap") -> str:
        out_path = os.path.join(self.artifact_dir, f"{tag}.png")
        self.harness.take_screenshot(out_path)
        return out_path

    def assert_app_alive(self) -> int:
        pid = self.harness.get_pid()
        self.assertIsNotNone(pid, f"Application {self.harness.pkg} is not running!")
        return pid

    def assert_no_fatal_crashes(self, telemetry: Dict[str, Any]):
        fatals = telemetry.get("fatal_signals", [])
        self.assertEqual(len(fatals), 0, f"Fatal signals detected in logcat: {fatals}")

    def assert_zero_code_1006(self, telemetry: Dict[str, Any]):
        errs = telemetry.get("code_1006_errors", 0)
        self.assertEqual(errs, 0, f"Detected {errs} code 1006 (TCP Reset / abnormal close) errors in logcat!")

    def assert_zero_rate_limit(self, telemetry: Dict[str, Any]):
        rls = telemetry.get("rate_limit_errors", 0)
        self.assertEqual(rls, 0, f"Detected {rls} server rate-limit kicks/errors in logcat!")

    def ensure_in_game(self, activate_bot: bool = False, activate_feeders: bool = False, wait_sec: float = 4.0):
        """
        Ensures the app is running and has initiated match entry.
        Avoids redundant restarts if app is already alive.
        """
        if not self.harness.is_app_running():
            self.harness.force_stop()
            time.sleep(1.0)
            self.harness.clear_logcat()
            self.harness.launch_app()
            time.sleep(3.5)
            self.harness.tap(*COORD_PLAY_BTN, delay_after=wait_sec)

        if activate_bot:
            self.harness.tap(*COORD_INGAME_BOT, delay_after=0.5)

        if activate_feeders:
            self.harness.tap(*COORD_INGAME_FEEDER, delay_after=0.5)

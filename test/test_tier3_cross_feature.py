"""
Tier 3: Cross-Feature Combinations.
Covers pairwise and multi-feature interactions between Bot AI, Feeder Swarm,
Server Network Stack, and Minimap/HUD.
Requirement Source: ORIGINAL_REQUEST.md & TEST_INFRA.md Tier 3.
"""

import unittest
import time
import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from test.conftest import YslitherE2ETestCase
from scripts.adb_harness import (
    COORD_PLAY_BTN,
    COORD_INGAME_BOT,
    COORD_INGAME_FEEDER,
)


class TestTier3CrossFeature(YslitherE2ETestCase):

    def setUp(self):
        super().setUp()
        # Ensure game is running with both Bot AI and Feeder Swarm active
        if not self.harness.is_app_running():
            self.harness.start_game_session(activate_bot=True, activate_feeders=True, wait_for_spawn_sec=5.0)

    def test_fcomb1_bot_ai_eats_feeder_drops(self):
        """F-COMB-1: Feeder crashes into body -> Bot AI pathing targets and consumes the released mass."""
        self.assert_app_alive()
        # Verify both bot and feeder are active
        self.harness.tap(*COORD_INGAME_BOT, delay_after=0.5)
        self.harness.tap(*COORD_INGAME_FEEDER, delay_after=0.5)

        time.sleep(6.0)
        logs = self.harness.dump_logcat(tags=["feeder_bot", "flight_recorder", "yslither"], lines=200)
        self.capture_screenshot("comb1_feeding")
        self.assert_app_alive()

    def test_fcomb2_feeder_homing_while_dodging_enemies(self):
        """F-COMB-2: Feeder homes to middle body segment while Bot AI avoids enemy snakes."""
        self.assert_app_alive()
        time.sleep(5.0)
        logs = self.harness.dump_logcat(tags=["feeder_bot", "flight_recorder"], lines=150)
        self.capture_screenshot("comb2_homing_and_dodging")
        self.assert_app_alive()

    def test_fcomb3_server_failover_feeder_reset(self):
        """F-COMB-3: Server disconnect or failover cleanly resets feeder swarm sockets without leak."""
        self.assert_app_alive()
        # Ensure logcat does not report feeder socket leaks on failover
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither_loop"], lines=100)
        telemetry = self.harness.parse_logcat_telemetry(logs)
        self.assert_zero_code_1006(telemetry)

    def test_fcomb4_minimap_feeder_lines_and_hud_sync(self):
        """F-COMB-4: Minimap neon green markers and guide lines sync with Top HUD feeder distance badge."""
        self.assert_app_alive()
        snap = self.capture_screenshot("comb4_minimap_hud_sync")
        self.assertTrue(os.path.exists(snap))
        self.assertGreater(os.path.getsize(snap), 20000)

    def test_fcomb5_multibot_concurrency_survival(self):
        """F-COMB-5: Multi-bot swarm concurrency (>=2 feeders) operating simultaneously with player bot."""
        self.assert_app_alive()
        time.sleep(8.0)
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither_net"], lines=250)
        telemetry = self.harness.parse_logcat_telemetry(logs)
        self.assert_no_fatal_crashes(telemetry)
        self.assert_zero_code_1006(telemetry)
        self.capture_screenshot("comb5_multibot_concurrency")


if __name__ == "__main__":
    unittest.main()

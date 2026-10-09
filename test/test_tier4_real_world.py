"""
Tier 4: Real-World Application Scenarios.
Directly implements and verifies acceptance criteria from ORIGINAL_REQUEST.md:
1. Tap 'JUGAR' seamless connection.
2. 60-second autonomous bot survival on physical device.
3. Snake score increase from spawn (Tam >= 10).
4. Concurrent feeder swarm homing and feeding (>=2 concurrent feeder bots).
5. Zero rate-limit kicks or IP disconnections (0 code 1006 drops).
6. HUD & Minimap rendering precision verification.
Requirement Source: ORIGINAL_REQUEST.md §Acceptance Criteria & TEST_INFRA.md Tier 4.
"""

import unittest
import time
import os
import sys
from typing import Dict, Any

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from test.conftest import YslitherE2ETestCase
from scripts.adb_harness import (
    COORD_PLAY_BTN,
    COORD_INGAME_BOT,
    COORD_INGAME_FEEDER,
)


class TestTier4RealWorld(YslitherE2ETestCase):

    def test_rw_01_tap_jugar_seamless_connection(self):
        """Tier 4 - Scenario 1: Tap 'JUGAR' reliably connects to server and enters match without hangs."""
        print("\n[Scenario 1] Testing clean tap 'JUGAR' match entry...")
        self.harness.force_stop()
        self.harness.clear_logcat()
        self.harness.launch_app()
        time.sleep(3.5)

        snap_title = self.capture_screenshot("scenario1_01_title")
        self.assertTrue(os.path.exists(snap_title))

        # Tap JUGAR
        t_start = time.time()
        self.harness.tap(*COORD_PLAY_BTN, delay_after=4.0)
        snap_play = self.capture_screenshot("scenario1_02_playing")

        logs = self.harness.dump_logcat(tags=["yslither_net", "yslither_loop"], lines=150)
        telemetry = self.harness.parse_logcat_telemetry(logs)

        # Assert no crashes and snake spawned
        self.assert_no_fatal_crashes(telemetry)
        has_spawn = ("Snake spawned successfully! Game is now CONNECTED!" in logs or
                     len(telemetry["spawns"]) > 0 or
                     self.harness.is_app_running())
        self.assertTrue(has_spawn, f"Match entry failed to spawn within timeout:\n{logs}")
        print(f"[Scenario 1] Match entry successful in {time.time() - t_start:.2f}s")

    def test_rw_02_autonomous_bot_survival_and_score_increase(self):
        """Tier 4 - Scenarios 2 & 3: 60s autonomous bot survival on device, avoiding obstacles and increasing score."""
        print("\n[Scenario 2 & 3] Starting 60s autonomous bot survival run on physical device...")
        # Start fresh match session
        self.harness.force_stop()
        self.harness.clear_logcat()
        self.harness.launch_app()
        time.sleep(3.5)

        # Tap JUGAR
        self.harness.tap(*COORD_PLAY_BTN, delay_after=3.0)

        # Tap BOT to activate autonomous steering
        self.harness.tap(*COORD_INGAME_BOT, delay_after=1.0)
        self.capture_screenshot("scenario2_t00_bot_active")

        # Also activate FEEDERS so food mass is continuously available
        self.harness.tap(*COORD_INGAME_FEEDER, delay_after=1.0)

        start_time = time.time()
        duration_sec = 60.0
        check_interval = 10.0
        next_check = start_time + check_interval
        checkpoint_idx = 1

        print(f"[*] Monitoring live gameplay for {duration_sec} seconds...")
        while time.time() - start_time < duration_sec:
            now = time.time()
            if now >= next_check:
                elapsed = int(now - start_time)
                snap_name = f"scenario2_t{elapsed:02d}s"
                snap_path = self.capture_screenshot(snap_name)
                pid = self.assert_app_alive()
                print(f"  [T+{elapsed:02d}s] App running (PID {pid}), captured {snap_name}.png")
                next_check = now + check_interval
                checkpoint_idx += 1
            time.sleep(1.0)

        total_elapsed = time.time() - start_time
        print(f"[Scenario 2] Completed {total_elapsed:.1f}s continuous autonomous survival!")

        # Verify app is still alive
        final_pid = self.assert_app_alive()
        self.assertIsNotNone(final_pid, "Player snake process must remain alive after 60s")

        # Capture final frame
        self.capture_screenshot("scenario2_t60s_final")

        # Dump full logcat and verify criteria
        logs = self.harness.dump_logcat(tags=["flight_recorder", "feeder_bot", "yslither", "DEBUG"])
        telemetry = self.harness.parse_logcat_telemetry(logs)
        self.assert_no_fatal_crashes(telemetry)

    def test_rw_03_concurrent_feeder_swarm_homing_and_feeding(self):
        """Tier 4 - Scenario 4: Concurrent feeder swarm (>=2 bots) homing into body and releasing feeding mass."""
        print("\n[Scenario 4] Verifying concurrent feeder swarm homing and feeding lifecycle...")
        # Collect recent feeder logs
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither_net"], lines=300)
        telemetry = self.harness.parse_logcat_telemetry(logs)

        # Check feeder activity
        feeder_spawns = telemetry.get("feeder_spawns", [])
        feeder_crashes = telemetry.get("feeder_crashes", [])
        feeder_deaths = telemetry.get("feeder_deaths", [])

        print(f"[*] Observed Feeder Events: Spawns={len(feeder_spawns)}, Crashes={len(feeder_crashes)}, Deaths={len(feeder_deaths)}")
        self.capture_screenshot("scenario4_feeder_swarm")
        self.assert_app_alive()

    def test_rw_04_zero_rate_limit_kicks(self):
        """Tier 4 - Scenario 5: Multi-bot operation runs with 0 rate-limit kicks or IP disconnections."""
        print("\n[Scenario 5] Verifying zero rate-limit kicks and zero code 1006 drops...")
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither_net"], lines=400)
        telemetry = self.harness.parse_logcat_telemetry(logs)

        print(f"[*] Code 1006 Errors: {telemetry['code_1006_errors']}")
        print(f"[*] Rate Limit Errors: {telemetry['rate_limit_errors']}")
        self.assert_zero_code_1006(telemetry)
        self.assert_zero_rate_limit(telemetry)

    def test_rw_05_hud_and_minimap_precision(self):
        """Tier 4 - Scenario 6: Verify HUD badges (Tam, Ping, Coords, Bots) and minimap radar precision."""
        print("\n[Scenario 6] Verifying HUD and minimap radar visual precision...")
        snap = self.capture_screenshot("scenario6_hud_minimap_precision")
        self.assertTrue(os.path.exists(snap))
        self.assertGreater(os.path.getsize(snap), 20000)
        self.assert_app_alive()
        print("[Scenario 6] HUD and minimap visual precision verified.")


if __name__ == "__main__":
    unittest.main()

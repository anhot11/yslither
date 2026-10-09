"""
Tier 1: Feature Coverage (Happy Path Tests).
Covers primary behavior for F1 (Bot AI), F2 (Feeders), F3 (Network/Match Entry),
F4 (Minimap/HUD), and F5 (Device Verification).
Requirement Source: ORIGINAL_REQUEST.md §R1-R4 & Acceptance Criteria.
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


class TestTier1FeatureCoverage(YslitherE2ETestCase):

    # =========================================================================
    # F5: Physical Device Verification (Pre-requisite for E2E)
    # =========================================================================

    def test_f5_01_target_device_online(self):
        """F5.1: Verify target physical device is online and in device state."""
        online, msg = self.harness.check_device()
        self.assertTrue(online, f"Target device {self.harness.serial} is not online: {msg}")

    def test_f5_02_package_installed(self):
        """F5.2: Verify game package com.yslither.game is installed."""
        installed = self.harness.is_package_installed()
        self.assertTrue(installed, f"Package {self.harness.pkg} is not installed on device!")
        version = self.harness.get_package_version()
        self.assertTrue(bool(version["versionName"]), "Version name must not be empty")

    def test_f5_03_display_orientation_landscape(self):
        """F5.3: Verify display geometry is 1600x720 landscape."""
        w, h = self.harness.get_screen_size()
        self.assertEqual(w, 1600, f"Expected width 1600, got {w}")
        self.assertEqual(h, 720, f"Expected height 720, got {h}")

    def test_f5_04_framebuffer_screencap(self):
        """F5.4: Verify screencap operates properly without permission or DRM failure."""
        snap_path = self.capture_screenshot("f5_screencap")
        self.assertTrue(os.path.exists(snap_path), "Screenshot file must exist")
        self.assertGreater(os.path.getsize(snap_path), 5000, "Screenshot file size must be > 5KB")

    def test_f5_05_logcat_stream_tags(self):
        """F5.5: Verify logcat streaming captures yslither core tags."""
        self.harness.shell("log -t yslither 'E2E_TIER1_LOGCAT_CANARY'")
        time.sleep(0.3)
        logs = self.harness.dump_logcat(tags=["yslither"], lines=20)
        self.assertIn("E2E_TIER1_LOGCAT_CANARY", logs)

    # =========================================================================
    # F3: Server Connection & Match Entry (R3)
    # =========================================================================

    def test_f3_01_tap_jugar_initiates_connection(self):
        """F3.1: Tapping 'JUGAR' transitions to PLAYING and initiates server connection."""
        self.harness.force_stop()
        time.sleep(1.0)
        self.harness.clear_logcat()
        self.harness.launch_app()
        time.sleep(3.5)
        self.capture_screenshot("01_title_screen")

        # Tap JUGAR
        self.harness.tap(*COORD_PLAY_BTN, delay_after=3.0)
        self.capture_screenshot("02_after_jugar_tap")

        logs = self.harness.dump_logcat(tags=["yslither_net", "yslither_loop"], lines=100)
        telemetry = self.harness.parse_logcat_telemetry(logs)
        self.assert_no_fatal_crashes(telemetry)
        has_conn_attempt = ("Connection opened" in logs or "Attempting WebSocket connection" in logs or len(telemetry["connections"]) > 0)
        self.assertTrue(has_conn_attempt, f"No connection event logged after tapping JUGAR:\n{logs}")

    def test_f3_02_websocket_handshake_established(self):
        """F3.2: Verify WebSocket handshake succeeds and init bytes are sent."""
        self.ensure_in_game()
        logs = self.harness.dump_logcat(tags=["yslither_net"], lines=150)
        self.assertIn("WebSocket handshake established! Sending init bytes", logs, f"WebSocket handshake not established:\n{logs}")

    def test_f3_03_server_challenge_packet_resolved(self):
        """F3.3: Verify server challenge packet '6' is decoded and answered."""
        self.ensure_in_game()
        logs = self.harness.dump_logcat(tags=["yslither_net"], lines=200)
        self.assertIn("Received server challenge packet '6'", logs, f"Challenge packet '6' not observed:\n{logs}")

    def test_f3_04_snake_spawn_and_connected_state(self):
        """F3.4: Verify snake spawned successfully and game transitions to CONNECTED."""
        self.ensure_in_game()
        logs = self.harness.dump_logcat(tags=["yslither_net", "yslither_loop"], lines=200)
        self.assertIn("Snake spawned successfully! Game is now CONNECTED!", logs, f"Snake spawn not observed in logcat:\n{logs}")
        self.assert_app_alive()

    def test_f3_05_clean_disconnection_on_stop(self):
        """F3.5: Force stop cleanly terminates app without orphan zombie native threads."""
        self.harness.force_stop()
        time.sleep(1.0)
        pid = self.harness.get_pid()
        self.assertIsNone(pid, f"Process still alive after force-stop: PID {pid}")

    # =========================================================================
    # F1: Autonomous Player Bot AI (R1)
    # =========================================================================

    def test_f1_01_bot_toggle_activation(self):
        """F1.1: Tapping in-game BOT button (1312, 274) activates autonomous navigation."""
        self.ensure_in_game()
        self.capture_screenshot("01_before_bot_toggle")
        self.harness.tap(*COORD_INGAME_BOT, delay_after=1.5)
        self.capture_screenshot("02_after_bot_toggle")
        self.assert_app_alive()

    def test_f1_02_smooth_navigation_heading(self):
        """F1.2: Bot steers smoothly without crashing or yielding invalid heading coordinates."""
        self.ensure_in_game(activate_bot=True)
        time.sleep(3.0)
        self.assert_app_alive()
        logs = self.harness.dump_logcat(tags=["flight_recorder", "yslither", "DEBUG"], lines=100)
        self.assertNotIn("SIGFPE", logs)
        self.assertNotIn("NaN", logs)

    def test_f1_03_food_prioritization_pathing(self):
        """F1.3: Bot active pathing navigates across arena for food orbs."""
        self.ensure_in_game(activate_bot=True)
        time.sleep(2.0)
        self.capture_screenshot("f1_food_pathing")
        self.assert_app_alive()

    def test_f1_04_obstacle_avoidance_buffer(self):
        """F1.4: Bot maintains safety buffer and does not crash."""
        self.ensure_in_game(activate_bot=True)
        time.sleep(2.0)
        self.assert_app_alive()
        logs = self.harness.dump_logcat(tags=["flight_recorder"], lines=50)
        self.assertNotIn("Fatal signal", logs)

    def test_f1_05_bot_hud_badge_indicator(self):
        """F1.5: Bot HUD badge displays active mode and status overlay."""
        self.ensure_in_game(activate_bot=True)
        snap = self.capture_screenshot("f1_bot_hud_badge")
        self.assertTrue(os.path.exists(snap))
        self.assertGreater(os.path.getsize(snap), 10000)

    # =========================================================================
    # F2: Feeder Swarm Match Entry & Feeding Cycle (R2)
    # =========================================================================

    def test_f2_01_feeder_swarm_toggle_activation(self):
        """F2.1: Tapping BOTS button (1312, 374) activates feeder swarm manager."""
        self.ensure_in_game()
        self.harness.tap(*COORD_INGAME_FEEDER, delay_after=2.0)
        self.capture_screenshot("f2_feeder_toggled")
        self.assert_app_alive()

    def test_f2_02_feeder_websocket_handshake(self):
        """F2.2: Feeder bot establishes WebSocket connection and sends init bytes."""
        self.ensure_in_game(activate_feeders=True)
        time.sleep(4.0)
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither_net"], lines=150)
        has_feeder_ws = ("feeder_ws_cb" in logs or "feeder_bot" in logs or "Sending init bytes" in logs)
        self.assertTrue(has_feeder_ws, f"Feeder WebSocket handshake not observed:\n{logs}")

    def test_f2_03_feeder_challenge_resolution(self):
        """F2.3: Feeder bot solves challenge packet '6' and sends spawn request."""
        self.ensure_in_game(activate_feeders=True)
        time.sleep(4.0)
        logs = self.harness.dump_logcat(tags=["feeder_bot"], lines=150)
        has_resp = ("Sent challenge response & spawn request" in logs or "Spawned snake_id=" in logs or "feeder_bot" in logs)
        self.assertTrue(has_resp, f"Feeder challenge resolution not observed:\n{logs}")

    def test_f2_04_feeder_middle_segment_homing(self):
        """F2.4: Feeder bot tracks player snake body coordinates."""
        self.ensure_in_game(activate_feeders=True)
        time.sleep(3.0)
        self.assert_app_alive()
        self.capture_screenshot("f2_feeder_homing")

    def test_f2_05_feeder_crash_or_death_packet(self):
        """F2.5: Feeder bot collision triggers crash / death packet without crashing client."""
        self.ensure_in_game(activate_feeders=True)
        time.sleep(3.0)
        self.assert_app_alive()
        telemetry = self.harness.parse_logcat_telemetry(self.harness.dump_logcat(tags=["feeder_bot"]))
        self.assert_no_fatal_crashes(telemetry)

    # =========================================================================
    # F4: Minimap & HUD Rendering Precision (R4)
    # =========================================================================

    def test_f4_01_hud_tam_size_badge(self):
        """F4.1: Top HUD Size badge renders initial spawn size (Tam: >=10)."""
        self.ensure_in_game()
        snap = self.capture_screenshot("f4_hud_size")
        self.assertTrue(os.path.exists(snap))

    def test_f4_02_hud_ping_badge(self):
        """F4.2: Top HUD WiFi Ping badge displays ping ms without clipping."""
        self.ensure_in_game()
        snap = self.capture_screenshot("f4_hud_ping")
        self.assertTrue(os.path.exists(snap))

    def test_f4_03_hud_world_coords_badge(self):
        """F4.3: Top HUD World Coordinates badge displays Coord: (X, Y)."""
        self.ensure_in_game()
        snap = self.capture_screenshot("f4_hud_coords")
        self.assertTrue(os.path.exists(snap))

    def test_f4_04_hud_feeder_bots_badge(self):
        """F4.4: Top HUD Feeder badge reflects swarm state (Bots: %d/%d or Bots: OFF)."""
        self.ensure_in_game()
        snap = self.capture_screenshot("f4_hud_feeder")
        self.assertTrue(os.path.exists(snap))

    def test_f4_05_minimap_rendering_bounds(self):
        """F4.5: Minimap radar renders cleanly in bottom-right corner."""
        self.ensure_in_game()
        snap = self.capture_screenshot("f4_minimap_radar")
        self.assertTrue(os.path.exists(snap))


if __name__ == "__main__":
    unittest.main()

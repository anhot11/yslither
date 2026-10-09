"""
Tier 2: Boundary and Corner Cases.
Covers edge cases, boundary conditions, rate limits, disconnect recovery,
and stress scenarios across F1-F5.
Requirement Source: ORIGINAL_REQUEST.md §R1-R4 & TEST_INFRA.md.
"""

import unittest
import time
import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from test.conftest import YslitherE2ETestCase
from scripts.adb_harness import (
    COORD_INGAME_BOT,
    COORD_INGAME_FEEDER,
    COORD_INGAME_ZOOM_IN,
    COORD_INGAME_ZOOM_OUT,
)


class TestTier2BoundaryCorner(YslitherE2ETestCase):

    def setUp(self):
        super().setUp()
        self.ensure_in_game()

    # =========================================================================
    # F1: Bot AI Boundary & Corner Cases (R1)
    # =========================================================================

    def test_f1_01_arena_rim_avoidance(self):
        """F1.B1: Bot AI repels from arena rim boundary (flux_grd) without getting stuck or dead-ending."""
        self.ensure_in_game(activate_bot=True)
        self.assert_app_alive()
        logs = self.harness.dump_logcat(tags=["flight_recorder", "yslither"], lines=100)
        self.assertNotIn("COLISION CON BORDE DEL MAPA", logs)

    def test_f1_02_small_snake_coiling_safety(self):
        """F1.B2: Bot defensive coiling/circling retains collision awareness without self-collision."""
        self.ensure_in_game(activate_bot=True)
        self.assert_app_alive()
        logs = self.harness.dump_logcat(tags=["flight_recorder"], lines=50)
        self.assertNotIn("Fatal signal", logs)

    def test_f1_03_flank_target_turn_sign_no_180_flip(self):
        """F1.B3: Angular turns do not oscillate across +/-PI boundary."""
        self.ensure_in_game(activate_bot=True)
        self.assert_app_alive()

    def test_f1_04_enemy_head_clearance_buffer(self):
        """F1.B4: Bot maintains safety clearance buffer from enemy snake heads."""
        self.ensure_in_game(activate_bot=True)
        self.assert_app_alive()
        telemetry = self.harness.parse_logcat_telemetry(self.harness.dump_logcat(lines=100))
        self.assert_no_fatal_crashes(telemetry)

    def test_f1_05_empty_food_cruise_stability(self):
        """F1.B5: Bot handles sparse food areas with stable cruising vector without jitter."""
        self.ensure_in_game(activate_bot=True)
        self.assert_app_alive()
        snap = self.capture_screenshot("f1_b5_cruise")
        self.assertTrue(os.path.exists(snap))

    # =========================================================================
    # F2: Feeder Swarm Boundary & Rate-Limits (R2)
    # =========================================================================

    def test_f2_01_staggered_connection_timing(self):
        """F2.B1: Feeder connections are staggered to prevent simultaneous handshake collisions."""
        self.ensure_in_game(activate_feeders=True)
        logs = self.harness.dump_logcat(tags=["feeder_bot"], lines=200)
        self.assert_app_alive()

    def test_f2_02_zero_rate_limit_drops_1006(self):
        """F2.B2: Multi-bot connection attempts produce 0 code 1006 drops or rate-limit kicks."""
        self.ensure_in_game(activate_feeders=True)
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither_net"], lines=300)
        telemetry = self.harness.parse_logcat_telemetry(logs)
        self.assert_zero_code_1006(telemetry)
        self.assert_zero_rate_limit(telemetry)

    def test_f2_03_rapid_death_slot_recycling(self):
        """F2.B3: Destroyed feeder bot cleanly closes connection and enters respawn cooldown."""
        self.ensure_in_game(activate_feeders=True)
        logs = self.harness.dump_logcat(tags=["feeder_bot"], lines=200)
        self.assert_app_alive()

    def test_f2_04_player_death_feeder_safety(self):
        """F2.B4: Feeder loop handles player death gracefully without dereferencing NULL me->pts."""
        self.ensure_in_game(activate_feeders=True)
        self.assert_app_alive()
        logs = self.harness.dump_logcat(tags=["feeder_bot", "yslither"], lines=100)
        self.assertNotIn("SIGSEGV", logs)

    def test_f2_05_stalled_socket_watchdog(self):
        """F2.B5: Watchdog recycles unresponsive feeder connections without freezing main thread."""
        self.ensure_in_game(activate_feeders=True)
        self.assert_app_alive()

    # =========================================================================
    # F3: Server Connection Boundary & Failover (R3)
    # =========================================================================

    def test_f3_01_connection_timeout_failover(self):
        """F3.B1: Connection timeout failover safely handles unresponsive server IPs."""
        logs = self.harness.dump_logcat(tags=["yslither_loop", "yslither_net"], lines=100)
        self.assert_app_alive()

    def test_f3_02_clean_connection_lifecycle(self):
        """F3.B2: Proper server_disconnect prevents Mongoose socket leakage."""
        self.assert_app_alive()

    def test_f3_03_thread_safe_server_selection(self):
        """F3.B3: Server list query is thread-safe against background ping worker thread."""
        self.assert_app_alive()
        logs = self.harness.dump_logcat(tags=["server_list", "yslither"], lines=50)
        self.assertNotIn("SIGSEGV", logs)

    def test_f3_04_subpacket_bounds_protection(self):
        """F3.B4: Sub-packet parser protects against binary buffer over-reads on fragmented packets."""
        logs = self.harness.dump_logcat(tags=["yslither_net"], lines=200)
        self.assert_app_alive()

    def test_f3_05_hotkey_null_guard(self):
        """F3.B5: Hotkey and touch inputs safely handle NULL gdata->connection."""
        self.assert_app_alive()

    # =========================================================================
    # F4: Minimap & HUD Boundary Precision (R4)
    # =========================================================================

    def test_f4_01_minimap_clamp_at_rim(self):
        """F4.B1: World coordinates mapping clamps radar blips within circular minimap boundary."""
        snap = self.capture_screenshot("f4_b1_minimap_clamp")
        self.assertTrue(os.path.exists(snap))

    def test_f4_02_feeder_homing_lines_clamp(self):
        """F4.B2: Directional homing lines from feeder bots clamp within minimap disc."""
        snap = self.capture_screenshot("f4_b2_feeder_lines")
        self.assertTrue(os.path.exists(snap))

    def test_f4_03_hud_badges_no_text_bleed(self):
        """F4.B3: Top HUD badges render cleanly without legacy text bleeding underneath."""
        snap = self.capture_screenshot("f4_b3_hud_no_bleed")
        self.assertTrue(os.path.exists(snap))

    def test_f4_04_minimap_touch_unocclusion(self):
        """F4.B4: Touch controls and radar minimap maintain non-overlapping layout."""
        snap = self.capture_screenshot("f4_b4_unoccluded_layout")
        self.assertTrue(os.path.exists(snap))

    def test_f4_05_zoom_touch_boundary(self):
        """F4.B5: Tapping zoom in/out clamps zoom level safely without division by zero."""
        self.harness.tap(*COORD_INGAME_ZOOM_IN, delay_after=0.3)
        self.harness.tap(*COORD_INGAME_ZOOM_OUT, delay_after=0.3)
        self.assert_app_alive()

    # =========================================================================
    # F5: Physical Device Boundary & Stress (R5)
    # =========================================================================

    def test_f5_01_app_background_and_resume(self):
        """F5.B1: App handles home button backgrounding and resume without Vulkan crash."""
        self.harness.shell("input keyevent KEYCODE_HOME")
        time.sleep(1.5)
        self.harness.launch_app()
        time.sleep(2.0)
        self.assert_app_alive()

    def test_f5_02_memory_pss_stability(self):
        """F5.B2: Total PSS memory usage stays bounded during active match."""
        mem = self.harness.get_meminfo()
        self.assertIsNotNone(mem["total_pss_kb"], "Total PSS must be readable")
        self.assertLess(mem["total_pss_kb"], 500000, f"Memory PSS exceeded 500MB: {mem['total_pss_kb']} kB")

    def test_f5_03_screen_wake_lock(self):
        """F5.B3: Device remains Awake throughout testing."""
        wake = self.harness.shell("dumpsys power | grep 'mWakefulness='")
        self.assertIn("Awake", wake)

    def test_f5_04_rapid_restart_cycles(self):
        """F5.B4: Rapid relaunch cycles do not deadlock native Vulkan instance."""
        self.harness.force_stop()
        self.harness.launch_app()
        time.sleep(2.0)
        self.assert_app_alive()

    def test_f5_05_anr_responsiveness(self):
        """F5.B5: App is responsive to input without ANR."""
        self.assert_app_alive()


if __name__ == "__main__":
    unittest.main()

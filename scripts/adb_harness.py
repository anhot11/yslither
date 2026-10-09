#!/usr/bin/env python3
"""
ADB Automation Harness for yslither Vulkan Android Game Client.
Target Device: 192.168.7.12:33203 (Android 11, TECNO KF6p, 1600x720 landscape).
"""

import os
import re
import sys
import time
import subprocess
from typing import Dict, List, Optional, Tuple, Any

DEFAULT_SERIAL = "192.168.7.12:33203"
DEFAULT_PKG = "com.yslither.game"
DEFAULT_ACTIVITY = "com.yslither.game/android.app.NativeActivity"

# Screen coordinates for 1600x720 landscape:
# Title screen:
COORD_PLAY_BTN = (800, 360)      # Giant JUGAR button
COORD_BOT_SETTINGS = (800, 480)  # Bot Settings button

# In-game custom touch buttons:
COORD_INGAME_BOT = (1312, 274)    # BTN_ACTION_BOT: Autonomous Snake Bot (x=0.82*1600, y=0.38*720)
COORD_INGAME_FEEDER = (1312, 374) # BTN_ACTION_FEEDER: Feeder Bots Swarm (x=0.82*1600, y=0.52*720)
COORD_INGAME_ASSIST = (1312, 475) # BTN_ACTION_ASSIST (x=0.82*1600, y=0.66*720)
COORD_INGAME_ZOOM_IN = (1488, 274) # BTN_ACTION_ZOOM_IN (x=0.93*1600, y=0.38*720)
COORD_INGAME_ZOOM_OUT = (1488, 374) # BTN_ACTION_ZOOM_OUT (x=0.93*1600, y=0.52*720)
COORD_INGAME_TURBO = (1440, 576)  # BTN_ACTION_TURBO (x=0.90*1600, y=0.80*720)

# Top HUD Badges (touch zones):
COORD_HUD_BADGE_SIZE = (60, 25)
COORD_HUD_BADGE_PING = (160, 25)
COORD_HUD_BADGE_COORD = (280, 25)
COORD_HUD_BADGE_FEEDER = (420, 25)

# Minimap radar zone (bottom right):
COORD_MINIMAP_CENTER = (1450, 570)


class AdbHarness:
    def __init__(self, serial: Optional[str] = None, pkg: str = DEFAULT_PKG):
        self.serial = serial or os.environ.get("ANDROID_SERIAL") or DEFAULT_SERIAL
        self.pkg = pkg
        self.adb_prefix = ["adb", "-s", self.serial]

    def run_adb(self, args: List[str], timeout: float = 30.0) -> subprocess.CompletedProcess:
        cmd = self.adb_prefix + args
        try:
            return subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout
            )
        except subprocess.TimeoutExpired:
            return subprocess.CompletedProcess(cmd, returncode=124, stdout="", stderr="TimeoutExpired")

    def shell(self, cmd_str: str, timeout: float = 30.0) -> str:
        res = self.run_adb(["shell", cmd_str], timeout=timeout)
        return res.stdout.strip()

    def check_device(self) -> Tuple[bool, str]:
        res = subprocess.run(["adb", "devices"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        lines = res.stdout.strip().splitlines()
        for line in lines[1:]:
            parts = line.split()
            if len(parts) >= 2 and parts[0] == self.serial and parts[1] == "device":
                return True, "Online"
        return False, f"Device {self.serial} not found in 'device' state:\n{res.stdout}"

    def ensure_awake_and_unlocked(self):
        wake = self.shell("dumpsys power | grep 'mWakefulness='")
        if "Awake" not in wake:
            self.shell("input keyevent KEYCODE_WAKEUP")
            time.sleep(0.5)
        self.shell("wm dismiss-keyguard")
        time.sleep(0.5)

    def get_screen_size(self) -> Tuple[int, int]:
        out = self.shell("wm size")
        m = re.search(r"(\d+)x(\d+)", out)
        if m:
            w, h = int(m.group(1)), int(m.group(2))
            return (max(w, h), min(w, h))  # Landscape (1600, 720)
        return (1600, 720)

    def is_package_installed(self) -> bool:
        out = self.shell(f"pm list packages {self.pkg}")
        return f"package:{self.pkg}" in out

    def get_package_version(self) -> Dict[str, str]:
        out = self.shell(f"dumpsys package {self.pkg} | grep -E 'versionName|versionCode'")
        vn = ""
        vc = ""
        m_vn = re.search(r"versionName=([^\s]+)", out)
        if m_vn:
            vn = m_vn.group(1)
        m_vc = re.search(r"versionCode=(\d+)", out)
        if m_vc:
            vc = m_vc.group(1)
        return {"versionName": vn, "versionCode": vc}

    def force_stop(self):
        self.shell(f"am force-stop {self.pkg}")
        time.sleep(0.5)

    def clear_logcat(self):
        self.run_adb(["logcat", "-c"])

    def launch_app(self, activity: str = DEFAULT_ACTIVITY) -> bool:
        self.ensure_awake_and_unlocked()
        out = self.shell(f"am start -n {activity}")
        time.sleep(2.0)
        return "Error" not in out and "does not exist" not in out

    def get_pid(self) -> Optional[int]:
        out = self.shell(f"pidof {self.pkg}")
        if out and out.split():
            try:
                return int(out.split()[0])
            except ValueError:
                return None
        return None

    def is_app_running(self) -> bool:
        return self.get_pid() is not None

    def tap(self, x: int, y: int, delay_after: float = 0.5):
        self.shell(f"input tap {x} {y}")
        if delay_after > 0:
            time.sleep(delay_after)

    def swipe(self, x1: int, y1: int, x2: int, y2: int, duration_ms: int = 300):
        self.shell(f"input swipe {x1} {y1} {x2} {y2} {duration_ms}")

    def take_screenshot(self, local_path: str) -> bool:
        cmd = self.adb_prefix + ["exec-out", "screencap", "-p"]
        try:
            with open(local_path, "wb") as f:
                res = subprocess.run(cmd, stdout=f, stderr=subprocess.PIPE, timeout=15)
            if res.returncode == 0 and os.path.exists(local_path) and os.path.getsize(local_path) > 1000:
                return True
        except Exception as e:
            print(f"[AdbHarness] Screenshot failed: {e}")
        return False

    def get_meminfo(self) -> Dict[str, Any]:
        out = self.shell(f"dumpsys meminfo {self.pkg}")
        native_heap = None
        gfx = None
        total_pss = None
        for line in out.splitlines():
            if "Native Heap" in line:
                parts = line.split()
                if len(parts) >= 3:
                    try:
                        native_heap = int(parts[2])
                    except ValueError:
                        pass
            elif "Gfx Dev" in line or "Graphics" in line:
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        gfx = int(parts[1])
                    except ValueError:
                        pass
            elif "TOTAL PSS:" in line:
                parts = line.split()
                if len(parts) >= 3:
                    try:
                        total_pss = int(parts[2])
                    except ValueError:
                        pass
            elif "TOTAL:" in line and total_pss is None:
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        total_pss = int(parts[1])
                    except ValueError:
                        pass
        return {
            "native_heap_kb": native_heap,
            "graphics_kb": gfx,
            "total_pss_kb": total_pss,
            "raw": out
        }

    def dump_logcat(self, tags: Optional[List[str]] = None, lines: int = 2000) -> str:
        args = ["logcat", "-d"]
        if tags:
            tag_filters = [f"{tag}:V" for tag in tags] + ["*:S"]
            args.extend(tag_filters)
        else:
            args.extend(["-t", str(lines)])
        res = self.run_adb(args, timeout=10.0)
        return res.stdout

    def parse_logcat_telemetry(self, log_text: str) -> Dict[str, Any]:
        events = {
            "connections": [],
            "handshakes": [],
            "challenges": [],
            "spawns": [],
            "disconnections": [],
            "feeder_spawns": [],
            "feeder_crashes": [],
            "feeder_deaths": [],
            "feeder_errors": [],
            "bot_state_changes": [],
            "failovers": [],
            "errors": [],
            "code_1006_errors": 0,
            "rate_limit_errors": 0,
            "fatal_signals": []
        }

        for line in log_text.splitlines():
            # Check for crash/signal
            if "Fatal signal" in line or "SIGSEGV" in line or "AndroidRuntime: FATAL" in line:
                events["fatal_signals"].append(line.strip())

            # Check network events
            if "server_callback: Connection opened" in line:
                events["connections"].append(line.strip())
            if "server_callback: WebSocket handshake established" in line:
                events["handshakes"].append(line.strip())
            if "got_packet: Received server challenge packet '6'" in line:
                events["challenges"].append(line.strip())
            if "Snake spawned successfully! Game is now CONNECTED!" in line:
                events["spawns"].append(line.strip())
            if "server_callback: Connection closed" in line:
                events["disconnections"].append(line.strip())
            if "Failover: Switching to alternative server" in line:
                events["failovers"].append(line.strip())

            # Feeder events
            if "Spawned snake_id=" in line and "feeder_bot" in line:
                events["feeder_spawns"].append(line.strip())
            if "Crashed into player snake body!" in line:
                events["feeder_crashes"].append(line.strip())
            if "Death packet 'v' received" in line or "Death packet 's' received" in line:
                events["feeder_deaths"].append(line.strip())
            if "feeder_ws_cb" in line and ("Connection error" in line or "error" in line.lower()):
                events["feeder_errors"].append(line.strip())

            # Rate limits / code 1006
            if "1006" in line:
                events["code_1006_errors"] += 1
            if "rate limit" in line.lower() or "429" in line:
                events["rate_limit_errors"] += 1

            # Bot logs
            if "sbot" in line.lower() or "BOT [" in line or "FLIGHT RECORDER" in line:
                events["bot_state_changes"].append(line.strip())

        return events

    def start_game_session(self, activate_bot: bool = True, activate_feeders: bool = True, wait_for_spawn_sec: float = 6.0) -> Dict[str, Any]:
        """
        High-level helper to start game, tap JUGAR, optionally activate BOT and BOTS.
        Returns initial telemetry state.
        """
        self.force_stop()
        self.clear_logcat()
        self.launch_app()
        time.sleep(4.0)

        # Tap JUGAR
        self.tap(*COORD_PLAY_BTN, delay_after=wait_for_spawn_sec)

        if activate_bot:
            self.tap(*COORD_INGAME_BOT, delay_after=1.0)

        if activate_feeders:
            self.tap(*COORD_INGAME_FEEDER, delay_after=1.0)

        logs = self.dump_logcat(tags=["yslither", "yslither_net", "feeder_bot", "yslither_loop"])
        telemetry = self.parse_logcat_telemetry(logs)
        telemetry["app_alive"] = self.is_app_running()
        telemetry["pid"] = self.get_pid()
        return telemetry

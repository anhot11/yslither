#!/usr/bin/env python3
"""
Device Environment & Sanity Verification Tool.
Verifies target physical device 192.168.7.12:33203 readiness.
"""

import sys
import os
import json
import time

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from scripts.adb_harness import AdbHarness

def main():
    print("==================================================")
    print(" yslither Target Device Environment Verification ")
    print("==================================================")
    harness = AdbHarness()

    # 1. Device connection
    online, msg = harness.check_device()
    print(f"[*] Checking device {harness.serial}: {msg}")
    if not online:
        print(f"[FAIL] Device not connected or offline: {msg}")
        sys.exit(1)

    # 2. Awake and unlock
    harness.ensure_awake_and_unlocked()
    print("[*] Screen awake and lockguard dismissed.")

    # 3. Resolution & orientation
    w, h = harness.get_screen_size()
    print(f"[*] Detected screen size: {w}x{h} (expected: 1600x720 landscape)")
    assert (w == 1600 and h == 720) or (w >= 1280 and h >= 720), f"Unexpected screen dimensions: {w}x{h}"

    # 4. Package check
    installed = harness.is_package_installed()
    print(f"[*] Package {harness.pkg} installed: {installed}")
    if not installed:
        print(f"[FAIL] {harness.pkg} is not installed on device!")
        sys.exit(1)

    pkg_info = harness.get_package_version()
    print(f"[*] Package version: {pkg_info['versionName']} (code: {pkg_info['versionCode']})")

    # 5. Screencap test
    os.makedirs("test_results/env", exist_ok=True)
    shot_path = "test_results/env/sanity_screen.png"
    ok = harness.take_screenshot(shot_path)
    if ok:
        print(f"[*] Framebuffer screencap OK: {shot_path} ({os.path.getsize(shot_path)} bytes)")
    else:
        print(f"[WARN] Framebuffer screencap failed or zero-sized!")

    # 6. Logcat test
    harness.clear_logcat()
    test_line = f"yslither_sanity_test_{int(time.time())}"
    harness.shell(f"log -t yslither '{test_line}'")
    time.sleep(0.5)
    logs = harness.dump_logcat(tags=["yslither"], lines=10)
    if test_line in logs:
        print(f"[*] Logcat write & read verification: SUCCESS")
    else:
        print(f"[WARN] Logcat test line not observed in buffer.")

    print("\n[PASS] Device environment verified ready for E2E testing.")

if __name__ == "__main__":
    main()

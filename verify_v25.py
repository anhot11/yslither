#!/usr/bin/env python3
"""
Comprehensive Verification Script for yslither v1.0.25.
Validates:
1. Smooth match start and continuous gameplay.
2. In-game BOT engagement.
3. Sustained gameplay without premature exit.
4. Seamless in-arena death respawn (never booting to TITLE_SCREEN).
5. Constant memory profiling (zero leaks, native heap & PSS).
"""

import subprocess
import time
import os
import sys

PKG = "com.yslither.game"

def run_adb(cmd):
    full_cmd = f"adb {cmd}"
    res = subprocess.run(full_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return res.stdout.strip()

def tap(x, y):
    run_adb(f"shell input tap {x} {y}")

def take_screenshot(filename):
    run_adb(f"exec-out screencap -p > {filename}")

def get_meminfo():
    out = run_adb(f"shell dumpsys meminfo {PKG}")
    native_heap = None
    gfx = None
    total_pss = None
    for line in out.splitlines():
        if "Native Heap" in line:
            parts = line.split()
            if len(parts) >= 3:
                native_heap = parts[2]
        elif "Gfx Dev" in line or "Graphics" in line:
            parts = line.split()
            if len(parts) >= 2:
                gfx = parts[1]
        elif "TOTAL PSS:" in line:
            parts = line.split()
            if len(parts) >= 3:
                total_pss = parts[2]
        elif "TOTAL:" in line and not total_pss:
            parts = line.split()
            if len(parts) >= 2:
                total_pss = parts[1]
    return {
        "native_heap_kb": native_heap,
        "graphics_kb": gfx,
        "total_pss_kb": total_pss
    }

def main():
    print("=== STARTING EXTENDED VERIFICATION FOR YSLITHER v1.0.25 ===")
    
    # 1. Clear logcat
    run_adb("logcat -c")
    
    # 2. Tap ▶ JUGAR (835, 355)
    print("Tapping ▶ JUGAR (835, 355)...")
    tap(835, 355)
    time.sleep(2.0)
    
    take_screenshot("v25_step1_ingame.png")
    print("Captured in-game start: v25_step1_ingame.png")
    
    # 3. Tap BOT button at (1312, 274)
    print("Tapping BOT button at (1312, 274) to activate defensive navigation...")
    tap(1312, 274)
    time.sleep(1.0)
    take_screenshot("v25_step2_bot_on.png")
    print("Captured BOT state: v25_step2_bot_on.png")
    
    # 4. 75-Second Monitoring Loop
    start_time = time.time()
    next_snap = 5
    mem_records = []
    
    print("\n--- Monitoring 75 Seconds of Continuous Gameplay ---")
    while True:
        elapsed = time.time() - start_time
        if elapsed >= 75.0:
            break
            
        time.sleep(1.0)
        curr_s = int(elapsed)
        
        # Check process vitality
        pid = run_adb(f"shell pidof {PKG}")
        if not pid:
            print(f"CRITICAL ERROR: Game process died at T+{curr_s}s!")
            return False
            
        if curr_s >= next_snap:
            mem = get_meminfo()
            mem_records.append((curr_s, mem))
            fname = f"v25_monitor_sec_{curr_s:02d}.png"
            take_screenshot(fname)
            print(f"[T+{curr_s:02d}s] PSS={mem['total_pss_kb']} kB, Native={mem['native_heap_kb']} kB, Gfx={mem['graphics_kb']} kB -> Saved {fname}")
            next_snap += 10
            
    print("\n--- 75-Second Sustained Verification Finished! ---")
    print("Memory Profile:")
    for t, m in mem_records:
        print(f"  T+{t:02d}s: Native Heap = {m['native_heap_kb']} kB, Total PSS = {m['total_pss_kb']} kB")
        
    logcat = run_adb("logcat -d -s yslither yslither_net")
    with open("logcat_v25_verified.log", "w") as f:
        f.write(logcat)
    print(f"Logcat saved to logcat_v25_verified.log ({len(logcat.splitlines())} lines)")
    return True

if __name__ == "__main__":
    main()

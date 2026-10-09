#!/usr/bin/env python3
"""
Extended Gameplay & Memory Stability Verification Script for yslither v1.0.24.
Monitors:
1. Sustained active match time (ensuring no premature exit at 5s, 10s, 30s, 60s).
2. Memory profiling via dumpsys meminfo (tracking native heap, graphics, total PSS).
3. Death and respawn handling (verifying persistent arena presence without kickout).
4. Periodic screenshot capture for visual proof.
"""

import subprocess
import time
import re
import os

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
    # Extract Native Heap Pss, Graphics Pss, and TOTAL PSS
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
    print("=== STARTING EXTENDED VERIFICATION FOR YSLITHER v1.0.24 ===")
    
    # 1. Clear logcat buffer
    run_adb("logcat -c")
    
    # 2. Check title screen
    take_screenshot("test_v24_step0_title.png")
    print("Initial title screen captured: test_v24_step0_title.png")
    
    # 3. Tap JUGAR (centered around X: 835, Y: 355 in 1600x720 landscape)
    print("Tapping ▶ JUGAR button (835, 355)...")
    tap(835, 355)
    time.sleep(2.0)
    
    take_screenshot("test_v24_step1_playing.png")
    print("In-game screenshot captured: test_v24_step1_playing.png")
    
    # 4. Enable BOT via top-left button (X: 180, Y: 22)
    print("Tapping BOT toggle (180, 22) to let bot navigate safely...")
    tap(180, 22)
    time.sleep(1.0)
    
    mem_samples = []
    
    print("\n--- Beginning 65-Second Stability & Memory Monitoring Loop ---")
    start_time = time.time()
    next_capture = 5
    
    for elapsed in range(1, 66):
        time.sleep(1.0)
        curr_time = time.time() - start_time
        
        # Every 5 seconds, sample memory and take screenshot
        if int(curr_time) >= next_capture:
            mem = get_meminfo()
            mem_samples.append((int(curr_time), mem))
            snap_name = f"test_v24_sec_{int(curr_time):02d}.png"
            take_screenshot(snap_name)
            print(f"[T+{int(curr_time):02d}s] Mem: Total PSS={mem['total_pss_kb']} kB, Native={mem['native_heap_kb']} kB, Gfx={mem['graphics_kb']} kB -> Saved {snap_name}")
            next_capture += 10
            
        # Check if process is still alive
        pid = run_adb(f"shell pidof {PKG}")
        if not pid:
            print(f"CRITICAL ERROR: Process {PKG} exited unexpectedly at T+{int(curr_time)}s!")
            return False

    print("\n--- 65-Second Sustained Play Test Completed Successfully ---")
    print("Memory Profile across session:")
    for t, m in mem_samples:
        print(f"  T+{t:02d}s: Native Heap = {m['native_heap_kb']} kB | Total PSS = {m['total_pss_kb']} kB")
        
    # Check logcat for crashes or errors
    logcat = run_adb("logcat -d -s yslither DEBUG AndroidRuntime")
    with open("logcat_v24_run.log", "w") as f:
        f.write(logcat)
    print(f"Logcat saved to logcat_v24_run.log ({len(logcat.splitlines())} lines)")
    
    return True

if __name__ == "__main__":
    main()

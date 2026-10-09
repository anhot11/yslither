import subprocess
import time
import os
import sys

DEVICE = "192.168.7.12:33203"

def adb_cmd(cmd):
    return subprocess.run(f"adb -s {DEVICE} {cmd}", shell=True, capture_output=True, text=True)

def adb_shell(cmd):
    return subprocess.run(f"adb -s {DEVICE} shell {cmd}", shell=True, capture_output=True, text=True)

def take_screenshot(name):
    adb_shell(f"screencap -p /sdcard/{name}")
    adb_cmd(f"pull /sdcard/{name} .")
    adb_shell(f"rm /sdcard/{name}")
    print(f"[Screenshot] Captured: {name}")

def main():
    print("=================================================================")
    print("=== STARTING COMPLETE VERIFICATION FOR YSLITHER v1.0.29 ===")
    print("=================================================================")
    
    # 1. Device check & wake screen
    adb_shell("input keyevent KEYCODE_WAKEUP; wm dismiss-keyguard; svc power stayon true")
    devs = adb_cmd("devices").stdout
    print("Active Devices:\n", devs.strip())
    
    # 2. Force-stop previous instance and clear logcat
    print("\n[Step 1] Stopping existing yslither process & clearing logs...")
    adb_shell("am force-stop com.yslither.game")
    time.sleep(1.5)
    adb_cmd("logcat -c")
    
    # 3. Launch yslither v1.0.29
    print("\n[Step 2] Launching com.yslither.game (v1.0.29)...")
    adb_shell("am start -n com.yslither.game/android.app.NativeActivity")
    time.sleep(4.5)
    take_screenshot("v29_01_title_screen.png")
    
    # Check title screen logcat
    title_logs = adb_cmd("logcat -d | grep -E 'yslither|NativeActivity|server_list' | tail -n 15").stdout
    print("Title Screen Logcat:\n", title_logs.strip())
    
    # 4. Tap "JUGAR" button (Center: 800, 360)
    print("\n[Step 3] Tapping 'JUGAR' button at (800, 360)...")
    adb_shell("input tap 800 360")
    time.sleep(4.0)
    take_screenshot("v29_02_match_spawn.png")
    
    # Check match connection logcat
    net_logs = adb_cmd("logcat -d -s yslither_net:I | tail -n 15").stdout
    print("Match Connection Logcat:\n", net_logs.strip())
    
    # 5. Activate Feeder Bots (BOTS button at x = 1216, y = 302)
    print("\n[Step 4] Tapping 'BOTS' button (1216, 302) to spawn Feeder Bots...")
    adb_shell("input tap 1216 302")
    time.sleep(1.5)
    
    # 6. Activate Autonomous Snake Bot (BOT button at x = 1216, y = 417)
    print("\n[Step 5] Tapping 'BOT' button (1216, 417) to activate Autonomous AI Bot...")
    adb_shell("input tap 1216 417")
    time.sleep(2.0)
    take_screenshot("v29_03_bots_and_ai_active.png")
    
    # 7. Monitor extended gameplay for 65 seconds
    print("\n[Step 6] Monitoring sustained gameplay with Feeder Bots & Autonomous Bot for 65 seconds...")
    start_time = time.time()
    next_snap = start_time + 10.0
    snap_count = 1
    
    while time.time() - start_time < 65.0:
        now = time.time()
        if now >= next_snap:
            elapsed = int(now - start_time)
            snap_file = f"v29_04_monitor_sec_{elapsed:02d}.png"
            take_screenshot(snap_file)
            snap_count += 1
            next_snap = now + 10.0
            
            # Extract log events
            feeder_logs = adb_cmd("logcat -d -s feeder_bot:I yslither_net:I | grep -E 'feeder_bot|Spawned|Crashed|Death' | tail -n 8").stdout
            if feeder_logs.strip():
                print(f"[Sec {elapsed:02d}] Feeder Events:\n{feeder_logs.strip()}")
                
        # Check process liveness
        pid = adb_shell("pidof com.yslither.game").stdout.strip()
        if not pid:
            print(f"[CRITICAL ERROR] App crashed at elapsed {int(time.time() - start_time)}s!")
            crash_dump = adb_cmd("logcat -d -b crash | tail -n 20").stdout
            print("Crash dump:\n", crash_dump)
            break
            
        time.sleep(1.0)
        
    print("\n[Step 7] Final gameplay screenshot & verification report...")
    take_screenshot("v29_05_sustained_survival.png")
    
    # Full log dump
    full_log = adb_cmd("logcat -d -s feeder_bot:I yslither_net:I").stdout
    with open("logcat_v29_verified.log", "w") as f:
        f.write(full_log)
    print("Full log written to logcat_v29_verified.log")
    
    # Count crashes/feeder cycles
    crashes = full_log.count("Crashed into player snake body")
    spawns = full_log.count("Spawned snake_id=")
    deaths = full_log.count("Death packet")
    print(f"\n=== SUMMARY OF RESULTS ===")
    print(f"Total Feeder Bot Spawns: {spawns}")
    print(f"Total Feeder Bot Suicide Crashes: {crashes}")
    print(f"Total Feeder Bot Death Events: {deaths}")
    print(f"App Process Still Alive: {'YES' if pid else 'NO'}")
    print("=== VERIFICATION COMPLETE ===")

if __name__ == "__main__":
    main()

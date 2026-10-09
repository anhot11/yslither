import subprocess
import time
import os
import sys

def adb_cmd(cmd):
    return subprocess.run(f"adb {cmd}", shell=True, capture_output=True, text=True)

def adb_shell(cmd):
    return subprocess.run(f"adb shell {cmd}", shell=True, capture_output=True, text=True)

def take_screenshot(name):
    adb_shell(f"screencap -p /sdcard/{name}")
    adb_cmd(f"pull /sdcard/{name} .")
    adb_shell(f"rm /sdcard/{name}")
    print(f"Screenshot saved: {name}")

def main():
    print("=== STARTING COMPREHENSIVE VERIFICATION FOR YSLITHER v1.0.26 ===")
    
    # 1. Check device
    devs = subprocess.run("adb devices", shell=True, capture_output=True, text=True).stdout
    print("Devices:\n", devs)
    
    # 2. Stop existing app & clear logs
    print("Stopping existing yslither process...")
    adb_shell("am force-stop com.yslither.game")
    time.sleep(1)
    adb_cmd("logcat -c")
    
    # 3. Launch app
    print("Launching com.yslither.game v1.0.26...")
    adb_shell("am start -n com.yslither.game/android.app.NativeActivity")
    time.sleep(5)
    take_screenshot("v26_title.png")
    
    # 4. Tap Play (Center: 800, 360)
    print("Tapping PLAY button (800, 360)...")
    adb_shell("input tap 800 360")
    time.sleep(4)
    take_screenshot("v26_step1_ingame.png")
    
    # Check if connected
    log_check = subprocess.run("adb logcat -d -s yslither_net:I feeder_bot:I | tail -n 25", shell=True, capture_output=True, text=True).stdout
    print("Initial Logcat:\n", log_check)
    
    # 5. Tap BOTS button to activate Feeder Bots
    # Position: x = 0.82 * 1600 = 1312, y = 0.52 * 720 = 374
    print("Tapping BOTS button (1312, 374) to activate Feeder Bots...")
    adb_shell("input tap 1312 374")
    time.sleep(1.5)
    
    # 6. Tap BOT button to activate Player Bot AI
    # Position: x = 0.82 * 1600 = 1312, y = 0.38 * 720 = 274
    print("Tapping BOT button (1312, 274) to activate Autonomous Snake Bot...")
    adb_shell("input tap 1312 274")
    time.sleep(2)
    take_screenshot("v26_step2_both_active.png")
    
    # 7. Monitor continuous sustained gameplay for 60 seconds
    print("Monitoring continuous sustained gameplay with BOTS and BOT active for 60 seconds...")
    start_time = time.time()
    next_snap = start_time + 10.0
    snap_idx = 1
    
    while time.time() - start_time < 60.0:
        now = time.time()
        if now >= next_snap:
            elapsed = int(now - start_time)
            snap_name = f"v26_monitor_sec_{elapsed:02d}.png"
            take_screenshot(snap_name)
            snap_idx += 1
            next_snap = now + 10.0
            
            # Print feeder bot events from logcat
            events = subprocess.run("adb logcat -d -s feeder_bot:I yslither_net:I | grep -E 'feeder_bot|Spawned|Crashed|Killed' | tail -n 10", shell=True, capture_output=True, text=True).stdout
            if events.strip():
                print(f"[Elapsed {elapsed}s] Feeder events:\n{events.strip()}")
        
        # Check if process is still alive
        ps = adb_shell("pidof com.yslither.game").stdout.strip()
        if not ps:
            print("ERROR: Process com.yslither.game terminated unexpectedly!")
            break
        time.sleep(1.0)
        
    print("=== EXTENDED VERIFICATION COMPLETED ===")
    full_log = subprocess.run("adb logcat -d -s feeder_bot:I yslither_net:I", shell=True, capture_output=True, text=True).stdout
    with open("logcat_v26_verified.log", "w") as f:
        f.write(full_log)
    print("Full verification log written to logcat_v26_verified.log")

if __name__ == "__main__":
    main()

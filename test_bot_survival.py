import subprocess
import time
import sys

def adb(cmd):
    return subprocess.run(f"adb {cmd}", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

print("1. Checking current activity...")
adb("shell am start -n com.yslither.game/android.app.NativeActivity")
time.sleep(1)

print("2. Clearing logcat...")
adb("logcat -c")

print("3. Tapping JUGAR (800, 360)...")
adb("shell input tap 800 360")
time.sleep(1.5)

print("4. Tapping BOT button (1312, 273)...")
adb("shell input tap 1312 273")

# Monitor for 25 seconds
print("5. Monitoring gameplay for 25 seconds...")
for sec in range(1, 26):
    time.sleep(1)
    if sec % 3 == 0 or sec == 5 or sec == 6:
        subprocess.run(f"adb exec-out screencap -p > bot_sec_{sec:02d}.png", shell=True)
        print(f"  Sec {sec} captured")

adb("logcat -d > logcat_bot_survival.log")
print("6. Test completed.")

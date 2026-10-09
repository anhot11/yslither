import subprocess
import time

def adb(cmd):
    return subprocess.run(f"adb {cmd}", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

print("Starting activity...")
adb("shell am start -n com.yslither.game/android.app.NativeActivity")
time.sleep(1)

print("Clearing logcat...")
adb("logcat -c")

print("Tapping JUGAR (800, 360)...")
adb("shell input tap 800 360")

# Monitor without touching anything for 20 seconds
print("Monitoring completely untouched for 20 seconds...")
for sec in range(1, 21):
    time.sleep(1)
    if sec % 2 == 0:
        subprocess.run(f"adb exec-out screencap -p > idle_sec_{sec:02d}.png", shell=True)
        print(f"  Sec {sec} captured")

adb("logcat -d > logcat_idle.log")
print("Done.")

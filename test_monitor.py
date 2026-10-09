import subprocess
import time
import os

def run_adb(cmd):
    return subprocess.run(f"adb {cmd}", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

print("Clearing logcat...")
run_adb("logcat -c")

print("Tapping JUGAR at 800, 480 (or middle button)...")
# Let's check button position: in 720x1600 landscape:
# Width is 1600, Height is 720. Center is 800, 360.
run_adb("shell input tap 800 360")

for i in range(12):
    time.sleep(1)
    subprocess.run(f"adb exec-out screencap -p > frame_{i+1:02d}.png", shell=True)
    print(f"Captured frame {i+1}s")

print("Dumping logcat...")
res = run_adb("logcat -d")
with open("logcat_test.log", "w") as f:
    f.write(res.stdout)

print("Done! Total log lines:", len(res.stdout.splitlines()))

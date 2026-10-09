#!/usr/bin/env python3
"""
Master E2E Test Runner for yslither Vulkan Android Client.
Executes Tiers 1-4, tracks metrics, captures artifacts, and outputs test reports.
"""

import sys
import os
import argparse
import unittest
import time
import json
from datetime import datetime

# Add project root to sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from test.test_tier1_feature_coverage import TestTier1FeatureCoverage
from test.test_tier2_boundary_corner import TestTier2BoundaryCorner
from test.test_tier3_cross_feature import TestTier3CrossFeature
from test.test_tier4_real_world import TestTier4RealWorld
from scripts.adb_harness import AdbHarness

TIER_MAP = {
    "1": ("Tier 1: Feature Coverage (Happy Path)", TestTier1FeatureCoverage),
    "2": ("Tier 2: Boundary & Corner Cases", TestTier2BoundaryCorner),
    "3": ("Tier 3: Cross-Feature Combinations", TestTier3CrossFeature),
    "4": ("Tier 4: Real-World Scenarios", TestTier4RealWorld),
}

def parse_args():
    parser = argparse.ArgumentParser(description="yslither E2E Test Suite Master Runner")
    parser.add_argument("--tier", choices=["1", "2", "3", "4", "all"], default="all",
                        help="Specify which tier to run (default: all)")
    parser.add_argument("--device", default=None,
                        help="Target device serial (default: 192.168.7.12:33203)")
    parser.add_argument("--output-dir", default="test_results",
                        help="Directory to store logs and screenshots (default: test_results)")
    parser.add_argument("--report-json", default="test_results/test_report.json",
                        help="Path to save JSON test execution summary")
    return parser.parse_args()

def main():
    args = parse_args()
    start_time = time.time()
    start_utc = datetime.utcnow().isoformat() + "Z"

    os.makedirs(args.output_dir, exist_ok=True)
    os.environ["TEST_RESULTS_DIR"] = os.path.abspath(args.output_dir)
    if args.device:
        os.environ["ANDROID_SERIAL"] = args.device

    print("======================================================================")
    print("           YSLITHER VULKAN ANDROID E2E TEST SUITE RUNNER              ")
    print(f" Timestamp: {start_utc}")
    print(f" Target Device: {args.device or os.environ.get('ANDROID_SERIAL', '192.168.7.12:33203')}")
    print(f" Selected Tier: {args.tier.upper()}")
    print("======================================================================\n")

    harness = AdbHarness(serial=args.device)
    online, msg = harness.check_device()
    if not online:
        print(f"[FATAL ERROR] Device not online: {msg}")
        sys.exit(1)
    harness.ensure_awake_and_unlocked()

    selected_tiers = ["1", "2", "3", "4"] if args.tier == "all" else [args.tier]
    loader = unittest.TestLoader()

    overall_results = {
        "timestamp": start_utc,
        "device": harness.serial,
        "selected_tier": args.tier,
        "tiers": {},
        "total_tests": 0,
        "total_passed": 0,
        "total_failed": 0,
        "total_errors": 0,
        "total_skipped": 0,
        "duration_seconds": 0.0,
        "status": "UNKNOWN"
    }

    total_failures = 0
    total_errors = 0

    for t_key in selected_tiers:
        tier_title, test_cls = TIER_MAP[t_key]
        print(f"\n>>> Running {tier_title} ...")
        suite = loader.loadTestsFromTestCase(test_cls)
        t_start = time.time()
        runner = unittest.TextTestRunner(verbosity=2)
        res = runner.run(suite)
        t_dur = time.time() - t_start

        passed = res.testsRun - len(res.failures) - len(res.errors) - len(res.skipped)
        tier_data = {
            "title": tier_title,
            "tests_run": res.testsRun,
            "passed": passed,
            "failed": len(res.failures),
            "errors": len(res.errors),
            "skipped": len(res.skipped),
            "duration_seconds": round(t_dur, 2),
            "failure_details": [f"{t.id()}: {err}" for t, err in res.failures],
            "error_details": [f"{t.id()}: {err}" for t, err in res.errors]
        }
        overall_results["tiers"][f"tier_{t_key}"] = tier_data
        overall_results["total_tests"] += res.testsRun
        overall_results["total_passed"] += passed
        overall_results["total_failed"] += len(res.failures)
        overall_results["total_errors"] += len(res.errors)
        overall_results["total_skipped"] += len(res.skipped)
        total_failures += len(res.failures)
        total_errors += len(res.errors)

    total_duration = time.time() - start_time
    overall_results["duration_seconds"] = round(total_duration, 2)
    overall_results["status"] = "PASSED" if (total_failures == 0 and total_errors == 0) else "FAILED"

    # Save JSON report
    report_path = os.path.abspath(args.report_json)
    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    with open(report_path, "w") as f:
        json.dump(overall_results, f, indent=2)

    # Print Summary Table
    print("\n======================================================================")
    print("                        TEST EXECUTION SUMMARY                        ")
    print("======================================================================")
    print(f"{'Tier':<35} | {'Run':<5} | {'Pass':<5} | {'Fail':<5} | {'Time':<7}")
    print("-" * 65)
    for t_key, t_data in overall_results["tiers"].items():
        print(f"{t_data['title']:<35} | {t_data['tests_run']:<5} | {t_data['passed']:<5} | {t_data['failed'] + t_data['errors']:<5} | {t_data['duration_seconds']:<5}s")
    print("-" * 65)
    print(f"{'TOTAL':<35} | {overall_results['total_tests']:<5} | {overall_results['total_passed']:<5} | {overall_results['total_failed'] + overall_results['total_errors']:<5} | {overall_results['duration_seconds']:<5}s")
    print(f"Overall Status: {overall_results['status']}")
    print(f"Detailed JSON Report: {report_path}")
    print("======================================================================\n")

    sys.exit(0 if overall_results["status"] == "PASSED" else 1)

if __name__ == "__main__":
    main()

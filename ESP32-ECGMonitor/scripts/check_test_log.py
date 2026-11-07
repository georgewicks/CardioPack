#!/usr/bin/env python3
"""
Parse an ECG Monitor serial log (as saved to docs/output*.txt) and print a
pass/fail test checklist.  Mirrors docs/TEST-WALKTHROUGH.md.

Usage:
    python3 scripts/check_test_log.py [path/to/log.txt]

If no path is given, the newest docs/output*.txt is used.  Exit code is 0
only when every gate passes.
"""

import glob
import os
import re
import sys

# Core I2C devices expected on the bus every session.
EXPECTED_I2C = {0x48, 0x49, 0x57, 0x68, 0x69}
# Optional device (PCF8574 GPIO expander) - warn only if absent.
OPTIONAL_I2C = {0x20}


def find_latest_log():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    candidates = sorted(glob.glob(os.path.join(root, "docs", "output*.txt")))
    return candidates[-1] if candidates else None


def parse_i2c_scan(text):
    """Return the set of addresses present in the boot-time I2C scanner grid."""
    found = set()
    token_re = re.compile(r"^(?:--|[0-9a-fA-F]{2})$")
    for line in text.splitlines():
        m = re.match(r"\s*([0-9a-fA-F]{2}):\s+(.*)", line)
        if not m:
            continue
        toks = m.group(2).split()
        if len(toks) < 16:
            continue
        if not all(token_re.match(t) for t in toks):
            continue
        base = int(m.group(1), 16)
        for j, tok in enumerate(toks):
            if tok != "--":
                found.add(base + j)
    return found


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else find_latest_log()
    if not path or not os.path.exists(path):
        print("No log file found.")
        return 2

    with open(path, errors="replace") as f:
        text = f.read()

    rows = []  # (status, label, detail)

    def add(status, label, detail=""):
        rows.append((status, label, detail))

    # 1. I2C bus
    found = parse_i2c_scan(text)
    missing = EXPECTED_I2C - found
    if found:
        detail = "present: " + " ".join("0x%02x" % x for x in sorted(found))
    else:
        detail = "scanner grid not found in log"
    if missing:
        add("FAIL", "I2C devices",
            detail + " | missing: " + " ".join("0x%02x" % x for x in sorted(missing)))
    else:
        add("PASS", "I2C devices", detail)

    for dev in sorted(OPTIONAL_I2C - found):
        add("WARN", "I2C optional", "0x%02x not present (PCF8574)" % dev)

    # Device-init markers as a cross-check (robust to scanner-grid absence).
    for label, pat in (
        ("ADS1115 #1 (0x48)", r"ADS1115 #1 \(0x48\) found"),
        ("ADS1115 #2 (0x49)", r"ADS1115 #2 \(0x49\) found"),
        ("DS3231 (0x68)", r"DS3231 found"),
    ):
        add("PASS" if re.search(pat, text) else "FAIL", label)

    # 2. MPU6050
    if re.search(r"MPU6050 \(0x69\) found and initialized", text):
        add("PASS", "MPU6050 (0x69)", "found and initialized (motion gating enabled)")
    elif re.search(r"MPU6050 \(0x69\) not found", text):
        add("FAIL", "MPU6050 (0x69)", "not found on I2C bus")
    else:
        add("WARN", "MPU6050 (0x69)", "not mentioned in log")

    # 3. SD card
    m = re.search(r"MOUNT OK\. Card info: Name:\s*(\S+)", text)
    if m:
        add("PASS", "SD card", "mounted: " + m.group(1))
    elif re.search(r"MOUNT FAILED", text):
        add("FAIL", "SD card", "mount failed (check SD power/wiring)")
    else:
        add("WARN", "SD card", "no mount result in log")

    # 4. ALERT/RDY timeouts (excluding the intentional chip-0 polling case is
    #    not possible from the log, so any timeout is reported and flagged).
    n_timeout = len(re.findall(r"timeout waiting for ALERT/RDY", text))
    add("PASS" if n_timeout == 0 else "FAIL", "ADS1115 ALERT/RDY",
        f"{n_timeout} timeout(s)" if n_timeout else "no timeouts")

    # 5. Leads-off / railed ECG
    n_leads = len(re.findall(r"LEADS OFF", text))
    add("PASS" if n_leads == 0 else "FAIL", "ECG leads-off / railed",
        f"{n_leads} warning(s)" if n_leads else "none")

    # 6. Recording result
    m = re.search(r"RECORDING COMPLETE:\s*(\d+) records,\s*(\d+) write errors", text)
    if m:
        records, errors = int(m.group(1)), int(m.group(2))
        ok = records > 0 and errors == 0
        add("PASS" if ok else "FAIL", "Recording",
            f"{records} records, {errors} write errors")
    else:
        add("WARN", "Recording", "no RECORDING COMPLETE line (log truncated?)")

    m = re.search(r"ISHNE file closed \((\d+) samples/lead\)", text)
    if m:
        n = int(m.group(1))
        add("PASS" if n > 0 else "FAIL", "ISHNE output",
            f"{n} samples/lead" + ("" if n > 0 else " (header only)"))

    # 7. Time sync (informational)
    synced = re.search(r"System time synchronized successfully|NTP Synchronized successfully", text)
    add("PASS" if synced else "WARN", "NTP time sync",
        "synchronized" if synced else "not seen in log")

    print(f"Checklist for: {os.path.basename(path)}\n" + "-" * 60)
    n_fail = 0
    for status, label, detail in rows:
        mark = {"PASS": "[PASS]", "FAIL": "[FAIL]", "WARN": "[WARN]"}[status]
        if status == "FAIL":
            n_fail += 1
        line = f"{mark:7s} {label}"
        if detail:
            line += f" — {detail}"
        print(line)
    print("-" * 60)
    total_fail = n_fail
    print(f"Result: {len(rows)} checks, {total_fail} failed")
    return 0 if total_fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

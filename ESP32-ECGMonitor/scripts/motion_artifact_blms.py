#!/usr/bin/env python3
"""
Offline prototype: adaptive cancellation of motion artifacts from ECG using a
3-axis accelerometer reference (MPU6050).

Implements the approach of Huang, Chen & Xiong (ICBET 2019):
  "An Effective Adaptive Filter to Reduce Motion Artifacts from ECG Signals
   Using Accelerometer"  -- adaptive noise canceller with BLMS (and LMS).

Primary input  d(n)  = ECG channel (motion-corrupted)
Reference      x(n)  = [ax, ay, az] (g), lagged by M taps
Output         e(n)  = d(n) - w^T x(n)   (motion estimate subtracted)

Inputs are the files already produced by the firmware:
  - ecg_trace.csv : HH:MM:SS,us,Ch0,Ch1,Ch2,Ch3   (records at ~302 Hz)
  - motion.csv    : us,ax_raw,ay_raw,az_raw        (~100 Hz, +-2g raw)

Run:
  python3 scripts/motion_artifact_blms.py
"""

import os
import sys
import numpy as np

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
DATA_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "docs", "logging-ref", "2026-11-02")
ECG_CSV = os.path.join(DATA_DIR, "ecg_trace.csv")
MOT_CSV = os.path.join(DATA_DIR, "motion.csv")

# Wall-clock window of the recording to analyze (Nov 2 2026, ~20:44:23-20:46:23).
TIME_LO = "20:44:15"
TIME_HI = "20:46:40"

ECG_CH = 2                    # 0-based channel index of the "real" ECG (chip 1)
ACCEL_FS = 16384.0            # MPU6050 +-2g -> 16384 LSB/g

M_TAPS = 8                    # FIR taps per accelerometer axis
BLOCK_L = 16                  # BLMS block length (samples)
MU = 0.001                    # step size (shared by LMS and BLMS)

RAIL = 32760                  # samples at/above this are treated as railed
QRS_MIN_RR = 0.30             # seconds; shorter RR counted as artifact
QRS_MAX_RR = 2.00             # seconds; longer RR counted as artifact


def load_ecg(path, t_lo, t_hi):
    """Return (t_us, ch[N]) for rows whose wall-clock time is in [t_lo, t_hi]."""
    rows = []
    with open(path) as f:
        for line in f:
            p = line.strip().split(",")
            if len(p) != 6:
                continue
            if not (t_lo <= p[0] <= t_hi):
                continue
            rows.append((int(p[1]), [int(x) for x in p[2:6]]))
    a = np.array(rows, dtype=object)
    t = np.array([r[0] for r in a], dtype=np.int64)
    ch = np.array([r[1] for r in a], dtype=np.float64)
    return t, ch


def load_motion_last_session(path):
    """Return (t_us, axyz_g) for the final boot session (after last reset)."""
    mot = np.loadtxt(path, delimiter=",", dtype=np.int64)
    us = mot[:, 0]
    resets = np.where(np.diff(us) < 0)[0]
    start = (resets[-1] + 1) if len(resets) else 0
    mot = mot[start:]
    t = mot[:, 0]
    axyz = mot[:, 1:4].astype(np.float64) / ACCEL_FS
    return t, axyz


def build_reference(t_ecg, t_mot, axyz, m_taps):
    """Resample accelerometer to ECG sample times and build lagged feature matrix."""
    axyz_up = np.column_stack([np.interp(t_ecg, t_mot, axyz[:, k])
                               for k in range(3)])
    n = len(t_ecg)
    p = 3 * m_taps
    X = np.empty((n, p))
    for k in range(3):
        x = axyz_up[:, k]
        for m in range(m_taps):
            lag = np.empty(n)
            lag[:m] = x[0]
            lag[m:] = x[:n - m]
            X[:, k * m_taps + m] = lag
    return axyz_up, X


def lms_anc(d, X, mu):
    n, p = X.shape
    w = np.zeros(p)
    e = np.empty(n)
    for i in range(n):
        y = X[i] @ w
        e[i] = d[i] - y
        w += 2.0 * mu * e[i] * X[i]
    return e


def blms_anc(d, X, mu, block):
    n, p = X.shape
    w = np.zeros(p)
    e = np.empty(n)
    for s in range(0, n, block):
        end = min(s + block, n)
        Xb = X[s:end]
        db = d[s:end]
        eb = db - Xb @ w
        e[s:end] = eb
        w += (mu / (end - s)) * (Xb.T @ eb)   # averaged block gradient
    return e


def mask_rails(x):
    """Linearly interpolate across railed (+-full-scale) samples."""
    bad = np.abs(x) >= RAIL
    nbad = int(bad.sum())
    out = x.copy()
    if nbad and not bad.all():
        idx = np.arange(len(x))
        out[bad] = np.interp(idx[bad], idx[~bad], x[~bad])
    return out, nbad


def detect_rr(x, fs):
    """Pan-Tompkins-lite R-peak detector; returns (r_peaks, rr_seconds)."""
    diff = np.diff(x, prepend=x[0])
    sq = diff ** 2
    win = max(1, int(0.120 * fs))
    mw = np.convolve(sq, np.ones(win) / win, mode="same")
    thr = 0.35 * np.mean(mw)
    min_gap = int(0.25 * fs)
    peaks = []
    i = 0
    n = len(mw)
    while i < n - 1:
        if mw[i] > thr:
            j = i
            while j < n - 1 and mw[j + 1] >= mw[j]:
                j += 1
            peaks.append(j)
            i = j + min_gap
        else:
            i += 1
    peaks = np.array(peaks, dtype=int)
    rr = np.diff(peaks) / fs
    return peaks, rr


def rr_stats(rr):
    rr = rr[(rr >= QRS_MIN_RR) & (rr <= QRS_MAX_RR)]
    if len(rr) == 0:
        return 0, 0.0, 0.0
    return len(rr), float(np.mean(rr)), float(np.std(rr))


def svg_polyline(pts, x0, y0, xs, ys, color, width):
    p = " ".join(f"{x0 + xs * px:.1f},{y0 - ys * py:.1f}" for px, py in pts)
    return (f'<polyline points="{p}" fill="none" stroke="{color}" '
            f'stroke-width="{width}"/>\n')


def write_svg(path, t, raw, filt, fs):
    """Two stacked panels (full recording + 10 s zoom), raw vs filtered."""
    # down-sample for drawing
    step = max(1, len(t) // 4000)
    idx = np.arange(0, len(t), step)
    tt = t[idx]
    raw_d = raw[idx]
    flt_d = filt[idx]

    def panel(tt, a, b, lo, hi):
        m = (tt >= lo) & (tt <= hi)
        tt_ = tt[m]
        a_ = a[m] - np.mean(a[m])
        b_ = b[m] - np.mean(b[m])
        span = max(np.ptp(a_), np.ptp(b_), 1.0)
        xs = 900.0 / max(hi - lo, 1e-9)
        ys = 160.0 / span
        s = ""
        s += svg_polyline(list(zip(tt_, a_)), 60, 180, xs, ys, "#888", 1.0)
        s += svg_polyline(list(zip(tt_, b_)), 60, 400, xs, ys, "#e33", 1.0)
        return s

    lo_full = t[0]
    hi_full = t[-1]
    mid = lo_full + 0.5 * (hi_full - lo_full)
    zoom_lo = mid
    zoom_hi = mid + 10e6

    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="1020" height="640" '
           'font-family="monospace" font-size="13">']
    svg.append('<rect width="1020" height="640" fill="#fff"/>')
    svg.append('<text x="60" y="20">raw ECG (gray) vs BLMS-filtered (red) — full recording</text>')
    svg.append(panel(tt, raw_d, flt_d, lo_full, hi_full))
    svg.append('<text x="60" y="330">10 s zoom (mid-recording)</text>')
    svg.append(panel(tt, raw_d, flt_d, zoom_lo, zoom_hi))
    svg.append('</svg>\n')
    with open(path, "w") as f:
        f.writelines(svg)


def main():
    print("Loading ECG ...")
    t_ecg, ch = load_ecg(ECG_CSV, TIME_LO, TIME_HI)
    fs = 1e6 / np.median(np.diff(t_ecg))
    print(f"  {len(t_ecg)} records, ~{fs:.0f} Hz")

    print("Loading motion (last session) ...")
    t_mot, axyz = load_motion_last_session(MOT_CSV)
    lo = t_ecg[0] - 0.5e6
    hi = t_ecg[-1] + 0.5e6
    m = (t_mot >= lo) & (t_mot <= hi)
    t_mot, axyz = t_mot[m], axyz[m]
    print(f"  {len(t_mot)} motion rows in overlap")

    d = ch[:, ECG_CH]
    d, nrail = mask_rails(d)
    if nrail:
        print(f"  NOTE: {nrail} railed samples on Ch{ECG_CH} interpolated away")

    d = d - np.mean(d)
    axyz_up, X = build_reference(t_ecg, t_mot, axyz, M_TAPS)
    X = X - X.mean(axis=0)

    print("Running LMS ...")
    e_lms = lms_anc(d, X, MU)
    print("Running BLMS ...")
    e_blms = blms_anc(d, X, MU, BLOCK_L)

    _, rr_raw = detect_rr(d, fs)
    _, rr_lms = detect_rr(e_lms, fs)
    _, rr_blms = detect_rr(e_blms, fs)

    print("\n=== RMS of the motion-artifact band (high-pass residual) ===")
    hp = d - np.convolve(d, np.ones(20) / 20, mode="same")
    hp_f = e_blms - np.convolve(e_blms, np.ones(20) / 20, mode="same")
    print(f"  raw  RMS = {np.sqrt(np.mean(hp**2)):8.1f}")
    print(f"  BLMS RMS = {np.sqrt(np.mean(hp_f**2)):8.1f}")

    print("\n=== R-peak / RR consistency (raw | LMS | BLMS) ===")
    for name, rr in (("raw", rr_raw), ("lms", rr_lms), ("blms", rr_blms)):
        n, mean, std = rr_stats(rr)
        print(f"  {name:4s}: valid RR={n:5d}   mean={mean*1000:6.0f} ms   std={std*1000:5.0f} ms")

    out_csv = os.path.join(DATA_DIR, "ecg_filtered_ch2.csv")
    with open(out_csv, "w") as f:
        f.write("t_us,raw,filtered_lms,filtered_blms,ax_g,ay_g,az_g\n")
        for i in range(len(t_ecg)):
            f.write(f"{t_ecg[i]},{d[i]:.1f},{e_lms[i]:.1f},{e_blms[i]:.1f},"
                    f"{axyz_up[i,0]:.4f},{axyz_up[i,1]:.4f},{axyz_up[i,2]:.4f}\n")
    print(f"\nWrote {out_csv}")

    out_svg = os.path.join(DATA_DIR, "ecg_filtered_ch2.svg")
    write_svg(out_svg, t_ecg, d, e_blms, fs)
    print(f"Wrote {out_svg}")


if __name__ == "__main__":
    sys.exit(main())

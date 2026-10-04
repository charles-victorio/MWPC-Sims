#!/usr/bin/env python3
"""
Summarise the fixed-geometry impact-parameter scan written by signal_scan.cc.

Reads every signal_d*.csv in the current directory, extracts per-track scalars
(peak amplitude, first-arrival time, time-to-peak), and plots them vs impact
parameter d with error bars = the INTRINSIC spread at each distance (gain Polya
+ diffusion + cluster statistics, with geometry fixed).

This decomposes the flux-averaged bimodality into a smooth monotonic curve plus
a narrow per-point resolution.

You can still run plot_signal_multi.py on any individual signal_d*.csv for the
per-distance waveform/overlay/histograms.

Usage:  python plot_scan_summary.py
"""

import glob, re
import numpy as np
import matplotlib.pyplot as plt

output_dir = "../output/sweep_tracks_2750V/"

files = sorted(glob.glob(output_dir + "signal_d*.csv"))
if not files:
    raise SystemExit("no signal_d*.csv files found in the directory " + output_dir)

def dist_from_name(fn):
    return float(re.search(r"signal_d([0-9.]+)\.csv", fn).group(1))

def first_arrival(el_row, T):
    pk = el_row.max()
    if pk <= 0: return np.nan
    above = np.where(el_row > 0.01 * pk)[0]
    return T[above[0]] if above.size else np.nan

dists, peak_m, peak_s, fa_m, fa_s, tp_m, tp_s = ([] for _ in range(7))

for fn in files:
    d = dist_from_name(fn)
    data = np.genfromtxt(fn, delimiter=",", names=True)
    trk = data["trackID"].astype(int)
    ids = np.unique(trk)
    nb = np.sum(trk == ids[0])
    T = data["t_ns"][trk == ids[0]]

    peaks, fas, tps = [], [], []
    for i in ids:
        m = trk == i
        tot = np.abs(data["total_fCns"][m])
        el  = np.abs(data["electron_fCns"][m])
        pk = tot.max()
        if pk <= 0:
            continue
        peaks.append(pk)
        tps.append(T[np.argmax(tot)])
        fas.append(first_arrival(el, T))

    peaks = np.array(peaks); fas = np.array(fas); tps = np.array(tps)
    fas = fas[np.isfinite(fas)]

    dists.append(d)
    peak_m.append(peaks.mean());  peak_s.append(peaks.std())
    fa_m.append(fas.mean());      fa_s.append(fas.std())
    tp_m.append(tps.mean());      tp_s.append(tps.std())
    print(f"d={d:.4f} cm: peak={peaks.mean():.2f}+/-{peaks.std():.2f} fC/ns, "
          f"first-arrival={fas.mean():.1f}+/-{fas.std():.1f} ns, "
          f"t-peak={tps.mean():.1f}+/-{tps.std():.1f} ns  (n={len(peaks)})")

dists = np.array(dists)
order = np.argsort(dists)
def srt(x): return np.array(x)[order]

fig, (a1, a2, a3) = plt.subplots(1, 3, figsize=(15, 4.5))

a1.errorbar(srt(dists), srt(fa_m), yerr=srt(fa_s), marker="o", capsize=3,
            color="tab:blue")
a1.set_xlabel("impact parameter  [cm]"); a1.set_ylabel("first arrival time [ns]")
a1.set_title("Drift time vs distance from wire")

a2.errorbar(srt(dists), srt(tp_m), yerr=srt(tp_s), marker="o", capsize=3,
            color="tab:green")
a2.set_xlabel("impact parameter  [cm]"); a2.set_ylabel("time to peak [ns]")
a2.set_title("Time to peak vs distance")

a3.errorbar(srt(dists), srt(peak_m), yerr=srt(peak_s), marker="o", capsize=3,
            color="tab:red")
a3.set_xlabel("impact parameter  [cm]"); a3.set_ylabel("peak amplitude [fC/ns]")
a3.set_title("Peak amplitude vs distance")

fig.suptitle("Fixed-geometry scan: response vs impact parameter "
             "(error bars = intrinsic spread)")
fig.tight_layout()
fig.savefig("scan_summary.png", dpi=150)
print("\nwrote scan_summary.png")
plt.show()
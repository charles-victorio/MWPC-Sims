#!/usr/bin/env python3
"""
Plot multi-track MWPC signals from signal_multi.csv (written by signal_multi.cc).

Produces three figures:
  A) overlay of individual track waveforms (the honest "what a pulse looks like")
  B) aligned mean waveform + percentile band (the "expected response +/- spread")
  C) histograms of per-track scalars: peak amplitude, first-arrival time,
     time-to-peak, rise time (10-90%)  -> these give the TIMING RESOLUTION.

Alignment for (B) is on FIRST ELECTRON ARRIVAL, which mimics a leading-edge
discriminator trigger. Averaging RAW (unaligned) waveforms would smear the fast
electron peak into nothing, so we shift each track to a common trigger first.

Usage:  python plot_signal_multi.py [signal_multi.csv]
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

input_fname = sys.argv[1] if len(sys.argv) > 1 else "signal_multi.csv"
output_dir = sys.argv[2] + "/" if len(sys.argv) > 2 else ""

# d = np.genfromtxt(input_fname, delimiter=",", names=True)

# trk = d["trackID"].astype(int)
# ids = np.unique(trk)

# # reshape into (nTracks, nbins) matrices, in magnitude
# # (assumes every track has the same nbins, written in order -- it is)
# nbins = np.sum(trk == ids[0])
# T = d["t_ns"][trk == ids[0]]                       # common time axis
# def matrix(col):
#     M = np.empty((len(ids), nbins))
#     for k, i in enumerate(ids):
#         M[k] = np.abs(d[col][trk == i])
#     return M

# TOT = matrix("total_fCns")
# EL  = matrix("electron_fCns")
# ION = matrix("ion_fCns")

# split into chunks instead of loading entire file into memory

print("Scanning CSV in chunks...")
CHUNK = 500_000

# First, get ids and nbins cheaply
counts = {}
for chunk in pd.read_csv(input_fname, usecols=["trackID"], chunksize=CHUNK):
    for tid, n in chunk["trackID"].value_counts().items():
        counts[tid] = counts.get(tid, 0) + n
ids   = np.array(sorted(counts), dtype=int)
nbins = counts[ids[0]]
id_to_k = {tid: k for k, tid in enumerate(ids)}

# Allocate (use float32 to halve memory vs float64)
TOT = np.zeros((len(ids), nbins), dtype=np.float32)
EL  = np.zeros((len(ids), nbins), dtype=np.float32)
T   = np.empty(nbins, dtype=np.float32)
t_filled = False
cursors = {tid: 0 for tid in ids}

for chunk in pd.read_csv(
    input_fname, chunksize=CHUNK,
    dtype={"trackID": np.int32, "t_ns": np.float32,
           "total_fCns": np.float32, "electron_fCns": np.float32},
):
    for tid, grp in chunk.groupby("trackID", sort=False):
        k   = id_to_k[tid]
        s   = cursors[tid]
        n   = len(grp)
        TOT[k, s:s+n] = np.abs(grp["total_fCns"].values)
        EL [k, s:s+n] = np.abs(grp["electron_fCns"].values)
        if not t_filled and tid == ids[0]:
            T[s:s+n] = grp["t_ns"].values
            if s + n == nbins:
                t_filled = True
        cursors[tid] += n

print(f"Loaded {len(ids)} tracks × {nbins} bins. "
      f"RAM: TOT+EL = {(TOT.nbytes+EL.nbytes)/1e9:.2f} GB")


tstep = T[1] - T[0]

# ---- per-track scalar features ------------------------------------------
def first_arrival(el_row):
    pk = el_row.max()
    if pk <= 0:
        return np.nan
    above = np.where(el_row > 0.01 * pk)[0]
    return T[above[0]] if above.size else np.nan

def rise_time(tot_row):
    pk = tot_row.max()
    if pk <= 0:
        return np.nan
    ip = int(np.argmax(tot_row))
    seg = tot_row[:ip+1]
    try:
        i10 = np.where(seg > 0.10 * pk)[0][0]
        i90 = np.where(seg > 0.90 * pk)[0][0]
        return T[i90] - T[i10]
    except IndexError:
        return np.nan

peaks   = TOT.max(axis=1)
tpeaks  = T[TOT.argmax(axis=1)]
tfirst  = np.array([first_arrival(EL[k])  for k in range(len(ids))])
trise   = np.array([rise_time(TOT[k])     for k in range(len(ids))])

# keep only tracks that actually produced a pulse (skip the ne~0 clippers)
good = peaks > (0.01 * np.nanmedian(peaks))

# ===================== Figure A: overlay =================================
figA, ax = plt.subplots(figsize=(10, 5))
sel = np.where(good)[0]
rng = np.random.default_rng(0)
show = rng.choice(sel, size=min(15, sel.size), replace=False)
for k in show:
    ax.plot(T, TOT[k], color="steelblue", lw=0.8, alpha=0.25)
# bold one representative (median peak among shown)
rep = show[np.argsort(peaks[show])[len(show)//2]]
ax.plot(T, TOT[rep], color="black", lw=1.6, label="representative track")
ax.set_xlim(0, 600)
ax.set_xlabel("time  [ns]"); ax.set_ylabel("|induced current|  [fC/ns]")
ax.set_title(f"Individual track waveforms (overlay of {show.size})")
ax.legend(fontsize=8)
figA.tight_layout(); figA.savefig(output_dir + "signal_overlay.png", dpi=150)

# ===================== Figure B: aligned mean + band =====================
# shift each good track so its first-arrival sits at a common bin.
ref_bin = nbins // 5                       # park the trigger 20% into window
aligned = []
for k in sel:
    fa = tfirst[k]
    if np.isnan(fa):
        continue
    shift = ref_bin - int(round(fa / tstep))
    aligned.append(np.roll(TOT[k], shift))
aligned = np.array(aligned)

t_rel = (np.arange(nbins) - ref_bin) * tstep   # time relative to trigger
mean_wf = aligned.mean(axis=0)
lo, hi  = np.percentile(aligned, [16, 84], axis=0)

figB, (bl, blog) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
for axx in (bl, blog):
    axx.fill_between(t_rel, lo, hi, color="tab:orange", alpha=0.3,
                     label="16-84 percentile")
    axx.plot(t_rel, mean_wf, color="black", lw=1.4, label="mean waveform")
    axx.set_xlim(-50, 550)
bl.set_title("Aligned mean response (trigger = first electron arrival)")
bl.set_ylabel("|I|  [fC/ns]"); bl.legend(fontsize=8)
blog.set_yscale("log")
blog.set_ylim(max(mean_wf.max()*1e-4, 1e-6), mean_wf.max()*1.5)
blog.set_ylabel("|I|  [fC/ns] (log)"); blog.set_xlabel("time since trigger  [ns]")
figB.tight_layout(); figB.savefig(output_dir + "signal_mean_band.png", dpi=150)

# ===================== Figure C: scalar resolution histograms ============
figC, axes = plt.subplots(2, 2, figsize=(11, 8))
def hist(ax, vals, title, unit):
    v = vals[good & np.isfinite(vals)]
    ax.hist(v, bins=30, color="slateblue", edgecolor="black", alpha=0.8)
    ax.set_title(f"{title}\nmean={v.mean():.2f} {unit}, std={v.std():.2f} {unit}")
    ax.set_xlabel(f"{title} [{unit}]"); ax.set_ylabel("tracks")

hist(axes[0,0], peaks,  "peak amplitude",      "fC/ns")
hist(axes[0,1], tfirst, "first arrival time",  "ns")
hist(axes[1,0], tpeaks, "time to peak",        "ns")
hist(axes[1,1], trise,  "rise time (10-90%)",  "ns")
figC.suptitle("Per-track timing/amplitude resolution")
figC.tight_layout(); figC.savefig(output_dir + "signal_resolution.png", dpi=150)

print("wrote signal_overlay.png, signal_mean_band.png, signal_resolution.png")
print(f"tracks total={len(ids)}, with a pulse={good.sum()}, "
      f"empty (clipped/ne~0)={(~good).sum()}")
print(f"first-arrival jitter (std) : {np.nanstd(tfirst[good]):.2f} ns")
print(f"time-to-peak (mean+/-std)   : {np.nanmean(tpeaks[good]):.1f} "
      f"+/- {np.nanstd(tpeaks[good]):.1f} ns")
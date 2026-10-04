#!/usr/bin/env python3
"""
Plot the MWPC induced-current signal from signal.csv (written by signal_sim.cc)
and annotate the timing features:
  - first electron arrival
  - slowest primary electron arrival
  - time to peak
  - time to 1% of peak
  - ion-tail timescale tau (exponential fit, shown as a dashed line)

Recomputes the features here (independently of the C++) so the plot is
self-contained and you can tweak thresholds without recompiling.

Usage:
    python plot_signal.py [signal.csv]
"""

import sys
import numpy as np
import matplotlib.pyplot as plt

fname = sys.argv[1] if len(sys.argv) > 1 else "signal.csv"

# columns: t_ns, total_fCns, electron_fCns, ion_fCns
data = np.genfromtxt(fname, delimiter=",", names=True)
t   = data["t_ns"]
tot = np.abs(data["total_fCns"])      # work in magnitude (anode current is -ve)
el  = np.abs(data["electron_fCns"])
ion = np.abs(data["ion_fCns"])

# --- timing features ------------------------------------------------------
peak   = tot.max()
i_peak = int(np.argmax(tot))
t_peak = t[i_peak]

el_peak = el.max()
edge = 0.01 * el_peak                  # 1% of electron peak = "appreciable"
above = np.where(el > edge)[0]
t_first   = t[above[0]]  if above.size else np.nan   # leading edge
t_slowest = t[above[-1]] if above.size else np.nan   # trailing edge

# time to 1% of peak on the falling side, after the peak
falling = np.where((np.arange(len(t)) >= i_peak) & (tot < 0.01 * peak))[0]
t_1pc = t[falling[0]] if falling.size else np.nan

# ion-tail timescale: log-linear fit on the decaying ion component (50%->5%)
ion_peak  = ion.max()
i_ionpeak = int(np.argmax(ion))
mask = (np.arange(len(t)) >= i_ionpeak) & (ion > 0.05 * ion_peak) & (ion > 0)
tau = np.nan
fit_line = None
if mask.sum() > 2:
    slope, intercept = np.polyfit(t[mask], np.log(ion[mask]), 1)
    if slope < 0:
        tau = -1.0 / slope
        fit_line = np.exp(intercept + slope * t[mask])

# --- plot -----------------------------------------------------------------
fig, (ax_lin, ax_log) = plt.subplots(2, 1, figsize=(10, 9), sharex=True)

for ax in (ax_lin, ax_log):
    ax.plot(t, tot, color="black", lw=1.4, label="total |I|")
    ax.plot(t, el,  color="tab:blue",   lw=1.0, alpha=0.8, label="electron")
    ax.plot(t, ion, color="tab:red",    lw=1.0, alpha=0.8, label="ion")
    if fit_line is not None:
        ax.plot(t[mask], fit_line, "--", color="darkred", lw=1.2,
                label=fr"ion tail fit, $\tau$={tau:.0f} ns")

def annotate(ax, x, text, color, y_frac):
    if np.isnan(x):
        return
    ax.axvline(x, color=color, ls=":", lw=1.0, alpha=0.8)
    ymin, ymax = ax.get_ylim()
    y = ymin + y_frac * (ymax - ymin) if ax.get_yscale() == "linear" \
        else ymin * (ymax / ymin) ** y_frac
    ax.annotate(f"{text}\n{x:.0f} ns", xy=(x, y), xytext=(5, 0),
                textcoords="offset points", fontsize=8, color=color,
                rotation=90, va="bottom", ha="left")

# linear panel: best for the fast electron edge and the peak
ax_lin.set_title("MWPC induced current, ????V")
annotate(ax_lin, t_first,   "first e⁻ arrival",     "tab:blue",   0.55)
annotate(ax_lin, t_slowest, "slowest primary",      "tab:cyan",   0.40)
annotate(ax_lin, t_peak,    "peak",                 "black",      0.70)
annotate(ax_lin, t_1pc,     "1% of peak",           "tab:green",  0.25)
ax_lin.set_ylabel("|induced current|  [fC/ns]")
ax_lin.legend(loc="upper right", fontsize=8)

# log panel: best for the ion tail and the 1%-of-peak crossing
ax_log.set_yscale("log")
# clip zeros/negatives so log scale behaves
floor = max(peak * 1e-5, 1e-12)
ax_log.set_ylim(floor, peak * 1.5)
annotate(ax_log, t_first,   "first e⁻ arrival", "tab:blue",  0.85)
annotate(ax_log, t_slowest, "slowest primary",  "tab:cyan",  0.70)
annotate(ax_log, t_peak,    "peak",             "black",     0.92)
annotate(ax_log, t_1pc,     "1% of peak",       "tab:green", 0.30)
ax_log.set_ylabel("|induced current|  [fC/ns]  (log)")
ax_log.set_xlabel("time  [ns]")
ax_log.legend(loc="upper right", fontsize=8)

plt.tight_layout()
fname_root = fname.split('.')[0]
plt.savefig(f"{fname_root}.png", dpi=150)

# --- zoomed view: x-axis limited to just past the 1% cutoff -------------
xmax = (t_slowest * 1.1) if not np.isnan(t_1pc) else 600.0   # a bit past 1% point

fig2, (zlin, zlog) = plt.subplots(2, 1, figsize=(10, 9), sharex=True)
for ax in (zlin, zlog):
    ax.plot(t, tot, color="black",    lw=1.4, label="total |I|")
    ax.plot(t, el,  color="tab:blue", lw=1.0, alpha=0.8, label="electron")
    ax.plot(t, ion, color="tab:red",  lw=1.0, alpha=0.8, label="ion")
    ax.set_xlim(0, xmax)
    # ax.axvline(t_first,   color="tab:blue",  ls=":", lw=1.0)
    # ax.axvline(t_slowest, color="tab:cyan",  ls=":", lw=1.0)
    # ax.axvline(t_peak,    color="black",     ls=":", lw=1.0)
    # if not np.isnan(t_1pc):
    #     ax.axvline(t_1pc, color="tab:green", ls=":", lw=1.0)

zlin.set_title(f"MWPC induced current, ????V, zoomed (0–{xmax:.0f} ns)")
annotate(zlin, t_first,   "first e⁻ arrival",     "tab:blue",   0.55)
annotate(zlin, t_slowest, "slowest primary",      "tab:cyan",   0.40)
annotate(zlin, t_peak,    "peak",                 "black",      0.70)
annotate(zlin, t_1pc,     "1% of peak",           "tab:green",  0.25)
zlin.set_ylabel("|induced current|  [fC/ns]")
zlin.legend(loc="upper right", fontsize=8)

zlog.set_yscale("log")
zlog.set_ylim(max(peak*1e-4, 1e-6), peak*1.5)
annotate(zlog, t_first,   "first e⁻ arrival", "tab:blue",  0.85)
annotate(zlog, t_slowest, "slowest primary",  "tab:cyan",  0.70)
annotate(zlog, t_peak,    "peak",             "black",     0.92)
annotate(zlog, t_1pc,     "1% of peak",       "tab:green", 0.30)
zlog.set_ylabel("|induced current|  [fC/ns]  (log)")
zlog.set_xlabel("time  [ns]")
zlog.legend(loc="upper right", fontsize=8)

plt.tight_layout()
plt.savefig(f"{fname_root}_zoom.png", dpi=150)

# --- echo the numbers -----------------------------------------------------
print(f"peak |I|                : {peak:.4g} fC/ns at t = {t_peak:.1f} ns")
print(f"first electron arrival  : {t_first:.1f} ns")
print(f"slowest primary arrival : {t_slowest:.1f} ns")
print(f"time to peak            : {t_peak:.1f} ns")
print(f"time to 1% of peak      : {t_1pc:.1f} ns")
print(f"ion-tail timescale tau  : {tau:.1f} ns")

if tot[-1] > 0.01 * peak:
    print("WARNING: signal still > 1% of peak at end of window; "
          "rerun signal_sim with more nbins to capture the full ion tail.")
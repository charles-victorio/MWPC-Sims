import numpy as np
import matplotlib.pyplot as plt

# --- Load Geant4 output ---
charges_fC = np.loadtxt("../output/charges.txt")
e_charge_fC = 1.6e-4  # fC per electron
n_primary = np.round(charges_fC / e_charge_fC).astype(int)

# --- Polya gain distribution ---
# For Ar/CO2 75:25, theta ~ 0.4-0.5 (measured)
# theta=0 is pure exponential, theta=1 is narrower
# Mean gain G: start with 1e4, adjust to match your HV
G     = 1e5 #1e4
theta = 0.45

def sample_polya(n_electrons, mean_gain, theta, rng):
    """
    For each primary electron, sample one gain value from Polya(G, theta).
    Total charge = sum of all individual gains.
    Polya is a Gamma distribution with shape=(1+theta)/theta, scale=G*theta/(1+theta)
    ... but simpler: sample as Gamma(shape, scale) per electron.
    """
    shape = (1.0 + theta) / theta
    scale = mean_gain * theta / (1.0 + theta)
    total = np.zeros(len(n_electrons))
    for i, n in enumerate(n_electrons):
        if n <= 0:
            continue
        gains = rng.gamma(shape, scale, size=n)
        total[i] = gains.sum()
    return total

rng = np.random.default_rng(42)
signal_electrons = sample_polya(n_primary, G, theta, rng)
signal_fC = signal_electrons * e_charge_fC

# --- Extract key quantities ---
counts, bin_edges = np.histogram(signal_fC, bins=200)
bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
q_mpv = bin_centers[np.argmax(counts)]
q5    = np.percentile(signal_fC, 5)
q99   = np.percentile(signal_fC, 99)

print(f"Q_MPV = {q_mpv:.1f} fC")
print(f"Q5    = {q5:.1f} fC")
print(f"Q99   = {q99:.1f} fC")
print(f"Q_ratio = {q99/q5:.1f}x")

# --- Plot ---
plt.figure(figsize=(8, 5))
plt.hist(signal_fC, bins=200, histtype='step', color='steelblue')
plt.axvline(q5,    color='red',    linestyle='--', label=f'Q5  = {q5:.1f} fC')
plt.axvline(q_mpv, color='green',  linestyle='--', label=f'MPV = {q_mpv:.1f} fC')
plt.axvline(q99,   color='orange', linestyle='--', label=f'Q99 = {q99:.1f} fC')
plt.xlabel('Signal charge (fC)')
plt.ylabel('Events')
plt.yscale('log')
plt.legend()
plt.tight_layout()
plt.savefig("charge_spectrum.png", dpi=150)
plt.show()
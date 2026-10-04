import matplotlib.pyplot as plt
import numpy as np

# P (power %) -> cut depths (mm)
# cut depth = 2.97 - D
data = {
    10: [0.34, 0.34, 0.34],
    40: [1.56, 1.56, 1.51],
    60: [2.02, 2.08, 2.11],
    70: [2.36, 2.48, 2.42],
}

powers = sorted(data.keys())
means = [np.mean(data[p]) for p in powers]
stds  = [np.std(data[p])  for p in powers]

fig, ax = plt.subplots(figsize=(7, 5))

# individual points
for p in powers:
    ax.scatter([p] * len(data[p]), data[p], color='steelblue', alpha=0.5, zorder=3)

# mean line with error bars (±1 std)
ax.errorbar(powers, means, yerr=stds, fmt='o-', color='steelblue',
            linewidth=2, markersize=7, capsize=5, label='mean ± 1 std')

ax.set_xlabel('Power (%)')
ax.set_ylabel('Cut depth (mm)')
ax.set_title('Laser cut depth vs power (speed = 10%)')
ax.set_xticks(powers)
ax.set_ylim(0, 3.2)
ax.axhline(2.97, color='tomato', linestyle='--', linewidth=1, label='material thickness (2.97 mm)')
ax.legend()
ax.grid(True, linestyle='--', alpha=0.4)

plt.tight_layout()
plt.savefig('laser_cut_depth.png', dpi=150)
plt.show()
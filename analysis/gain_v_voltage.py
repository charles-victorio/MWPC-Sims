"""
import matplotlib.pyplot as plt
import numpy as np

# Data
V = np.array([2000, 2250, 2500, 2750, 3000])
G = np.array([633.042, 2714.35, 11619.7, 50234.9, 244508])
n = np.array([240, 276, 28678, 854, 1420])

errors = np.sqrt(n * 50)

plt.figure(figsize=(10, 6))
plt.errorbar(V, G, yerr=errors, fmt='o-', capsize=5, 
             linewidth=2, markersize=8, label='Gain', color='blue',
             elinewidth=1, capthick=1)
plt.xlabel('Voltage (V)', fontsize=12)
plt.ylabel('Mean Gain (G)', fontsize=12)
plt.title('Gain vs Voltage with Error Bars', fontsize=14, fontweight='bold')
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig("gv.png")

"""

import matplotlib.pyplot as plt
import numpy as np

# Data
V = np.array([2000, 2250, 2500, 2750, 3000])
G = np.array([633.042, 2714.35, 11619.7, 51776.1, 244508])
n = np.array([240, 276, 28678, 28930, 1420])


errors = G / np.sqrt(n)  # Error bars

# fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
fig, ax1 = plt.subplots(1, 1, figsize=(14, 5))

# Linear plot
ax1.errorbar(V, G, yerr=errors, fmt='o-', capsize=5, color='blue')
ax1.set_xlabel('Voltage (V)', fontsize=12)
ax1.set_ylabel('Mean Gain (G)', fontsize=12)
ax1.set_title('Linear Scale', fontsize=12, fontweight='bold')
ax1.grid(True, alpha=0.3)

# # Log plot
# ax2.semilogy(V, G, 'o-', linewidth=2, markersize=8, color='red')
# ax2.set_xlabel('Voltage (V)', fontsize=12)
# ax2.set_ylabel('Mean Gain (G) [log scale]', fontsize=12)
# ax2.set_title('Semi-log Scale', fontsize=12, fontweight='bold')
# ax2.grid(True, alpha=0.3, which='both')

plt.suptitle('Gain vs Voltage', fontsize=14, fontweight='bold')
plt.tight_layout()
plt.savefig("gv_lin_only.png")

import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import gaussian_kde


# Read data
# fname = 'ne_per_track_2500V.txt'
voltage = 2750
fname = f'ne{voltage}V.txt'
with open(fname, 'r') as f:
    data = np.array([int(line.strip()) * 1.6e-4 for line in f if line.strip()])

# Calculate statistics
mean_val = np.mean(data)
# q5 = np.percentile(data, 5)
q10 = np.percentile(data, 10)
for i in range(5, 11):
    print(i, np.percentile(data, i))
q99 = np.percentile(data, 99)

# find the peak of the kde
kde = gaussian_kde(data)

# evaluate on a fine grid, then argmax
xs = np.linspace(data.min(), data.max(), 10000)
ys = kde(xs)
peak = xs[np.argmax(ys)]


# Create histogram
sns.set_style('whitegrid')
plt.figure(figsize=(10, 6))

# Plot histogram
# sns.histplot(data, bins='auto', kde=True, color='blue', edgecolor='black', alpha=0.6)

# Add vertical lines
plt.axvline(mean_val, color='orange', linestyle='--', linewidth=2, label=f'Mean: {mean_val:.2f}')
plt.axvline(peak, color='red', linestyle='--', linewidth=2, label=f'MPV: {peak:.2f}')
plt.axvline(q10, color='lightgreen', linestyle='--', linewidth=2, label=f'Q10: {q10:.2f}')
plt.axvline(q99, color='purple', linestyle='--', linewidth=2, label=f'Q99: {q99:.2f}')

# Create histogram
sns.set_style('whitegrid')
sns.histplot(data, bins='auto', kde=True, color='blue', edgecolor='black')
plt.xlabel('Signal charge per track (fC)')
plt.ylabel('Events')
plt.title(f'fC per track ({voltage}V)')
plt.legend(loc='upper right', framealpha=0.9)
plt.grid(True, alpha=0.3)
plt.savefig(f'hist{voltage}V.png')
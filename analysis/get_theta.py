import numpy as np
import pandas as pd


for voltage in [2500, 2750]:
    fname = f'ne{voltage}V.txt'
    with open(fname, 'r') as f:
        data = np.array([int(line.strip()) * 1.6e-4 for line in f if line.strip()])

    ne = data[data > 0] # drop the ne=0 (lost) primaries
    mean = ne.mean()
    rel_var = ne.var() / mean**2
    theta = 1.0/rel_var - 1.0 # from (sigma/mean)^2 = 1/(1+theta)
    print(voltage, theta)

# voltage theta
# 2500 0.158
# 2750 0.239
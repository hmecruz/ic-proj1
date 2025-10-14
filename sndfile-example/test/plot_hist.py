#!/usr/bin/env python3
import sys
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("Usage: python3 plot_hist.py <histogram_file>")
    sys.exit(1)

# Read histogram file (value\tcount)
values = []
counts = []
with open(sys.argv[1], 'r') as f:
    for line in f:
        parts = line.strip().split()
        if len(parts) == 2:
            values.append(int(parts[0]))
            counts.append(int(parts[1]))

# Convert to numpy arrays
values = np.array(values)
counts = np.array(counts)

# Sort by value (for plotting)
sorted_indices = np.argsort(values)
values = values[sorted_indices]
counts = counts[sorted_indices]

# Plot
plt.figure(figsize=(10, 6))
plt.bar(values, counts, width=(values[1] - values[0]) if len(values) > 1 else 1)
plt.title("Audio Sample Histogram")
plt.xlabel("Quantized Sample Value")
plt.ylabel("Count")
plt.grid(True, linestyle='--', alpha=0.5)
plt.tight_layout()
plt.show()

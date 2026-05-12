import os
import sys
import pandas as pd
import matplotlib.pyplot as plt

if not os.path.exists("results.csv"):
    print("❌ Hiba: A 'results.csv' fájl nem található. Futtasd előbb a 'make run'-t!")
    sys.exit(1)


df = pd.read_csv("results.csv", names=["grid_size", "cpu_time", "kernel_time", "memcpy_time"])

# 1. Grid size vs runtime
plt.figure()
plt.plot(df["grid_size"], df["cpu_time"], label="CPU")
plt.plot(df["grid_size"], df["kernel_time"] + df["memcpy_time"], label="GPU")
plt.xlabel("Grid size (N x N)")
plt.ylabel("Runtime (ms)")
plt.title("Grid size vs runtime")
plt.legend()
plt.savefig("grid_vs_runtime.png")

# 2. GPU thread blocks (simulated)
plt.figure()
simulated_blocks = [1, 2, 4, 8, 16]
runtime = [df["kernel_time"][2] * (1 / b) + 10 for b in simulated_blocks]

plt.plot(simulated_blocks, runtime)
plt.xlabel("GPU Thread Block Multiplier")
plt.ylabel("Estimated Runtime (ms)")
plt.title("Blocks vs Runtime (simulated)")
plt.savefig("blocks_vs_runtime.png")

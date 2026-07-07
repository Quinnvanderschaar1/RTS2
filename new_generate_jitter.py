import pandas as pd
import matplotlib.pyplot as plt

# ======================================================
# Load data
# ======================================================

file_path = "build/timing_report.csv"
df = pd.read_csv(file_path)
df.columns = df.columns.str.strip()

# convert time (if you have timestamp column)
# adjust this if your column name differs
df = df.sort_values("latency_ns")
df["time_ms"] = (df["latency_ns"] - df["latency_ns"].min()) / 1e6

# ======================================================
# Settings
# ======================================================

window_size = 50   # adjust: 20–200 typical

stages = df["stage"].unique()

plt.figure(figsize=(12, 6))

# ======================================================
# Rolling jitter per stage
# ======================================================

for stage in stages:

    stage_df = df[df["stage"] == stage].copy()

    if len(stage_df) < window_size:
        continue

    # rolling jitter (std dev over time window)
    stage_df["jitter"] = stage_df["latency_ms"].rolling(window_size).std()

    plt.plot(
        stage_df["time_ms"],
        stage_df["jitter"],
        label=stage
    )

# ======================================================
# Plot styling
# ======================================================

plt.title("Jitter Over Time per Stage (Rolling Std Dev)")
plt.xlabel("Time (ms)")
plt.ylabel("Jitter (ms)")
plt.grid(True, alpha=0.3)
plt.legend(fontsize=8)
plt.tight_layout()

plt.show()
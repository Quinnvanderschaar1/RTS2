import pandas as pd
import matplotlib.pyplot as plt

file_path = "build/timing_report.csv"
df = pd.read_csv(file_path)

# Convert to ms for readability
df["latency_ms"] = df["latency_ns"] / 1e6

print(df.head())

# -------------------------------------------------
# 1. Average latency per stage (MOST IMPORTANT VIEW)
# -------------------------------------------------
stage_avg = df.groupby("stage")["latency_ms"].mean().sort_values()

plt.figure()
stage_avg.plot(kind="bar")
plt.title("Average Latency per Stage")
plt.ylabel("Latency (ms)")
plt.xlabel("Pipeline Stage")
plt.grid(axis="y")
plt.tight_layout()
plt.show()

# -------------------------------------------------
# 2. Worst-case latency per stage (WCET view)
# -------------------------------------------------
stage_max = df.groupby("stage")["latency_ms"].max().sort_values()

plt.figure()
stage_max.plot(kind="bar")
plt.title("Worst-Case Latency per Stage (WCET)")
plt.ylabel("Latency (ms)")
plt.xlabel("Pipeline Stage")
plt.grid(axis="y")
plt.tight_layout()
plt.show()

# -------------------------------------------------
# 3. Latency over time per stage (behavior over run)
# -------------------------------------------------
plt.figure()

for stage in df["stage"].unique():
    subset = df[df["stage"] == stage]
    plt.plot(subset["block"], subset["latency_ms"], label=stage)

plt.title("Latency Over Time per Stage")
plt.xlabel("Block index")
plt.ylabel("Latency (ms)")
plt.legend()
plt.grid()
plt.tight_layout()
plt.show()

# -------------------------------------------------
# 4. Jitter distribution per stage (variability)
# -------------------------------------------------
plt.figure()

for stage in df["stage"].unique():
    subset = df[df["stage"] == stage]
    plt.hist(subset["latency_ms"], bins=30, alpha=0.5, label=stage)

plt.title("Latency Distribution per Stage")
plt.xlabel("Latency (ms)")
plt.ylabel("Count")
plt.legend()
plt.grid()
plt.tight_layout()
plt.show()

# -------------------------------------------------
# 5. Summary table
# -------------------------------------------------
summary = df.groupby("stage")["latency_ms"].agg(["mean", "max", "min"])
print("\n=== Stage Timing Summary (ms) ===")
print(summary)
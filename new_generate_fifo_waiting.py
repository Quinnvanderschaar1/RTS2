import pandas as pd
from collections import deque
import matplotlib.pyplot as plt

# ======================================================
# Load CSV
# ======================================================

file_path = "build/fifo_occupancy.csv"   # <-- change if needed

df = pd.read_csv(file_path)
df.columns = df.columns.str.strip()

# convert to ms (optional but useful)
df["time_ms"] = (df["timestamp_ns"] - df["timestamp_ns"].min()) / 1e6

# sort by time (critical for correctness)
df = df.sort_values("timestamp_ns")

# ======================================================
# FIFO simulation: compute waiting times
# ======================================================

waiting_stats = {}

for fifo in df["fifo_name"].unique():

    fifo_df = df[df["fifo_name"] == fifo]

    queue = deque()
    waiting_times = []

    for _, row in fifo_df.iterrows():

        if row["event"] == "push":
            queue.append(row["timestamp_ns"])

        elif row["event"] == "pop":
            if queue:
                push_time = queue.popleft()
                wait_ns = row["timestamp_ns"] - push_time
                waiting_times.append(wait_ns / 1e6)  # ms

    if len(waiting_times) > 0:
        waiting_stats[fifo] = {
            "avg_wait_ms": sum(waiting_times) / len(waiting_times),
            "max_wait_ms": max(waiting_times),
            "samples": len(waiting_times)
        }
    else:
        waiting_stats[fifo] = {
            "avg_wait_ms": 0,
            "max_wait_ms": 0,
            "samples": 0
        }

# ======================================================
# Print results
# ======================================================

print("\n===== FIFO WAITING TIME ANALYSIS =====\n")

for fifo, stats in waiting_stats.items():
    print(f"{fifo:20s} | "
          f"avg={stats['avg_wait_ms']:.3f} ms | "
          f"max={stats['max_wait_ms']:.3f} ms | "
          f"samples={stats['samples']}")

# ======================================================
# Plot average waiting time
# ======================================================

labels = list(waiting_stats.keys())
values = [waiting_stats[f]["avg_wait_ms"] for f in labels]

plt.figure(figsize=(10,5))
plt.bar(labels, values)

plt.title("Average FIFO Waiting Time")
plt.ylabel("Waiting Time (ms)")
plt.xticks(rotation=20, ha="right")
plt.grid(axis="y")

plt.tight_layout()
plt.show()
import pandas as pd
import matplotlib.pyplot as plt

# ----------------------------
# Load CSV
# ----------------------------
file_path = "build/fifo_occupancy.csv"
df = pd.read_csv(file_path)

df = pd.read_csv(file_path, skipinitialspace=True)

df.columns = df.columns.str.strip()

print(df.columns)

# Convert timestamp to relative ms for readability
df["time_ms"] = (df["timestamp_ns"] - df["timestamp_ns"].min()) / 1e6

# ----------------------------
# 1. Plot FIFO size over time
# ----------------------------
plt.figure(figsize=(12, 6))

for fifo in df["fifo_name"].unique():
    fifo_data = df[df["fifo_name"] == fifo]

    plt.plot(
        fifo_data["time_ms"],
        fifo_data["current_size"],
        label=fifo
    )

plt.title("FIFO Occupancy Over Time (Bottleneck Detection)")
plt.xlabel("Time (ms)")
plt.ylabel("Queue Size (current)")
plt.legend()
plt.grid()
plt.show()

# ----------------------------
# 2. Compute bottleneck score
# ----------------------------
summary = df.groupby("fifo_name").agg(
    max_size_seen=("current_size", "max"),
    avg_size=("current_size", "mean"),
    max_capacity=("max_size", "max")
).reset_index()

summary["utilization"] = summary["avg_size"] / summary["max_capacity"]

print("\n=== FIFO Pressure Report ===")
print(summary.sort_values("utilization", ascending=False))

# ----------------------------
# 3. Push vs pop imbalance
# ----------------------------
ops = df.groupby(["fifo_name", "event"]).size().unstack(fill_value=0)
ops["imbalance"] = ops.get("push", 0) - ops.get("pop", 0)

print("\n=== Push/Pop Imbalance ===")
print(ops.sort_values("imbalance", ascending=False))

# ----------------------------
# 4. Highlight critical FIFOs
# ----------------------------
critical = summary[summary["utilization"] > 0.7]

print("\n=== Potential Bottlenecks (>70% avg utilization) ===")
print(critical)

# ----------------------------
# 5. Simple interpretation helper
# ----------------------------
print("\n=== Interpretation ===")
for _, row in summary.iterrows():
    if row["utilization"] > 0.7:
        print(f"{row['fifo_name']} → LIKELY BOTTLENECK")
    elif row["utilization"] > 0.4:
        print(f"{row['fifo_name']} → moderate load")
    else:
        print(f"{row['fifo_name']} → healthy")
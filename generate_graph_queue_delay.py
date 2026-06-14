import pandas as pd
import matplotlib.pyplot as plt

# Load CSV
file_path = "build/queue_latency.csv"
df = pd.read_csv(file_path)

# If timestamps exist in another version, we ignore for now
print(df.head())

# Convert to milliseconds for readability
df["latency_ms"] = df["latency_ns"] / 1e6

# ---------------------------
# 1. Latency over time
# ---------------------------
plt.figure()
plt.plot(df["latency_ms"])
plt.title("Queue Latency Over Time")
plt.xlabel("Sample index")
plt.ylabel("Latency (ms)")
plt.grid()
plt.show()

# ---------------------------
# 2. Histogram (jitter view)
# ---------------------------
plt.figure()
plt.hist(df["latency_ms"], bins=50)
plt.title("Queue Latency Distribution (Jitter)")
plt.xlabel("Latency (ms)")
plt.ylabel("Count")
plt.grid()
plt.show()

# ---------------------------
# 3. Basic statistics
# ---------------------------
print("\n--- Latency Stats (ms) ---")
print(df["latency_ms"].describe())

print("\nWorst case latency (ms):", df["latency_ms"].max())
print("Average latency (ms):", df["latency_ms"].mean())

# ---------------------------
# 4. Push vs Pop timeline (optional insight)
# ---------------------------
if "push_ns" in df.columns and "pop_ns" in df.columns:
    plt.figure()
    plt.plot(df["push_ns"], label="push timestamp")
    plt.plot(df["pop_ns"], label="pop timestamp")
    plt.title("Queue Timing (Push vs Pop)")
    plt.legend()
    plt.grid()
    plt.show()
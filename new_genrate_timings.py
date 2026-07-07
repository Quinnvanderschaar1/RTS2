import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
import numpy as np

# ======================================================
# Load CSV
# ======================================================

file_path = "build/timing_report.csv"     

df = pd.read_csv(file_path)
df.columns = df.columns.str.strip()

# ======================================================
# Execution stages only
# ======================================================
matplotlib.use("TkAgg")
plt.close("all")
execution_stages = [
    "recorder_hw_record",
    "lowpass_proc",
    "echocancel_proc",
    "audioencoder_proc",
    "transmit_block_assembly_total",
    "transmit_active_send_or_push",
    "receive_block_disassembly_total",
    "audiordecoder_proc",
    "player_hw_proc"
]

rename = {
    "transmit_active_send_or_push": "transmit_send",
    "transmit_block_assembly_total": "transmit_assembly",
    "receive_block_disassembly_total": "receive_disassembly",
    "audiordecoder_proc": "audiodecoder_proc"
}

exec_df = df[df["stage"].isin(execution_stages)].copy()
exec_df["stage"] = exec_df["stage"].replace(rename)

# ======================================================
# Remove statistical outliers (IQR)
# ======================================================

filtered = []
means = []
removed = []

for stage in exec_df["stage"].unique():

    data = exec_df[exec_df["stage"] == stage]["latency_ms"]

    q1 = data.quantile(0.25)
    q3 = data.quantile(0.75)
    iqr = q3 - q1

    lower = q1 - 1.5 * iqr
    upper = q3 + 1.5 * iqr

    #clean = data[(data >= lower) & (data <= upper)]
    clean = data

    filtered.append(clean)

    means.append(round(clean.mean(), 4))

    removed.append(len(data) - len(clean))

    print(f"{stage:20s} removed {len(data)-len(clean)} outlier(s)")

# ======================================================
# Plot
# ======================================================

fig, ax = plt.subplots(figsize=(13,6))

colors = [
    "#4C72B0",
    "#55A868",
    "#C44E52",
    "#8172B2",
    "#CCB974",
    "#72B28C",
    "#F8F8F8",
    "#3B383B",
    "#F8F8F8"
]

bp = ax.boxplot(
    filtered,
    labels=exec_df["stage"].unique(),
    patch_artist=True,
    showfliers=False
)

# Color each box
for patch, color in zip(bp["boxes"], colors):
    patch.set_facecolor(color)
    patch.set_alpha(0.7)

# Color whiskers/caps/median
for median in bp["medians"]:
    median.set_color("black")
    median.set_linewidth(2)

for whisker in bp["whiskers"]:
    whisker.set_linewidth(1.5)

for cap in bp["caps"]:
    cap.set_linewidth(1.5)

ax.set_title("Execution Time Distribution per Processing Stage")
ax.set_ylabel("Execution Time (ms)")
ax.grid(axis="y", alpha=0.4)

plt.xticks(rotation=15)

# ======================================================
# Average execution time panel
# ======================================================

text = "Average Execution Time\n\n"

for stage, mean, color in zip(exec_df["stage"].unique(), means, colors):
    text += f"■ {stage:<18} {mean:.4f} ms\n"

# Place text outside axes
ax.text(
    1.03,
    0.98,
    text,
    transform=ax.transAxes,
    fontsize=10,
    verticalalignment="top",
    family="monospace",
    bbox=dict(
        boxstyle="round,pad=0.5",
        facecolor="whitesmoke",
        edgecolor="gray"
    )
)
worst = []
for stage in exec_df["stage"].unique():

    data = exec_df[exec_df["stage"] == stage]["latency_ms"]

    worst.append(data.max())
# ======================================================
# Worst-case execution time panel
# ======================================================

wcet_text = "Worst-Case Execution Time\n\n"

for stage, w, color in zip(exec_df["stage"].unique(), worst, colors):
    wcet_text += f"■ {stage:<18} {w:.4f} ms\n"

# Place text outside axes (shifted further right)
ax.text(
    1.03,
    0.45,   # lower than avg panel so both fit
    wcet_text,
    transform=ax.transAxes,
    fontsize=10,
    verticalalignment="top",
    family="monospace",
    bbox=dict(
        boxstyle="round,pad=0.5",
        facecolor="whitesmoke",
        edgecolor="gray"
    )
)

deadlines = {
    "recorder_hw_record": 1.0,
    "lowpass_proc": 1.0,
    "echocancel_proc": 1.0,
    "audioencoder_proc": 1.0,
    "transmit_assembly": 1.0,
    "transmit_send": 1.0,
    "receive_disassembly": 1.0,
    "audiodecoder_proc": 1.0,
    "player_hw_proc": 1.0
}

# ======================================================
# DEADLINE MISS ANALYSIS
# ======================================================

miss_stats = {}

for stage in exec_df["stage"].unique():

    data = exec_df[exec_df["stage"] == stage]["latency_ms"]

    if stage not in deadlines:
        continue

    deadline = deadlines[stage]

    misses = (data > deadline).sum()
    total = len(data)

    miss_rate = misses / total if total > 0 else 0

    miss_stats[stage] = {
        "deadline": deadline,
        "misses": misses,
        "total": total,
        "miss_rate": miss_rate
    }

# ======================================================
# PRINT RESULTS
# ======================================================

print("\n===== DEADLINE MISS ANALYSIS =====\n")

for stage, stats in miss_stats.items():
    print(f"{stage:30s} | "
          f"DL={stats['deadline']:.1f} ms | "
          f"misses={stats['misses']}/{stats['total']} "
          f"({stats['miss_rate']*100:.1f}%)")

jitter_stats = {}
for stage in exec_df["stage"].unique():

    data = exec_df[exec_df["stage"] == stage]["latency_ms"]
    mean = data.mean()

    abs_dev = (data - mean).abs()

    jitter_stats[stage] = {
        "mean": mean,
        "max_jitter": abs_dev.max(),
        "std_jitter": data.std()
    }

print("\n===== JITTER ANALYSIS =====\n")

for stage, stats in jitter_stats.items():
    print(f"{stage:30s} | "
          f"mean={stats['mean']:.3f} ms | "
          f"std(jitter)={stats['std_jitter']:.3f} ms | ")


plt.subplots_adjust(right=0.70)

plt.show()
plt.close()
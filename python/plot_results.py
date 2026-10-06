#!/usr/bin/env python3
"""Creates graphs/ from results/*.csv.  Requires: pip install matplotlib pandas"""
from pathlib import Path
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parent.parent
(ROOT / "graphs").mkdir(exist_ok=True)
size = pd.read_csv(ROOT / "results" / "input_size_results.csv")
thr = pd.read_csv(ROOT / "results" / "thread_results.csv")


def save(name):
    plt.tight_layout(); plt.savefig(ROOT / "graphs" / name, dpi=150); plt.close()


plt.figure(figsize=(6, 4))
plt.plot(size.records, size.speedup, "o-", label="Measured speedup")
plt.axhline(1, color="grey", ls="--", label="Break-even (1x)")
plt.xscale("log"); plt.xlabel("Number of records"); plt.ylabel("Speedup"); plt.title("Speedup vs Input Size")
plt.grid(alpha=.3); plt.legend(); save("speedup_vs_input_size.png")

plt.figure(figsize=(6, 4))
plt.plot(size.records, size.seq_time_s, "o-", label="Sequential")
plt.plot(size.records, size.par_time_s, "s-", label="OpenMP")
plt.xscale("log"); plt.yscale("log"); plt.xlabel("Number of records"); plt.ylabel("Time (s)")
plt.title("Execution Time vs Input Size"); plt.grid(alpha=.3); plt.legend(); save("execution_time_vs_input_size.png")

plt.figure(figsize=(6, 4))
plt.plot(thr.threads, thr.par_time_s, "o-", label="OpenMP")
plt.axhline(thr.seq_time_s.iloc[0], color="grey", ls="--", label="Sequential")
plt.xticks(thr.threads); plt.xlabel("Number of threads"); plt.ylabel("Time (s)")
plt.title("Parallel Execution Time vs Threads"); plt.grid(alpha=.3); plt.legend(); save("execution_time_vs_threads.png")

fig, ax = plt.subplots(figsize=(6, 4))
ax.plot(thr.threads, thr.speedup, "o-", label="Speedup"); ax.plot(thr.threads, thr.threads, "--", color="grey", label="Ideal")
ax.set_xticks(thr.threads); ax.set_xlabel("Number of threads"); ax.set_ylabel("Speedup")
ax2 = ax.twinx(); ax2.plot(thr.threads, thr.efficiency_pct, "s:", color="tab:red", label="Efficiency (%)"); ax2.set_ylabel("Efficiency (%)")
ax.set_title("Speedup and Efficiency vs Threads"); ax.grid(alpha=.3)
h1, l1 = ax.get_legend_handles_labels(); h2, l2 = ax2.get_legend_handles_labels(); ax.legend(h1 + h2, l1 + l2)
save("speedup_efficiency_vs_threads.png")
print("Graphs saved in graphs/")

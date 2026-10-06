#!/usr/bin/env python3
"""Runs both experiments and writes results/input_size_results.csv and results/thread_results.csv.
Usage: python3 python/run_experiments.py [--repeats 5] [--threads 8]"""
import argparse, csv, json, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GEN, ANA = ROOT / "bin" / "traffic_generator", ROOT / "bin" / "network_traffic_analysis"
SIZES = [10_000, 100_000, 500_000, 1_000_000]
THREADS = [1, 2, 4, 8]


def analyze(n, threads, repeats):
    f = ROOT / "data" / f"traffic_{n}.csv"
    if not f.exists():
        subprocess.run([str(GEN), str(n), str(f)], check=True)
    out = subprocess.run([str(ANA), str(f), "--json", "--repeats", str(repeats), "--threads", ",".join(map(str, threads))],
                         check=True, capture_output=True, text=True)
    return json.loads(out.stdout)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repeats", type=int, default=5)
    ap.add_argument("--threads", type=int, default=8, help="thread count for the input-size experiment")
    a = ap.parse_args()
    (ROOT / "results").mkdir(exist_ok=True)

    with open(ROOT / "results" / "input_size_results.csv", "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["records", "threads", "seq_time_s", "par_time_s", "speedup", "efficiency_pct", "correct"])
        for n in SIZES:
            d = analyze(n, [a.threads], a.repeats); r = d["runs"][0]
            w.writerow([n, a.threads, d["sequential"]["mean"], r["mean"], r["speedup"], r["efficiency"], r["correct"]])
            print(f"[size] {n:>9}: seq={d['sequential']['mean']:.6f}s par={r['mean']:.6f}s speedup={r['speedup']:.2f} correct={r['correct']}")

    d = analyze(1_000_000, THREADS, a.repeats)
    with open(ROOT / "results" / "thread_results.csv", "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["threads", "seq_time_s", "par_time_s", "speedup", "efficiency_pct", "correct"])
        for r in d["runs"]:
            w.writerow([r["threads"], d["sequential"]["mean"], r["mean"], r["speedup"], r["efficiency"], r["correct"]])
            print(f"[threads] {r['threads']}: par={r['mean']:.6f}s speedup={r['speedup']:.2f} eff={r['efficiency']:.1f}% correct={r['correct']}")
    print("Saved CSVs in results/. Now run: python3 python/plot_results.py")


if __name__ == "__main__":
    main()

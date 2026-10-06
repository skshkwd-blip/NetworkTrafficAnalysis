#!/usr/bin/env python3
"""Correctness tests. Run after `make`:  python3 tests/test_analyzer.py"""
import json, subprocess, tempfile, os
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ANA = str(ROOT / "bin" / "network_traffic_analysis")


def run(csv, threads="1,2,4"):
    return json.loads(subprocess.run([ANA, str(csv), "--json", "--repeats", "2", "--threads", threads],
                                     check=True, capture_output=True, text=True).stdout)


d = run(ROOT / "data" / "sample_10.csv")
r = d["result"]
expected = dict(total_packets=10, total_bytes=20861, tcp=6, udp=3, icmp=1, anomalies=2, top_ip="192.168.1.10", top_port=443)
for k, v in expected.items():
    assert r[k] == v, f"{k}: expected {v}, got {r[k]}"
assert all(x["correct"] for x in d["runs"]), "parallel result differs from sequential"
print("PASS sample_10: all statistics match hand-computed values (5000 B is NOT an anomaly, 5001 B is)")

with tempfile.TemporaryDirectory() as t:
    f = os.path.join(t, "big.csv")
    subprocess.run([str(ROOT / "bin" / "traffic_generator"), "50000", f, "7"], check=True, capture_output=True)
    d = run(f, "1,2,3,4,8")
    assert all(x["correct"] for x in d["runs"]), "parallel != sequential on generated data"
    assert d["result"]["total_packets"] == 50000
    print("PASS generated 50,000 records: parallel == sequential for 1,2,3,4,8 threads")
print("All tests passed.")

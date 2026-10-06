# Network Traffic Analysis: Sequential vs OpenMP 

Analyses large volumes of network-traffic records and compares a **sequential C++** implementation with an **OpenMP** implementation (data parallelism, thread-local results, merged at the end).

For every CSV (`src_ip,dst_ip,protocol,dst_port,packet_size`) it computes: total packets, total bytes, TCP/UDP/ICMP counts, anomalies (packet size **> 5000**), most active source IP and most-used destination port. It then checks parallel == sequential and reports time, speedup and efficiency.

## Project layout
```
src/traffic_generator.cpp        synthetic CSV generator (inserts anomalies + a 5000 B boundary case)
src/network_traffic_analysis.cpp sequential + OpenMP analysis, correctness check, timing (text or --json)
ui/server.py, ui/index.html      local web UI (Python standard library only)
python/run_experiments.py        runs both experiments -> results/*.csv
python/plot_results.py           results/*.csv -> graphs/*.png  (needs matplotlib, pandas)
tests/test_analyzer.py           correctness tests (hand-verified sample + generated data)
data/sample_10.csv               10-record file with known answers
LLM_Usage_Log.md, REPORT_OUTLINE.md   documentation templates
```

## 1. Build
| OS | Requirement | Command |
|---|---|---|
| Linux | `g++` with OpenMP | `make` |
| macOS | `brew install libomp` (Apple Clang has no built-in OpenMP) | `make` |
| Windows | use WSL (Ubuntu) or MSYS2 MinGW-w64 | `make` |

## 2. Run the UI
```bash
python3 ui/server.py        # open http://localhost:8000
```
Pick a traffic volume, choose thread counts, press **Run analysis**. **Run scaling study** repeats 10K → 1M records. You can also upload your own CSV. Missing datasets are generated automatically.

## 3. Command line
```bash
./bin/traffic_generator 100000 data/traffic_100000.csv
./bin/network_traffic_analysis data/traffic_100000.csv --threads 1,2,4,8 --repeats 5
```

## 4. Experiments for the report
```bash
python3 tests/test_analyzer.py                   # correctness first
python3 python/run_experiments.py --repeats 5    # Experiment 1 (sizes, 8 threads) + Experiment 2 (1M records, 1/2/4/8 threads)
pip install matplotlib pandas && python3 python/plot_results.py
```
Outputs: `results/input_size_results.csv`, `results/thread_results.csv`, and four PNGs in `graphs/`.

## Design notes (viva)
- **Why it parallelises:** each record is independent, so the record loop is split across threads (`#pragma omp for schedule(static)`).
- **Race conditions:** threads never share the frequency maps; each owns a local `AnalysisResult`, merged in an `omp critical` block.
- **Deterministic output:** ties for "most active" are broken by smallest key, so sequential and parallel always agree.
- **Timing:** CSV loading is excluded; thread creation and merging are included. Each measurement is repeated and the mean is used (sd is also reported).
- **Why small inputs can be slower:** thread start-up and merging cost more than the work saved. Efficiency drops as threads grow because of overhead, memory bandwidth, the serial merge, and the number of physical cores (asking for more threads than cores cannot help).
- **Complexity:** sequential O(N + U); parallel record processing about O(N/P) plus overhead and merge, where U = unique IPs/ports.

## Limitations
Synthetic data; simple threshold anomaly rule; a few chosen statistics; timings depend on the machine. No MPI/CUDA comparison.

# Network Traffic Analysis: Sequential vs OpenMP

A C++ tool that analyses network-traffic records and compares a **sequential** implementation with a **parallel OpenMP** implementation. It checks that both give identical results and measures execution time, speedup and parallel efficiency. A small web UI is included for running experiments.

**Live demo:** https://networktrafficanalysis.onrender.com

> The demo runs on a free hosting tier. It may take 30–60 seconds to wake up on the first visit, and it has very limited CPU, so parallel speedup measured there is not representative. Use a local machine for performance measurements.

## Features

- Reads traffic records from CSV (`src_ip,dst_ip,protocol,dst_port,packet_size`)
- Computes total packets, total bytes, TCP/UDP/ICMP counts, anomalies, most active source IP and most-used destination port
- Anomaly rule: a packet is anomalous if its size is **greater than 5000 bytes**
- Verifies that parallel output equals sequential output
- Reports mean time, standard deviation, speedup and efficiency for different thread counts
- Web UI, experiment scripts and graph generation

## Dataset

All data in this project is **synthetic**. It is produced by `src/traffic_generator.cpp` and is not real network traffic. The generator inserts random records, a number of anomalous large packets and a boundary case at exactly 5000 bytes.

`data/sample_10.csv` is a small hand-written synthetic file with known answers, used for testing. Large generated files (`data/traffic_*.csv`) are not stored in the repository; create them with the generator.

## Project structure

```
src/
  traffic_generator.cpp          synthetic CSV generator
  network_traffic_analysis.cpp   sequential + OpenMP analysis, correctness check, timing
ui/
  server.py                      web server (Python standard library only)
  index.html                     web interface
python/
  run_experiments.py             runs the experiments and writes results/*.csv
  plot_results.py                turns results/*.csv into graphs/*.png
tests/
  test_analyzer.py               correctness tests
data/sample_10.csv               synthetic sample with known answers
Dockerfile                       container build used for deployment
Makefile
```

## Requirements

| OS | Requirement |
|---|---|
| Linux | `g++` with OpenMP, Python 3 |
| macOS | `brew install libomp`, Python 3 |
| Windows | WSL (Ubuntu) or MSYS2 MinGW-w64 |

For graphs only: `pip install matplotlib pandas`

## Build

```bash
mkdir -p bin
make
```

## Usage

### Web UI

```bash
python3 ui/server.py
```

Open http://localhost:8000, choose the number of records and thread counts, and press **Run analysis**. **Run scaling study** repeats the analysis for 10K to 1M records. You can also upload your own CSV file with the same columns.

### Command line

```bash
./bin/traffic_generator 100000 data/traffic_100000.csv
./bin/network_traffic_analysis data/traffic_100000.csv --threads 1,2,4,8 --repeats 5
```

### Tests

```bash
python3 tests/test_analyzer.py
```

### Experiments and graphs

```bash
python3 python/run_experiments.py --repeats 5
python3 python/plot_results.py
```

- Experiment 1: input sizes 10K, 100K, 500K and 1M records at a fixed thread count
- Experiment 2: 1M records with 1, 2, 4 and 8 threads

Results are written to `results/input_size_results.csv` and `results/thread_results.csv`; graphs are saved in `graphs/`.

## How it works

**Sequential:** one pass over all records, updating the counters and frequency maps. Complexity is O(N + U), where N is the number of records and U the number of unique IPs and ports.

**OpenMP:** the record loop is split across threads with `#pragma omp for schedule(static)`. Each thread keeps its own local result (counters and frequency maps), so threads never write to shared data while processing. The local results are merged inside an `omp critical` block. This avoids race conditions without locking every record.

**Deterministic results:** ties for "most active" are broken by the smallest key, so sequential and parallel runs always agree.

**Timing:** CSV loading is excluded. Thread creation and merging are included. Each measurement is repeated and the mean is reported along with the standard deviation.

## Deployment

The app is deployed on Render as a Docker web service. The `Dockerfile` installs `g++` and `make`, builds the C++ programs and starts `ui/server.py`. Pushing to the `main` branch redeploys it automatically.

To run the same container locally:

```bash
docker build -t traffic-analysis .
docker run -p 8000:8000 traffic-analysis
```

## Performance notes

- Small inputs can run slower in parallel because thread start-up and merging cost more than the work saved.
- Efficiency falls as threads increase because of overhead, memory bandwidth, the serial merge, and the number of physical cores.
- Timings depend on the machine, so results from different computers should not be compared directly.

## Limitations

- Synthetic data only
- Simple threshold-based anomaly rule
- A limited set of statistics
- OpenMP only; no MPI or CUDA comparison
- The hosted demo has limited CPU, so it is not suitable for benchmarking
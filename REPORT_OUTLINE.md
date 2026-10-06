# Report outline (fill with your own measured numbers)
1. Title page, team, roll numbers
2. Abstract
3. Introduction and problem statement (Q14)
4. Objectives
5. System design: architecture diagram (generator → CSV → reader → sequential / OpenMP → correctness → performance → graphs); data structures (TrafficRecord, vector, AnalysisResult, unordered_maps, thread-local results)
6. Sequential algorithm: pseudocode, complexity O(N+U)
7. Parallel algorithm: OpenMP, data parallelism, race-condition avoidance, merge, complexity
8. Correctness verification (tests/test_analyzer.py output)
9. Experiments: A) 10K/100K/500K/1M at fixed threads; B) 1M records at 1/2/4/8 threads; repeats and mean/sd; machine specs (CPU, cores, OS, compiler)
10. Results: tables from results/*.csv, graphs from graphs/
11. Analysis and bottlenecks (overhead, serial merge, memory access, cores)
12. Limitations and future scope
13. LLM usage log and critical evaluation
14. Individual contributions (must match what each member really did)

// network_traffic_analysis.cpp - sequential vs OpenMP network-traffic analysis (Q14).
// Usage: network_traffic_analysis <traffic.csv> [--threads 1,2,4,8] [--repeats 5] [--top 5] [--json]
#include <omp.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;

const int ANOMALY_THRESHOLD = 5000;   // anomaly if packet_size > 5000

struct TrafficRecord { string src_ip, dst_ip, protocol; int dst_port; int packet_size; };

struct AnalysisResult {
    long long totalPackets = 0, totalBytes = 0, tcp = 0, udp = 0, icmp = 0, anomalies = 0;
    unordered_map<string, long long> ipCount;
    unordered_map<int, long long> portCount;
    string topIP; int topPort = -1;

    void add(const TrafficRecord& r) {
        totalPackets++; totalBytes += r.packet_size;
        if (r.protocol == "TCP") tcp++; else if (r.protocol == "UDP") udp++; else if (r.protocol == "ICMP") icmp++;
        if (r.packet_size > ANOMALY_THRESHOLD) anomalies++;
        ipCount[r.src_ip]++; portCount[r.dst_port]++;
    }
    void merge(const AnalysisResult& o) {
        totalPackets += o.totalPackets; totalBytes += o.totalBytes;
        tcp += o.tcp; udp += o.udp; icmp += o.icmp; anomalies += o.anomalies;
        for (auto& kv : o.ipCount) ipCount[kv.first] += kv.second;
        for (auto& kv : o.portCount) portCount[kv.first] += kv.second;
    }
    // Ties are broken by smallest key so sequential and parallel always agree.
    void finalize() {
        long long best = -1;
        for (auto& kv : ipCount) if (kv.second > best || (kv.second == best && kv.first < topIP)) { best = kv.second; topIP = kv.first; }
        best = -1;
        for (auto& kv : portCount) if (kv.second > best || (kv.second == best && kv.first < topPort)) { best = kv.second; topPort = kv.first; }
    }
    bool equals(const AnalysisResult& o) const {
        return totalPackets == o.totalPackets && totalBytes == o.totalBytes && tcp == o.tcp && udp == o.udp &&
               icmp == o.icmp && anomalies == o.anomalies && topIP == o.topIP && topPort == o.topPort &&
               ipCount == o.ipCount && portCount == o.portCount;
    }
};

static vector<TrafficRecord> readCSV(const string& path, long long& bad) {
    ifstream in(path); vector<TrafficRecord> v; string line; bad = 0;
    if (!in) return v;
    getline(in, line);                                   // header
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        size_t p[4], pos = 0; bool ok = true;
        for (int i = 0; i < 4; i++) { p[i] = line.find(',', pos); if (p[i] == string::npos) { ok = false; break; } pos = p[i] + 1; }
        if (!ok) { bad++; continue; }
        TrafficRecord r;
        r.src_ip = line.substr(0, p[0]); r.dst_ip = line.substr(p[0] + 1, p[1] - p[0] - 1);
        r.protocol = line.substr(p[1] + 1, p[2] - p[1] - 1);
        char *e1, *e2;
        string ps = line.substr(p[2] + 1, p[3] - p[2] - 1), ss = line.substr(p[3] + 1);
        r.dst_port = (int)strtol(ps.c_str(), &e1, 10); r.packet_size = (int)strtol(ss.c_str(), &e2, 10);
        if (e1 == ps.c_str() || e2 == ss.c_str()) { bad++; continue; }
        v.push_back(move(r));
    }
    return v;
}

static AnalysisResult runSequential(const vector<TrafficRecord>& recs) {
    AnalysisResult res;
    for (const auto& r : recs) res.add(r);
    res.finalize();
    return res;
}

// Data parallelism: each thread fills its own thread-local result (no shared-map races),
// then the partial results are merged inside a critical section.
static AnalysisResult runParallel(const vector<TrafficRecord>& recs, int threads) {
    AnalysisResult global; long long n = (long long)recs.size();
    #pragma omp parallel num_threads(threads)
    {
        AnalysisResult local;
        #pragma omp for schedule(static) nowait
        for (long long i = 0; i < n; i++) local.add(recs[i]);
        #pragma omp critical
        global.merge(local);
    }
    global.finalize();
    return global;
}

struct Stat { double mean, mn, sd; };
static Stat stats(const vector<double>& t) {
    double s = 0, mn = t[0]; for (double x : t) { s += x; mn = min(mn, x); }
    double m = s / t.size(), v = 0; for (double x : t) v += (x - m) * (x - m);
    return {m, mn, sqrt(v / t.size())};
}
static string esc(const string& s) { string o; for (char c : s) { if (c == '"' || c == '\\') o += '\\'; if ((unsigned char)c >= 32) o += c; } return o; }

template <class F> static double timeIt(F f, AnalysisResult& out) {
    auto t0 = Clock::now(); out = f(); return chrono::duration<double>(Clock::now() - t0).count();
}

int main(int argc, char** argv) {
    if (argc < 2) { cerr << "Usage: " << argv[0] << " <traffic.csv> [--threads 1,2,4,8] [--repeats N] [--top K] [--json]\n"; return 1; }
    string path = argv[1]; vector<int> threadList; int repeats = 3, topK = 5; bool json = false;
    for (int i = 2; i < argc; i++) {
        string a = argv[i];
        if (a == "--json") json = true;
        else if (a == "--repeats" && i + 1 < argc) repeats = max(1, atoi(argv[++i]));
        else if (a == "--top" && i + 1 < argc) topK = max(1, atoi(argv[++i]));
        else if (a == "--threads" && i + 1 < argc) { stringstream ss(argv[++i]); string t; while (getline(ss, t, ',')) if (atoi(t.c_str()) > 0) threadList.push_back(atoi(t.c_str())); }
    }
    if (threadList.empty()) threadList.push_back(omp_get_max_threads());

    long long bad; auto t0 = Clock::now();
    vector<TrafficRecord> recs = readCSV(path, bad);
    double loadTime = chrono::duration<double>(Clock::now() - t0).count();
    if (recs.empty()) { cerr << "No valid records read from " << path << "\n"; return 1; }

    // Sequential baseline (timed `repeats` times; first run's result is the reference).
    AnalysisResult seq, tmp; vector<double> st;
    for (int r = 0; r < repeats; r++) st.push_back(timeIt([&] { return runSequential(recs); }, r == 0 ? seq : tmp));
    Stat S = stats(st);

    struct Run { int threads; Stat t; bool ok; double speedup, eff; };
    vector<Run> runs;
    for (int th : threadList) {
        AnalysisResult par; vector<double> pt; bool ok = true;
        for (int r = 0; r < repeats; r++) { double t = timeIt([&] { return runParallel(recs, th); }, par); pt.push_back(t); ok = ok && par.equals(seq); }
        Stat P = stats(pt); double sp = S.mean / P.mean;
        runs.push_back({th, P, ok, sp, sp / th * 100.0});
    }

    vector<pair<string, long long>> ips(seq.ipCount.begin(), seq.ipCount.end());
    vector<pair<int, long long>> prts(seq.portCount.begin(), seq.portCount.end());
    sort(ips.begin(), ips.end(), [](auto& a, auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
    sort(prts.begin(), prts.end(), [](auto& a, auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });

    if (json) {
        printf("{\"file\":\"%s\",\"records\":%lld,\"skipped_lines\":%lld,\"load_time\":%.6f,\"repeats\":%d,\"max_threads\":%d,", esc(path).c_str(), seq.totalPackets, bad, loadTime, repeats, omp_get_max_threads());
        printf("\"result\":{\"total_packets\":%lld,\"total_bytes\":%lld,\"tcp\":%lld,\"udp\":%lld,\"icmp\":%lld,\"anomalies\":%lld,\"top_ip\":\"%s\",\"top_port\":%d,\"unique_ips\":%zu,\"unique_ports\":%zu,",
               seq.totalPackets, seq.totalBytes, seq.tcp, seq.udp, seq.icmp, seq.anomalies, esc(seq.topIP).c_str(), seq.topPort, seq.ipCount.size(), seq.portCount.size());
        printf("\"top_ips\":["); for (int i = 0; i < topK && i < (int)ips.size(); i++) printf("%s{\"ip\":\"%s\",\"count\":%lld}", i ? "," : "", esc(ips[i].first).c_str(), ips[i].second);
        printf("],\"top_ports\":["); for (int i = 0; i < topK && i < (int)prts.size(); i++) printf("%s{\"port\":%d,\"count\":%lld}", i ? "," : "", prts[i].first, prts[i].second);
        printf("]},\"sequential\":{\"mean\":%.6f,\"min\":%.6f,\"sd\":%.6f},\"runs\":[", S.mean, S.mn, S.sd);
        for (size_t i = 0; i < runs.size(); i++)
            printf("%s{\"threads\":%d,\"mean\":%.6f,\"min\":%.6f,\"sd\":%.6f,\"speedup\":%.4f,\"efficiency\":%.2f,\"correct\":%s}", i ? "," : "", runs[i].threads, runs[i].t.mean, runs[i].t.mn, runs[i].t.sd, runs[i].speedup, runs[i].eff, runs[i].ok ? "true" : "false");
        printf("]}\n");
        return 0;
    }

    printf("===== Sequential Analysis =====\nTotal packets: %lld\nTotal bytes: %lld\nTCP: %lld  UDP: %lld  ICMP: %lld\nLarge-packet anomalies (> %d B): %lld\nMost active source IP: %s\nMost-used destination port: %d\nMean time (%d runs): %.6f s\n\n",
           seq.totalPackets, seq.totalBytes, seq.tcp, seq.udp, seq.icmp, ANOMALY_THRESHOLD, seq.anomalies, seq.topIP.c_str(), seq.topPort, repeats, S.mean);
    for (auto& r : runs)
        printf("===== OpenMP, %d thread(s) =====\nMean time: %.6f s (min %.6f, sd %.6f)\nCorrectness check: %s\nSpeedup: %.2fx\nParallel efficiency: %.2f%%\n\n", r.threads, r.t.mean, r.t.mn, r.t.sd, r.ok ? "PASSED" : "FAILED", r.speedup, r.eff);
    return 0;
}

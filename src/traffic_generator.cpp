// traffic_generator.cpp - creates synthetic network-traffic CSV files.
// Usage: traffic_generator <num_records> <output.csv> [seed]
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
using namespace std;

int main(int argc, char** argv) {
    if (argc < 3) { cerr << "Usage: " << argv[0] << " <num_records> <output.csv> [seed]\n"; return 1; }
    long long n = atoll(argv[1]);
    unsigned long long seed = argc > 3 ? strtoull(argv[3], nullptr, 10) : 42;
    ofstream out(argv[2]);
    if (!out || n <= 0) { cerr << "Cannot write output or invalid record count\n"; return 1; }

    mt19937_64 rng(seed);
    uniform_real_distribution<double> U(0.0, 1.0);
    const int ports[] = {80, 443, 53, 22, 8080, 3306, 25, 123};
    const double portCum[] = {0.25, 0.60, 0.75, 0.82, 0.88, 0.92, 0.96, 1.0};

    out << "src_ip,dst_ip,protocol,dst_port,packet_size\n";
    for (long long i = 0; i < n; i++) {
        int s = 1 + (int)(253 * pow(U(rng), 2.0));          // skewed: low host numbers talk the most
        int d3 = (int)(U(rng) * 10), d4 = 1 + (int)(U(rng) * 254);
        double p = U(rng);
        const char* proto = p < 0.70 ? "TCP" : (p < 0.95 ? "UDP" : "ICMP");
        int port = 0;
        if (proto[0] != 'I') { double q = U(rng); int k = 0; while (q > portCum[k]) k++; port = ports[k]; }
        double a = U(rng); int size;
        if (a < 0.02)       size = 5001 + (int)(U(rng) * 4000);   // anomaly: > 5000
        else if (a < 0.021) size = 5000;                          // boundary: NOT an anomaly
        else                size = 40 + (int)(U(rng) * 1460);     // normal: 40..1499
        out << "192.168.1." << s << ",10.0." << d3 << "." << d4 << "," << proto << "," << port << "," << size << "\n";
    }
    cout << "Generated " << n << " records -> " << argv[2] << "\n";
    return 0;
}

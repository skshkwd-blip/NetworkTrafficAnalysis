# Build: make   |  Clean: make clean
# Linux: g++ -fopenmp.  macOS: Apple Clang needs Homebrew libomp (brew install libomp).
UNAME := $(shell uname -s)
CXXFLAGS = -O2 -std=c++17
ifeq ($(UNAME),Darwin)
  CXX = clang++
  LIBOMP := $(shell brew --prefix libomp 2>/dev/null)
  OMPFLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP)/include -L$(LIBOMP)/lib -lomp
else
  CXX = g++
  OMPFLAGS = -fopenmp
endif

all: bin/traffic_generator bin/network_traffic_analysis

bin/traffic_generator: src/traffic_generator.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

bin/network_traffic_analysis: src/network_traffic_analysis.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(OMPFLAGS)

clean:
	rm -f bin/traffic_generator bin/network_traffic_analysis

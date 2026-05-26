CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I./include
TARGET_ARCH = -march=native

.PHONY: all clean lib tests benchmarks run_tests run_benchmarks

all: lib tests benchmarks

lib:
	$(MAKE) -C src

tests:
	$(MAKE) -C tests

benchmarks:
	$(MAKE) -C benchmarks

run_tests: tests
	./build/bin/tests/test_runner

run_benchmarks: benchmarks
	./build/bin/benchmarks/benchmark_runner

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(MAKE) -C benchmarks clean
	rm -rf build/obj/*/*.o
	rm -rf build/lib/libparallel.a
	rm -rf build/bin/tests/test_*
	rm -rf build/bin/benchmarks/benchmark_*
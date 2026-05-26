.PHONY: all clean lib tests benchmarks run_tests run_benchmarks

all:
	$(MAKE) -C parallel-algorithms all

clean:
	$(MAKE) -C parallel-algorithms clean

lib:
	$(MAKE) -C parallel-algorithms lib

tests:
	$(MAKE) -C parallel-algorithms tests

benchmarks:
	$(MAKE) -C parallel-algorithms benchmarks

run_tests:
	$(MAKE) -C parallel-algorithms run_tests

run_benchmarks:
	$(MAKE) -C parallel-algorithms run_benchmarks

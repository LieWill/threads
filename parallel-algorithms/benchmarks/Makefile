CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I../include
BUILD_DIR = ../build
OBJ_DIR = $(BUILD_DIR)/obj/benchmarks
BIN_DIR = $(BUILD_DIR)/bin/benchmarks
LIB_DIR = ../build/lib

BENCHMARK_SOURCES = benchmark_main.cpp benchmark_for_each.cpp \
                    benchmark_accumulate.cpp benchmark_quick_sort.cpp benchmark_thread_count.cpp \
                    benchmark_bounded_buffer.cpp
BENCHMARK_OBJECTS = $(addprefix $(OBJ_DIR)/,$(BENCHMARK_SOURCES:.cpp=.o))
BENCHMARK_TARGET = benchmark_runner

LIBS = -L$(LIB_DIR) -lparallel -lpthread

.PHONY: all clean

all: $(BIN_DIR)/$(BENCHMARK_TARGET)

$(BIN_DIR)/$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@
	@echo "Benchmarks built: $@"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(BENCHMARK_OBJECTS)
	rm -f $(BIN_DIR)/$(BENCHMARK_TARGET)

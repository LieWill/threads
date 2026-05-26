CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I../include
BUILD_DIR = ../build
OBJ_DIR = $(BUILD_DIR)/obj/tests
BIN_DIR = $(BUILD_DIR)/bin/tests
LIB_DIR = $(BUILD_DIR)/lib

TEST_SOURCES = test_main.cpp test_thread_pool.cpp test_for_each.cpp \
               test_accumulate.cpp test_quick_sort.cpp test_exception_safety.cpp \
               test_bounded_buffer.cpp
TEST_OBJECTS = $(addprefix $(OBJ_DIR)/,$(TEST_SOURCES:.cpp=.o))
TEST_TARGET = test_runner

LIBS = -L$(LIB_DIR) -lparallel -lpthread

.PHONY: all clean

all: $(BIN_DIR)/$(TEST_TARGET)

$(BIN_DIR)/$(TEST_TARGET): $(TEST_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@
	@echo "Tests built: $@"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TEST_OBJECTS)
	rm -f $(BIN_DIR)/$(TEST_TARGET)

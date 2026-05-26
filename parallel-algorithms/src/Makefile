CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I../include
BUILD_DIR = ../build
OBJ_DIR = $(BUILD_DIR)/obj/src
LIB_DIR = $(BUILD_DIR)/lib

SOURCES = thread_pool.cpp
OBJECTS = $(addprefix $(OBJ_DIR)/,$(SOURCES:.cpp=.o))

.PHONY: all clean

all: $(LIB_DIR)/libparallel.a

$(LIB_DIR)/libparallel.a: $(OBJECTS)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^
	@echo "Library created: $@"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(LIB_DIR)/libparallel.a
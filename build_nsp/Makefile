# Compiler and flags
CXX := g++
CXXFLAGS := -I../src/include -fno-stack-protector
LDFLAGS := -T ../linker.ld
LDLIBS := ../dependency/rpc.a

# Source directories
SRC_DIR := ../src
EXAMPLES_DIR := ../examples

# Source files
CPP_SOURCES := $(wildcard $(SRC_DIR)/lib/*.cpp) \
               $(wildcard $(SRC_DIR)/lib/threadlib/*.cpp) \
               $(wildcard $(SRC_DIR)/lib/util/*.cpp) \
               $(wildcard $(EXAMPLES_DIR)/test1.cpp)

ASM_SOURCES := $(wildcard $(SRC_DIR)/lib/*.s)

# Object files
OBJS := $(patsubst %.cpp,%.o,$(CPP_SOURCES)) \
        $(patsubst %.s,%.o,$(ASM_SOURCES))

# Output executable (defaults to a.out if not specified)
TARGET := a.out

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.s
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

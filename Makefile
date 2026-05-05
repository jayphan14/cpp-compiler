# cs241cc — a basic WLP4 -> MIPS compiler
# Pipeline: scanner | parser | typecheck | codegen
#
# Each stage is a separate stdin/stdout filter, exactly as in CS241.

CXX      := g++
CXXFLAGS := -g -O0 -fPIC -std=c++20 -Wall -Wextra -pedantic-errors \
            -Wno-unused-parameter -Wno-dollar-in-identifier-extension

SRC_DIR   := src
BUILD_DIR := build

STAGES := scanner parser typecheck codegen
BINS   := $(addprefix $(BUILD_DIR)/,$(STAGES))

.PHONY: all clean test

all: $(BINS)

$(BUILD_DIR)/%: $(SRC_DIR)/%.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

# Run a sample end-to-end. Usage: make test PROG=examples/simple.wlp4
PROG ?= examples/simple.wlp4
test: all
	./scripts/wlp4c $(PROG)

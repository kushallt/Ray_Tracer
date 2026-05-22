CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
LDFLAGS  :=

# Directories
SRC_DIR   := src
BUILD_DIR := build
OUT_DIR   := output

TARGET := $(OUT_DIR)/main

# Auto-discover all .cpp files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# ── Default target ────────────────────────────────────────────────
.PHONY: all clean run dirs

all: dirs $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built: $@"

# Compile each .cpp → build/*.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories if missing
dirs:
	@mkdir -p $(BUILD_DIR) $(OUT_DIR)

# ── Run (pipes PPM output to file) ───────────────────────────────
run: all
	$(TARGET) > $(OUT_DIR)/image.ppm
	@echo "Rendered: $(OUT_DIR)/image.ppm"

# ── Clean ─────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)
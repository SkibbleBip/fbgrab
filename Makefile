# Compiler and flags
CC ?= gcc
CFLAGS := -O3 -Wall -Wextra -pedantic

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Executable name
TARGET := $(BUILD_DIR)/fbgrab

# Include directories
INC := -I$(INC_DIR)

# Build rule
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Compile rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Phony target to clean the build directory
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Ensure the build directory exists
$(shell mkdir -p $(BUILD_DIR))

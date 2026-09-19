# Compiler
CXX = g++
CC = gcc
# gnu11, not c11: -std=c11 sets __STRICT_ANSI__, which hides the POSIX
# declarations (fork, getppid, pid_t) these demo programs are built to show.
CFLAGS = -std=gnu11 -Wall -Wextra -O2
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -D_XOPEN_SOURCE_EXTENDED
# ncursesw, not ncurses: the UI prints multibyte characters (the arrow keys in
# the help bar), which the 8-bit library renders one broken cell per byte.
LDFLAGS = -lncursesw -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/kernel-monitor

# Standalone teaching programs; see demo/README.md
DEMO_TARGETS = $(BIN_DIR)/zombie $(BIN_DIR)/orphan $(BIN_DIR)/busy

# Source files
SOURCES = $(SRC_DIR)/main.cpp \
          $(SRC_DIR)/utils/parser.cpp \
          $(SRC_DIR)/proc/proc_reader.cpp \
          $(SRC_DIR)/system/system_monitor.cpp \
          $(SRC_DIR)/process/process_monitor.cpp \
          $(SRC_DIR)/signals/process_control.cpp \
          $(SRC_DIR)/ui/ui.cpp

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Default target
all: directories $(TARGET) demo

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)/utils $(BUILD_DIR)/proc $(BUILD_DIR)/system \
	          $(BUILD_DIR)/process $(BUILD_DIR)/signals $(BUILD_DIR)/ui $(BIN_DIR)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Demo programs. Plain C, no ncurses, each a single translation unit.
demo: directories $(DEMO_TARGETS)

$(BIN_DIR)/%: demo/%.c
	$(CC) $(CFLAGS) $< -o $@
	@echo "Built demo: $@"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Run the application
run: all
	$(TARGET)

# Install (copy to /usr/local/bin - requires sudo)
install: all
	@echo "Installing kernel-monitor to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Removing kernel-monitor from /usr/local/bin..."
	sudo rm -f /usr/local/bin/kernel-monitor
	@echo "Uninstallation complete"

# Debug build
debug: CXXFLAGS += -g -DDEBUG -O0
debug: clean all

# Help
help:
	@echo "Kernel Monitor - Makefile targets:"
	@echo "  make          - Build the monitor and the demo programs"
	@echo "  make demo     - Build only demo/zombie and demo/orphan"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run the application"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make install  - Install to /usr/local/bin (requires sudo)"
	@echo "  make uninstall- Remove from /usr/local/bin (requires sudo)"
	@echo "  make help     - Show this help message"

.PHONY: all clean run install uninstall debug help directories demo

# Compiler
CC = gcc
# gnu11, not c11: -std=c11 sets __STRICT_ANSI__, which hides the POSIX
# declarations (fork, getppid, pid_t) this project is built on.
CFLAGS = -std=gnu11 -Wall -Wextra -O2
# The monitor additionally needs the GNU/POSIX extensions (sysinfo, DT_DIR,
# clock_gettime) and the wide-character ncurses API.
MON_CFLAGS = $(CFLAGS) -D_GNU_SOURCE -D_XOPEN_SOURCE_EXTENDED
# ncursesw, not ncurses: the UI prints multibyte characters (the arrow keys in
# the help bar), which the 8-bit library renders one broken cell per byte.
LDFLAGS = -lncursesw

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/kernel-monitor

# Standalone teaching programs; see demo/README.md
DEMO_TARGETS = $(BIN_DIR)/zombie $(BIN_DIR)/orphan $(BIN_DIR)/busy

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/utils/parser.c \
          $(SRC_DIR)/proc/proc_reader.c \
          $(SRC_DIR)/system/system_monitor.c \
          $(SRC_DIR)/process/process_monitor.c \
          $(SRC_DIR)/signals/process_control.c \
          $(SRC_DIR)/ui/ui.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: directories $(TARGET) demo

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)/utils $(BUILD_DIR)/proc $(BUILD_DIR)/system \
	          $(BUILD_DIR)/process $(BUILD_DIR)/signals $(BUILD_DIR)/ui $(BIN_DIR)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(MON_CFLAGS) -c $< -o $@

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
# kernelmoni is a short alias for the same binary, so the tool can be launched
# by name from any directory without remembering the build path.
install: all
	@echo "Installing kernel-monitor to /usr/local/bin..."
	@# install(1) writes to a temporary name and renames over the target, so a
	@# copy that is currently executing does not cause "Text file busy". A plain
	@# cp fails whenever the monitor is already running somewhere.
	sudo install -m 755 $(TARGET) /usr/local/bin/kernel-monitor
	sudo ln -sf /usr/local/bin/kernel-monitor /usr/local/bin/kernelmoni
	sudo install -m 755 $(BIN_DIR)/zombie $(BIN_DIR)/orphan $(BIN_DIR)/busy /usr/local/bin/
	@echo "Installed. Run with:  kernelmoni   (or kernel-monitor)"

# Uninstall
uninstall:
	@echo "Removing kernel-monitor from /usr/local/bin..."
	sudo rm -f /usr/local/bin/kernel-monitor /usr/local/bin/kernelmoni
	sudo rm -f /usr/local/bin/zombie /usr/local/bin/orphan /usr/local/bin/busy
	@echo "Uninstallation complete"

# Debug build
debug: MON_CFLAGS += -g -DDEBUG -O0
debug: clean all

# Help
help:
	@echo "Kernel Monitor - Makefile targets:"
	@echo "  make          - Build the monitor and the demo programs"
	@echo "  make demo     - Build only demo/zombie and demo/orphan"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run the application"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make install  - Install as 'kernelmoni' in /usr/local/bin (needs sudo)"
	@echo "  make uninstall- Remove from /usr/local/bin (requires sudo)"
	@echo "  make help     - Show this help message"

.PHONY: all clean run install uninstall debug help directories demo

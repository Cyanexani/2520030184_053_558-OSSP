# Kernel Monitor

A **user-space Linux systems programming application** that provides real-time monitoring of Linux system and process states. This project demonstrates Operating Systems and Systems Programming concepts using Linux/POSIX APIs and the `/proc` filesystem.

## Overview

Kernel Monitor is a terminal-based application that displays live system information, process details, and allows basic process control operations. It is built entirely in user space using standard Linux interfaces.

---

## 🚀 Quick Installation (3 Steps)

**⭐ For clear, simple installation instructions, see [INSTALL_SIMPLE.md](INSTALL_SIMPLE.md)**

```bash
# Step 1: Clone
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP

# Step 2: Build (automatic dependency installation)
./builder.sh

# Step 3: Run
./builder.sh --run
```

**That's it!** Press **Q** to quit.

---

## Features

### System Monitoring
- **CPU Usage**: Real-time CPU utilization calculated from `/proc/stat`
- **Memory Statistics**: RAM and swap usage from `/proc/meminfo`
- **System Information**: Kernel version, uptime, load averages
- **Per-core CPU statistics**: Individual CPU core monitoring

### Process Monitoring
- **Live Process List**: View all running processes with PID, name, state, CPU%, memory%, threads
- **Process Details**: In-depth inspection of individual processes
- **Process Tree**: Visual hierarchy showing parent-child relationships
- **Process States**: Running, Sleeping, Stopped, Zombie, etc.
- **File Descriptor Count**: Number of open file descriptors per process

### Process Control
- **Signal Operations**: Send SIGTERM, SIGKILL, SIGSTOP, SIGCONT
- **Safety Features**: Confirmation dialogs for destructive operations
- **Permission Handling**: Graceful handling of permission-denied scenarios

### Event Tracking
- **Process Events**: Detect process creation and termination
- **Event Log**: Timestamped event history

### User Interface
- **Terminal-based TUI**: Built with ncurses
- **Multiple Views**: Process list, details, tree, and events
- **Interactive Navigation**: Keyboard-driven interface
- **Color-coded Display**: Visual indicators for resource usage

## Technical Implementation

### Linux/POSIX APIs Used

1. **`/proc` Filesystem**
   - `/proc/stat` - CPU statistics
   - `/proc/meminfo` - Memory information
   - `/proc/uptime` - System uptime
   - `/proc/loadavg` - Load averages
   - `/proc/version` - Kernel version
   - `/proc/[pid]/stat` - Process statistics
   - `/proc/[pid]/status` - Process status
   - `/proc/[pid]/statm` - Process memory
   - `/proc/[pid]/cmdline` - Command line
   - `/proc/[pid]/exe` - Executable path
   - `/proc/[pid]/fd/` - File descriptors

2. **System Calls**
   - `kill()` - Send signals to processes
   - `readlink()` - Read symbolic links
   - `sysconf()` - System configuration
   - `sysinfo()` - System information
   - `sigaction()` - Signal handling

3. **POSIX APIs**
   - Directory operations: `opendir()`, `readdir()`, `closedir()`
   - File I/O: `open()`, `read()`, `close()`
   - Time functions: `time()`, `localtime()`

4. **ncurses Library**
   - Terminal UI management
   - Window creation and manipulation
   - Color support
   - Keyboard input handling

### Architecture

```
kernel-monitor/
├── src/
│   ├── main.cpp                 # Application entry point
│   ├── ui/                      # Terminal user interface
│   │   ├── ui.cpp
│   │   └── ui.hpp
│   ├── proc/                    # /proc filesystem reader
│   │   ├── proc_reader.cpp
│   │   └── proc_reader.hpp
│   ├── process/                 # Process monitoring logic
│   │   ├── process_monitor.cpp
│   │   └── process_monitor.hpp
│   ├── system/                  # System monitoring logic
│   │   ├── system_monitor.cpp
│   │   └── system_monitor.hpp
│   ├── signals/                 # Process control (signals)
│   │   ├── process_control.cpp
│   │   └── process_control.hpp
│   └── utils/                   # Parsing utilities
│       ├── parser.cpp
│       └── parser.hpp
├── tests/                       # Test cases
├── docs/                        # Documentation
├── Makefile                     # Build system
└── README.md                    # This file
```

### Data Flow

1. **Monitoring Layer** (`proc/`, `system/`, `process/`)
   - Reads raw data from `/proc` filesystem
   - Parses kernel-provided information
   - Calculates derived metrics (CPU%, memory%)
   - Detects process state changes

2. **Control Layer** (`signals/`)
   - Sends signals to processes using `kill()` system call
   - Validates operations and handles errors

3. **Presentation Layer** (`ui/`)
   - Displays information using ncurses
   - Handles user input
   - Manages multiple view modes

## Requirements

### System Requirements
- **OS**: Linux (tested on Ubuntu 20.04+)
- **Kernel**: 3.x or higher
- **Architecture**: x86_64 or ARM64

### Build Requirements
- **Compiler**: g++ with C++17 support
- **Libraries**: 
  - libncurses-dev (ncurses development headers)
  - pthread (POSIX threads)

### Installation of Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ ncurses-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel ncurses
```

## Building

### Compile the project:
```bash
make
```

### Build with debug symbols:
```bash
make debug
```

### Clean build artifacts:
```bash
make clean
```

## Running

### Run directly after building:
```bash
./bin/kernel-monitor
```

### Or use make:
```bash
make run
```

### Install system-wide (optional):
```bash
sudo make install
```

Then run from anywhere:
```bash
kernel-monitor
```

## Usage

### Keyboard Controls

**Process List View:**
- `↑/↓` - Navigate process list
- `Page Up/Down` - Fast scroll
- `Enter` - View process details
- `T` - Show process tree
- `E` - Show events log
- `K` - Send SIGTERM (with confirmation)
- `S` - Send SIGSTOP (with confirmation)
- `C` - Send SIGCONT
- `R` - Force refresh
- `Q` - Quit

**Process Detail View:**
- `ESC` or `B` - Back to process list
- `Q` - Quit

**Process Tree View:**
- `↑/↓` - Scroll
- `B` - Back to process list
- `Q` - Quit

**Events View:**
- `B` - Back to process list
- `Q` - Quit

### Understanding the Display

**System Information Bar:**
```
CPU: 34.2%     RAM: 61.5%     SWAP: 4.3%     LOAD: 1.32
Kernel: 6.x.x-generic     Uptime: 04:21:17     CPUs: 8
```

**Process List:**
```
PID     PROCESS              CPU%   MEM%    STATE     THR
1241    firefox              21.4   8.2     Sleeping  24
2310    code                  9.7   6.1     Sleeping  18
```

**State Abbreviations:**
- `Running` - Process is currently executing
- `Sleeping` - Interruptible sleep (waiting for event)
- `Disk Sleep` - Uninterruptible sleep (usually I/O)
- `Zombie` - Terminated but not reaped by parent
- `Stopped` - Stopped by signal (e.g., SIGSTOP)
- `Tracing Stop` - Stopped by debugger

## How It Works

### CPU Usage Calculation

The monitor reads CPU time counters from `/proc/stat`:
```
cpu  user nice system idle iowait irq softirq
```

CPU usage percentage is calculated by:
1. Reading counter values at time T1
2. Reading counter values at time T2
3. Computing: `CPU% = 100 * (total_diff - idle_diff) / total_diff`

This demonstrates understanding of CPU time accounting in Linux.

### Memory Statistics

Memory information is read from `/proc/meminfo` which provides:
- `MemTotal` - Total usable RAM
- `MemFree` - Free memory
- `MemAvailable` - Available memory for new applications
- `Buffers` - Temporary storage for raw disk blocks
- `Cached` - Page cache
- `SwapTotal`, `SwapFree` - Swap space

Used memory is calculated as: `Used = Total - Free - Buffers - Cached`

### Process Information

For each process, the monitor reads:

1. **`/proc/[pid]/stat`** - Process statistics
   - PID, PPID, state, utime, stime, priority, nice, threads

2. **`/proc/[pid]/status`** - Human-readable status
   - UID, memory details

3. **`/proc/[pid]/statm`** - Memory usage
   - Virtual memory size, resident set size

4. **`/proc/[pid]/cmdline`** - Command line arguments

5. **`/proc/[pid]/exe`** - Symbolic link to executable

### Process Control

Signals are sent using the POSIX `kill()` system call:
```c
int kill(pid_t pid, int sig);
```

The application demonstrates:
- **SIGTERM (15)**: Graceful termination (catchable)
- **SIGKILL (9)**: Forced termination (uncatchable)
- **SIGSTOP (19)**: Suspend process (uncatchable)
- **SIGCONT (18)**: Resume stopped process

### Error Handling

The application properly handles:
- **EACCES/EPERM**: Permission denied (can't read /proc or send signal)
- **ESRCH**: Process doesn't exist (race condition)
- **EINVAL**: Invalid signal number
- **Process disappearance**: Processes terminating during scan
- **Malformed /proc data**: Unexpected file contents
- **Terminal resize**: Dynamic UI adjustment

## Testing

### Manual Test Cases

1. **System Monitoring**
   - Start monitor, verify CPU/memory display
   - Create CPU load: `stress --cpu 4 --timeout 30`
   - Create memory load: `stress --vm 2 --vm-bytes 512M --timeout 30`

2. **Process Monitoring**
   - Start a process: `sleep 1000 &`
   - Verify it appears in the list
   - Kill it: `kill $!`
   - Verify it disappears

3. **Process Control**
   - Select a safe process (e.g., your own `sleep`)
   - Send SIGSTOP (S key) - verify state changes to 'T'
   - Send SIGCONT (C key) - verify state returns to 'S'
   - Send SIGTERM (K key) - verify process terminates

4. **Process Tree**
   - Press 'T' to view tree
   - Verify parent-child relationships (e.g., shell → child processes)

5. **Events**
   - Press 'E' to view events
   - Start/stop processes in another terminal
   - Verify events appear

6. **Stability**
   - Run monitor for 5+ minutes
   - Create/destroy processes rapidly
   - Resize terminal window
   - Verify no crashes or memory leaks

### Automated Testing

Run the test script:
```bash
cd tests
./run_tests.sh
```

## Performance Analysis

### Resource Overhead

Measured on a system with ~200 processes:

| Metric | Value |
|--------|-------|
| CPU Usage | 0.5-2% |
| Memory (RSS) | ~3-5 MB |
| Refresh Rate | 1 second |
| `/proc` reads per refresh | ~200-600 files |

### Refresh Latency

- Process list scan: 10-50ms (depends on process count)
- System stats update: 1-5ms
- UI rendering: 5-10ms
- **Total refresh time**: 20-70ms

The monitor uses efficient polling with proper sleep intervals to avoid excessive CPU consumption.

## Limitations

1. **User-space only**: Cannot access internal kernel structures
2. **Permission restrictions**: Some process information requires root
3. **Polling-based**: Uses periodic sampling, not real-time kernel events
4. **Process race conditions**: Processes may disappear during scanning
5. **No historical graphs**: Shows current state only
6. **Terminal-based**: No graphical interface

## OS Concepts Demonstrated

1. **Process Management**
   - Process states and lifecycle
   - PID/PPID relationships
   - Process hierarchy

2. **CPU Scheduling**
   - CPU time accounting (user/system time)
   - Priority and nice values
   - Load averages

3. **Memory Management**
   - Virtual memory
   - Resident set size
   - Page cache and buffers
   - Swap space

4. **Signals**
   - Signal types and purposes
   - Signal delivery mechanism
   - Process termination vs. suspension

5. **File Systems**
   - `/proc` pseudo-filesystem
   - File descriptors
   - Symbolic links

6. **System Calls**
   - Process information retrieval
   - Signal sending
   - Error handling

## Educational Value

This project demonstrates:
- How Linux exposes kernel information to user space
- How monitoring tools like `top`, `htop`, and `ps` work internally
- Practical application of OS concepts (processes, memory, signals)
- Systems programming best practices
- Error handling in systems software
- Building interactive terminal applications

## Future Enhancements

Possible extensions (not implemented):
- Network statistics monitoring
- Disk I/O statistics
- Historical graphs with data logging
- Process filtering and search
- Custom signal sending
- CPU affinity display
- cgroups information
- Container awareness

## Troubleshooting

**Problem**: Compilation fails with "ncurses.h not found"
- **Solution**: Install ncurses development package

**Problem**: Permission denied reading some `/proc` entries
- **Solution**: Normal behavior for other users' processes. Run as root only if needed.

**Problem**: UI appears garbled
- **Solution**: Ensure terminal supports ANSI colors and is properly sized (minimum 80x24)

**Problem**: High CPU usage
- **Solution**: Check refresh interval in source code. Default is 1 second.

## References

- Linux Programmer's Manual: `man proc`, `man kill`, `man signal`
- ncurses documentation: `man ncurses`
- Linux Kernel Documentation: `/proc` filesystem
- "The Linux Programming Interface" by Michael Kerrisk

## Authors

Kernel Monitor - Operating Systems Project
Systems Programming Demonstration

## License

Educational project for Operating Systems course.

## Acknowledgments

- Linux kernel developers for the `/proc` interface
- ncurses library maintainers
- Operating Systems course instructors

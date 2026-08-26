# Kernel Monitor - Quick Start Guide

## What is Kernel Monitor?

Kernel Monitor is a **user-space Linux systems programming application** that provides real-time monitoring of system resources and processes. Built entirely with Linux/POSIX APIs and the `/proc` filesystem, it demonstrates core Operating Systems concepts including process management, CPU scheduling, memory management, and signal handling.

## Features at a Glance

✓ **Real-time CPU and memory monitoring**  
✓ **Live process list with sorting**  
✓ **Detailed process inspection**  
✓ **Process tree visualization**  
✓ **Process control via signals (SIGTERM, SIGKILL, SIGSTOP, SIGCONT)**  
✓ **Event tracking (process creation/termination)**  
✓ **Interactive terminal UI with ncurses**  

## Prerequisites

### System Requirements
- **Operating System**: Linux (Ubuntu 20.04+, Fedora, Arch, etc.)
- **Kernel**: 3.x or later
- **Terminal**: Any terminal with ncurses support

### Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install gcc-c++ ncurses-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel ncurses
```

## Building the Project

```bash
# Navigate to project directory
cd kernel-monitor

# Build the project
make

# The executable will be created at: bin/kernel-monitor
```

### Build Options

```bash
make           # Standard build
make debug     # Build with debug symbols
make clean     # Remove build artifacts
make run       # Build and run immediately
make install   # Install to /usr/local/bin (requires sudo)
```

## Running the Monitor

```bash
# From project directory
./bin/kernel-monitor

# Or after installation
kernel-monitor
```

## Using the Monitor

### Main Screen

When you start the monitor, you'll see:
```
┌──────────────────── KERNEL MONITOR ────────────────────┐
│ CPU: 34%     RAM: 61%     SWAP: 4%     LOAD: 1.32      │
│ Kernel: 6.x.x      Uptime: 04:21:17                    │
├────────────────────────────────────────────────────────┤
│ PID     PROCESS       CPU%     MEM%     STATE    THR    │
│ 1241    firefox       21.4     8.2      S        24     │
│ 2310    code           9.7     6.1      S        18     │
│ 1023    systemd       0.2     0.4      S         1     │
└────────────────────────────────────────────────────────┘
```

### Keyboard Controls

#### Process List View
- **↑/↓ Arrow Keys** - Navigate through process list
- **Page Up/Down** - Fast scroll through processes
- **Enter** - View detailed information about selected process
- **T** - Switch to process tree view
- **E** - View event log
- **R** - Force refresh display
- **Q** - Quit application

#### Process Control
- **K** - Send SIGTERM to selected process (with confirmation)
- **S** - Send SIGSTOP to selected process (pause)
- **C** - Send SIGCONT to selected process (resume)

#### Other Views
- **ESC or B** - Go back to process list
- **Q** - Quit from any view

### Understanding the Display

**System Information Bar:**
- **CPU**: Overall CPU utilization percentage
- **RAM**: Memory usage percentage
- **SWAP**: Swap space usage percentage
- **LOAD**: 1-minute load average
- **Kernel**: Linux kernel version
- **Uptime**: System uptime in HH:MM:SS format

**Process List Columns:**
- **PID**: Process ID
- **PROCESS**: Process name
- **CPU%**: CPU usage percentage
- **MEM%**: Memory usage percentage
- **STATE**: Process state (see below)
- **THR**: Number of threads

**Process States:**
- **Running** - Currently executing on CPU
- **Sleeping** - Waiting for an event (interruptible)
- **Disk Sleep** - Waiting for I/O (uninterruptible)
- **Zombie** - Terminated but not reaped by parent
- **Stopped** - Paused by signal (SIGSTOP)
- **Tracing Stop** - Stopped by debugger

## Example Usage Scenarios

### Monitoring CPU Usage

1. Start the monitor
2. Observe CPU percentage in top bar
3. In another terminal, create load:
   ```bash
   stress --cpu 4 --timeout 30
   ```
4. Watch CPU usage increase in real-time

### Finding Resource-Heavy Processes

1. Start the monitor
2. Processes are automatically sorted by CPU usage
3. Top processes are the most CPU-intensive
4. Press **Enter** on any process for detailed info

### Managing Processes

1. Start a test process:
   ```bash
   sleep 1000 &
   ```
2. Find it in the monitor (search for "sleep")
3. Select it with arrow keys
4. Press **S** to pause it (SIGSTOP)
5. Press **C** to resume it (SIGCONT)
6. Press **K** to terminate it (SIGTERM)

### Viewing Process Hierarchy

1. Press **T** to switch to tree view
2. See parent-child relationships
3. Find init/systemd at the root
4. Trace process ancestry

### Tracking Process Events

1. Press **E** to view events log
2. In another terminal:
   ```bash
   sleep 5 &
   ```
3. Watch "Process created" event appear
4. Wait 5 seconds for "Process terminated" event

## Testing the Installation

```bash
# Run automated tests
cd tests
chmod +x run_tests.sh
./run_tests.sh

# Tests verify:
# - /proc filesystem accessibility
# - ncurses library installation
# - Build process
# - Signal capabilities
# - System statistics format
```

## Troubleshooting

### "ncurses.h not found" during compilation
**Solution:** Install ncurses development package
```bash
sudo apt-get install libncurses-dev  # Ubuntu/Debian
```

### "Permission denied" for some processes
**Solution:** Normal behavior. You can only fully inspect processes you own. Run as root to see all processes (not recommended for daily use).

### Terminal appears garbled
**Solution:** 
- Ensure terminal is at least 80x24 characters
- Try resizing the terminal window
- Check TERM environment variable is set
- Press Ctrl+C to exit and reset terminal: `reset`

### High CPU usage from monitor itself
**Solution:** This is unusual. Check:
- System has many processes (>1000)
- Reduce refresh rate in source code if needed
- Report as potential bug

### Cannot send signals to processes
**Solution:**
- You can only send signals to your own processes
- Use sudo to control other users' processes (carefully!)
- Some processes require special permissions

## Understanding the Implementation

### How CPU Usage is Calculated

The monitor reads `/proc/stat` twice (1 second apart):
```
Time 1: cpu  100 0 50 850 ...
Time 2: cpu  105 0 55 840 ...
```

CPU usage = 100 × (active_time_diff / total_time_diff)

### How Process Info is Gathered

For each process, the monitor reads:
- `/proc/[pid]/stat` - Basic process statistics
- `/proc/[pid]/status` - Human-readable status
- `/proc/[pid]/statm` - Memory usage
- `/proc/[pid]/cmdline` - Command line
- `/proc/[pid]/fd/` - Open file descriptors

### How Signals Work

When you press **K** to terminate a process:
1. UI shows confirmation dialog
2. User confirms action
3. Application calls `kill(pid, SIGTERM)`
4. Kernel delivers signal to target process
5. Process handles signal (terminate gracefully or ignore)

## Project Structure

```
kernel-monitor/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── ui/                   # User interface (ncurses)
│   ├── proc/                 # /proc filesystem reader
│   ├── process/              # Process monitoring
│   ├── system/               # System monitoring
│   ├── signals/              # Signal handling
│   └── utils/                # Parsing utilities
├── tests/                    # Test suite
├── docs/                     # Documentation
├── Makefile                  # Build system
└── README.md                 # Full documentation
```

## Learning Resources

### To Learn More About:

**The /proc filesystem:**
- `man proc`
- `/usr/src/linux/Documentation/filesystems/proc.txt`

**Signals:**
- `man signal`
- `man kill`

**System calls:**
- `man 2 kill`
- `man 2 sysconf`
- `man 3 sysinfo`

**ncurses:**
- `man ncurses`
- Online tutorial: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/

## Getting Help

### Check the Documentation
- `README.md` - Complete project overview
- `docs/PROJECT_DOCUMENTATION.md` - Technical details
- `docs/PRESENTATION.md` - Presentation outline
- `tests/TEST_CASES.md` - Testing guide

### Common Questions

**Q: Can I modify the kernel with this?**  
A: No, this is user-space only. It reads kernel information but doesn't modify it.

**Q: Why can't I see all process information?**  
A: Linux security prevents users from accessing other users' process details.

**Q: Is this safe to run?**  
A: Yes, the monitor only reads information and can only control your own processes.

**Q: Can I use this in production?**  
A: It's an educational tool, but it's stable enough for personal use. Use established tools like `htop` for production monitoring.

## Next Steps

1. **Explore the code** - Read through the implementation
2. **Run experiments** - Create processes, monitor behavior
3. **Extend the project** - Add new features
4. **Study OS concepts** - Understand the underlying mechanisms

## Contact & Contribution

This is an educational project for Operating Systems coursework. 

**For Students:**
- Study the code to understand OS concepts
- Modify and extend for your own learning
- Use as reference for similar projects

**For Instructors:**
- Use as teaching example
- Assign as lab project
- Extend with additional requirements

---

## Quick Command Reference

```bash
# Build
make

# Run
./bin/kernel-monitor

# Test
cd tests && ./run_tests.sh

# Install
sudo make install

# Clean
make clean

# Debug build
make debug
```

## Exit the Monitor

Press **Q** at any time to quit cleanly.

---

**Enjoy exploring Linux system programming!**

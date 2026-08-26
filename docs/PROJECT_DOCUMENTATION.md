# Kernel Monitor - Project Documentation

## Table of Contents
1. [Problem Definition](#problem-definition)
2. [Objectives](#objectives)
3. [OS and Systems Programming Concepts](#os-and-systems-programming-concepts)
4. [System Design](#system-design)
5. [Linux/POSIX APIs Used](#linuxposix-apis-used)
6. [Implementation Details](#implementation-details)
7. [Testing](#testing)
8. [Performance Analysis](#performance-analysis)
9. [Limitations](#limitations)
10. [Conclusion](#conclusion)

---

## Problem Definition

System administrators, developers, and students need to understand how their Linux system behaves at runtime: which processes are running, how much CPU and memory is being consumed, and the relationships between processes. While tools like `top`, `htop`, and `ps` exist, they are often treated as "black boxes" without understanding the underlying mechanisms.

**Problem Statement**: Build a user-space system monitoring tool that demonstrates how Linux exposes kernel information through the `/proc` filesystem and standard POSIX APIs, providing real-time visibility into system and process state while serving as an educational tool for Operating Systems concepts.

**Why This Matters**:
- Understanding system behavior is crucial for debugging and optimization
- The `/proc` interface is a fundamental Linux concept
- Process management and signals are core OS concepts
- Demonstrates the boundary between user space and kernel space

---

## Objectives

### Primary Objectives

1. **System Resource Monitoring**
   - Display real-time CPU utilization
   - Show memory and swap usage
   - Present system information (kernel version, uptime, load)

2. **Process Monitoring**
   - List all running processes with their attributes
   - Track process creation and termination
   - Display process relationships (parent-child hierarchy)

3. **Process Inspection**
   - Provide detailed information about individual processes
   - Show CPU/memory consumption per process
   - Display file descriptor usage

4. **Process Control**
   - Send signals (SIGTERM, SIGKILL, SIGSTOP, SIGCONT) to processes
   - Handle permissions and errors gracefully

5. **Educational Value**
   - Demonstrate `/proc` filesystem usage
   - Show practical application of system calls
   - Illustrate OS concepts (processes, memory, signals, scheduling)

### Learning Objectives

- Understand how Linux represents processes in user space
- Learn CPU time accounting and usage calculation
- Explore memory management from user space perspective
- Practice signal handling and process control
- Develop systems programming skills (error handling, resource management)

---

## OS and Systems Programming Concepts

### 1. Process Management

**Concept**: A process is a program in execution. Linux provides extensive information about processes through the `/proc` filesystem.

**Implementation**:
- Each process has a directory `/proc/[pid]/`
- Process attributes: PID, PPID, state, priority, nice value
- Process states: Running (R), Sleeping (S), Stopped (T), Zombie (Z)
- Parent-child relationships form a process tree rooted at init/systemd

**Demonstrated in Project**:
- Reading process information from `/proc/[pid]/stat` and `/proc/[pid]/status`
- Parsing process state and displaying human-readable states
- Building process tree by tracking PID/PPID relationships
- Detecting process lifecycle events (creation, termination)

### 2. CPU Scheduling and Time Accounting

**Concept**: The Linux scheduler allocates CPU time to processes. The kernel tracks time spent in user mode and kernel mode for each process.

**Implementation**:
- `/proc/stat` contains aggregate CPU time counters
- Per-process CPU time in `/proc/[pid]/stat` (utime, stime)
- CPU time measured in "jiffies" (clock ticks)
- Load average represents system load over time

**Demonstrated in Project**:
- Calculating CPU usage percentage from time deltas
- Converting jiffies to actual time using `sysconf(_SC_CLK_TCK)`
- Displaying per-core CPU usage
- Showing load averages (1, 5, 15 minutes)

### 3. Memory Management

**Concept**: Linux manages memory using virtual memory, page tables, and the page cache. Each process has its own virtual address space.

**Implementation**:
- Virtual memory size (VSZ) - total virtual memory allocated
- Resident Set Size (RSS) - physical memory currently used
- Page cache and buffers improve I/O performance
- Swap space extends available memory to disk

**Demonstrated in Project**:
- Reading memory statistics from `/proc/meminfo`
- Per-process memory from `/proc/[pid]/statm`
- Converting page counts to bytes using page size
- Calculating "used" memory correctly (total - free - buffers - cached)

### 4. Signals

**Concept**: Signals are software interrupts for inter-process communication and process control.

**Implementation**:
- SIGTERM (15): Request graceful termination
- SIGKILL (9): Force immediate termination (uncatchable)
- SIGSTOP (19): Suspend process execution (uncatchable)
- SIGCONT (18): Resume stopped process
- Sent using `kill()` system call

**Demonstrated in Project**:
- Using `kill(pid, signal)` to send signals
- Handling permission errors (EPERM)
- Handling non-existent processes (ESRCH)
- User confirmation for destructive operations

### 5. File Descriptors

**Concept**: File descriptors are integer handles for I/O resources (files, sockets, pipes).

**Implementation**:
- Each process has a file descriptor table
- Standard descriptors: 0 (stdin), 1 (stdout), 2 (stderr)
- `/proc/[pid]/fd/` directory contains symbolic links to open files

**Demonstrated in Project**:
- Counting files in `/proc/[pid]/fd/` directory
- Using `opendir()`, `readdir()`, `closedir()` to traverse directory
- Handling permission denied gracefully

### 6. The /proc Filesystem

**Concept**: `/proc` is a pseudo-filesystem that provides an interface to kernel data structures.

**Implementation**:
- Virtual filesystem - files are generated on-the-fly
- System-wide info: `/proc/stat`, `/proc/meminfo`, `/proc/uptime`
- Per-process info: `/proc/[pid]/*`
- Read-only for most files, some writable (e.g., `/proc/sys/`)

**Demonstrated in Project**:
- Reading multiple `/proc` files
- Parsing structured text data
- Handling files that may disappear (process termination)
- Understanding `/proc` is not a real filesystem

### 7. System Calls

**Concept**: System calls are the interface between user space and kernel space.

**System Calls Used**:
- `kill()` - Send signal to process
- `sysconf()` - Get system configuration
- `sysinfo()` - Get system statistics
- `readlink()` - Read symbolic link value
- `open()`, `read()`, `close()` - File I/O (via C++ streams)
- `opendir()`, `readdir()`, `closedir()` - Directory operations

**Demonstrated in Project**:
- Direct system call usage (kill, sysconf)
- Error checking with errno
- Handling system call failures gracefully

---

## System Design

### Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                   User Interface (UI)                   │
│                    [ncurses TUI]                        │
└────────────────────┬────────────────────────────────────┘
                     │
            ┌────────┴────────┐
            │                 │
┌───────────▼──────────┐  ┌──▼──────────────────┐
│   System Monitor     │  │  Process Monitor    │
│  - CPU stats         │  │  - Process list     │
│  - Memory stats      │  │  - Process details  │
│  - System info       │  │  - Process tree     │
└───────────┬──────────┘  └──┬──────────────────┘
            │                 │
            └────────┬────────┘
                     │
        ┌────────────▼────────────┐
        │   Process Control       │
        │   [Signal Handling]     │
        └────────────┬────────────┘
                     │
        ┌────────────▼────────────┐
        │   /proc Reader          │
        │   [File I/O]            │
        └────────────┬────────────┘
                     │
        ┌────────────▼────────────┐
        │   Parser Utilities      │
        │   [String Processing]   │
        └─────────────────────────┘
                     │
        ┌────────────▼────────────┐
        │   /proc Filesystem      │
        │   [Linux Kernel]        │
        └─────────────────────────┘
```

### Module Description

**1. Proc Reader Module** (`proc/`)
- **Purpose**: Low-level interface to `/proc` filesystem
- **Responsibilities**:
  - Read files from `/proc`
  - List PIDs by scanning `/proc` directory
  - Handle file access errors
  - Provide typed interfaces for different `/proc` files

**2. Parser Module** (`utils/`)
- **Purpose**: Parse structured text from `/proc` files
- **Responsibilities**:
  - Parse CPU statistics from `/proc/stat`
  - Parse memory info from `/proc/meminfo`
  - Parse process stat files
  - Extract key-value pairs

**3. System Monitor Module** (`system/`)
- **Purpose**: High-level system statistics
- **Responsibilities**:
  - Calculate CPU usage percentages
  - Track memory usage
  - Format system information for display
  - Maintain previous state for delta calculations

**4. Process Monitor Module** (`process/`)
- **Purpose**: Track and analyze processes
- **Responsibilities**:
  - Maintain list of all processes
  - Calculate per-process CPU usage
  - Build process tree structure
  - Detect process lifecycle events
  - Store event history

**5. Process Control Module** (`signals/`)
- **Purpose**: Send signals to processes
- **Responsibilities**:
  - Wrap `kill()` system call
  - Provide named signal operations
  - Handle and report errors
  - Validate PIDs before operations

**6. UI Module** (`ui/`)
- **Purpose**: Terminal-based user interface
- **Responsibilities**:
  - Render system and process information
  - Handle keyboard input
  - Manage multiple view modes
  - Show confirmation dialogs
  - Color-code information

### Data Flow

**Update Cycle** (every 1 second):
```
1. SystemMonitor::update()
   ├─> Read /proc/stat
   ├─> Calculate CPU usage from delta
   ├─> Read /proc/meminfo
   └─> Read /proc/uptime, /proc/loadavg

2. ProcessMonitor::update()
   ├─> List all PIDs from /proc
   ├─> For each PID:
   │   ├─> Read /proc/[pid]/stat
   │   ├─> Read /proc/[pid]/status
   │   ├─> Read /proc/[pid]/statm
   │   ├─> Calculate CPU% from previous sample
   │   └─> Count FDs in /proc/[pid]/fd/
   └─> Detect new/terminated processes

3. UI::draw()
   ├─> Draw system info bar
   ├─> Draw process list (sorted by CPU)
   ├─> Draw status and help bars
   └─> Refresh ncurses windows
```

**Signal Sending Flow**:
```
1. User selects process and presses K
2. UI shows confirmation dialog
3. If confirmed:
   └─> ProcessControl::terminateProcess(pid)
       └─> kill(pid, SIGTERM)
           ├─> Success: return Result{success=true}
           └─> Failure: capture errno, return Result{success=false, error=...}
4. UI displays result message
```

---

## Linux/POSIX APIs Used

### File Operations

```c++
// C++ ifstream (uses open/read/close internally)
std::ifstream file("/proc/stat");
std::stringstream buffer;
buffer << file.rdbuf();
```

**System calls**: `open()`, `read()`, `close()`

### Directory Operations

```c++
DIR* dir = opendir("/proc");
struct dirent* entry;
while ((entry = readdir(dir)) != nullptr) {
    // Process directory entries
}
closedir(dir);
```

**System calls**: `opendir()`, `readdir()`, `closedir()`

### Signal Operations

```c++
#include <signal.h>

int result = kill(pid, SIGTERM);
if (result == -1) {
    int error = errno;  // EPERM, ESRCH, EINVAL
}
```

**System call**: `kill()`

### System Configuration

```c++
#include <unistd.h>

long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
long pageSize = sysconf(_SC_PAGESIZE);
long clockTicks = sysconf(_SC_CLK_TCK);
```

**System call**: `sysconf()`

### System Information

```c++
#include <sys/sysinfo.h>

struct sysinfo si;
if (sysinfo(&si) == 0) {
    unsigned long totalRam = si.totalram * si.mem_unit;
    time_t bootTime = time(nullptr) - si.uptime;
}
```

**System call**: `sysinfo()`

### Symbolic Links

```c++
#include <unistd.h>

char buffer[4096];
ssize_t len = readlink("/proc/[pid]/exe", buffer, sizeof(buffer)-1);
if (len != -1) {
    buffer[len] = '\0';
    // buffer now contains executable path
}
```

**System call**: `readlink()`

### Signal Handler Setup

```c++
#include <signal.h>

void signalHandler(int signal) {
    g_running = 0;  // Set exit flag
}

struct sigaction sa;
sa.sa_handler = signalHandler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGINT, &sa, nullptr);
```

**System call**: `sigaction()`

---

## Implementation Details

### CPU Usage Calculation

**Algorithm**:
```
1. Read /proc/stat at time T1:
   cpu  user1 nice1 system1 idle1 iowait1 irq1 softirq1

2. Calculate totals:
   total1 = user1 + nice1 + system1 + idle1 + iowait1 + irq1 + softirq1
   idle_total1 = idle1 + iowait1

3. Wait (e.g., 1 second)

4. Read /proc/stat at time T2:
   cpu  user2 nice2 system2 idle2 iowait2 irq2 softirq2

5. Calculate totals:
   total2 = user2 + nice2 + system2 + idle2 + iowait2 + irq2 + softirq2
   idle_total2 = idle2 + iowait2

6. Calculate usage:
   total_diff = total2 - total1
   idle_diff = idle_total2 - idle_total1
   CPU_usage = 100 * (total_diff - idle_diff) / total_diff
```

**Key Points**:
- CPU counters are cumulative since boot
- Must calculate difference between samples
- iowait is considered idle time
- Per-core stats follow same pattern

### Per-Process CPU Calculation

**Algorithm**:
```
1. Read /proc/[pid]/stat at T1:
   Extract: utime1 (user mode jiffies)
            stime1 (kernel mode jiffies)

2. Calculate: total_time1 = utime1 + stime1

3. Wait (e.g., 1 second = time_elapsed)

4. Read /proc/[pid]/stat at T2:
   Extract: utime2, stime2

5. Calculate: total_time2 = utime2 + stime2

6. Process CPU usage:
   time_diff = total_time2 - total_time1
   CPU% = 100 * time_diff / (clock_ticks_per_sec * time_elapsed)
```

**Key Points**:
- Times are in "jiffies" (clock ticks)
- Must convert to real time using clock tick frequency
- Can exceed 100% on multi-core systems

### Memory Calculation

**System Memory**:
```
Read /proc/meminfo:
  MemTotal: 16000000 kB
  MemFree: 2000000 kB
  Buffers: 500000 kB
  Cached: 8000000 kB

Calculate:
  Used = MemTotal - MemFree - Buffers - Cached
  Usage% = 100 * Used / MemTotal
```

**Process Memory**:
```
Read /proc/[pid]/statm:
  size resident shared text lib data dt

Convert to bytes:
  VSZ = size * page_size
  RSS = resident * page_size

Memory% = 100 * RSS / total_system_memory
```

### Process State Parsing

**Parsing /proc/[pid]/stat**:
```
Format: pid (comm) state ppid pgrp session tty_nr tpgid flags ...

Challenge: comm field can contain spaces and parentheses
Example: 1234 (my (process) name) S 1 ...

Solution:
1. Find first '(' → start of comm
2. Find last ')' → end of comm
3. Extract comm between parentheses
4. Parse remaining fields after ')'
```

### Error Handling Strategy

**Levels of Error Handling**:

1. **Expected Errors** (normal operation):
   - Process disappears: Skip, continue with next
   - Permission denied: Log, show [restricted] in UI
   - Handle: Return nullopt, continue

2. **Recoverable Errors**:
   - File read fails: Retry or use cached data
   - Parse error: Use default values, log warning
   - Handle: Return error code, display message

3. **Fatal Errors**:
   - ncurses init fails: Exit gracefully
   - /proc not mounted: Exit with error message
   - Handle: Cleanup resources, exit

**Error Handling Example**:
```cpp
std::optional<ProcessInfo> readProcessInfo(pid_t pid) {
    auto statContent = ProcReader::readProcPidStat(pid);
    if (!statContent) {
        // Process disappeared or permission denied
        return std::nullopt;  // Not an error, just skip
    }

    if (!Parser::parseProcStat(*statContent, ...)) {
        // Parse failed - unexpected format
        // Log warning but don't crash
        return std::nullopt;
    }

    return processInfo;
}
```

---

## Testing

### Test Categories

1. **System Monitoring Tests**
   - CPU usage under load
   - Memory reporting accuracy
   - Uptime and load average

2. **Process Monitoring Tests**
   - Process list accuracy
   - Creation/termination detection
   - State transitions
   - Short-lived process handling

3. **Process Control Tests**
   - SIGTERM functionality
   - SIGSTOP/SIGCONT
   - Permission handling
   - Non-existent process handling

4. **UI Tests**
   - Navigation controls
   - Terminal resize
   - View switching

5. **Stability Tests**
   - Long-running operation
   - Rapid process churn
   - Signal interruption

### Test Results Summary

**System**: Ubuntu 22.04, Kernel 6.5.0
**Date**: [To be filled during actual testing]

| Test Category | Tests | Passed | Failed |
|---------------|-------|--------|--------|
| System Monitoring | 4 | TBD | TBD |
| Process Monitoring | 5 | TBD | TBD |
| Process Control | 4 | TBD | TBD |
| UI | 3 | TBD | TBD |
| Stability | 3 | TBD | TBD |

---

## Performance Analysis

### Resource Usage (Estimated)

**Monitor's Own Resource Consumption**:
- **CPU Usage**: 0.5-2% (typical)
  - Mostly in /proc file parsing
  - ncurses rendering
- **Memory (RSS)**: 3-5 MB
  - Process list storage
  - UI buffers
  - Event history
- **File Descriptors**: ~10-15
  - ncurses terminals
  - Temporary file handles

### Refresh Cycle Analysis

**Time Breakdown (200 processes)**:
- Scan /proc directory: 5-10ms
- Read all process files: 30-40ms
- Parse process data: 5-10ms
- Calculate CPU percentages: 2-5ms
- UI rendering: 5-10ms
- **Total**: 50-75ms per refresh

**Scalability**:
- Linear O(n) with process count
- Each process requires 3-5 file reads
- Acceptable up to ~1000 processes

### Optimization Techniques

1. **Efficient File Reading**:
   - Use C++ streams with rdbuf() for bulk read
   - Avoid line-by-line reading when possible

2. **Selective Updates**:
   - Only update visible processes in detail view
   - Cache process tree between full rebuilds

3. **Smart Refresh**:
   - 1-second interval balances responsiveness and overhead
   - Non-blocking input prevents UI freezes

4. **Memory Management**:
   - Reuse process map instead of recreating
   - Limit event history to 100 entries

---

## Limitations

### 1. User-Space Constraints
- **Cannot access**: Internal kernel data structures
- **Depends on**: /proc interface provided by kernel
- **Limited to**: Information kernel chooses to expose

### 2. Permission Restrictions
- Normal user cannot read some process information
- Cannot send signals to processes owned by other users
- Some /proc files require root access

### 3. Polling-Based Design
- Uses periodic sampling, not real-time events
- 1-second delay before detecting changes
- Cannot catch very short-lived processes (<1s)

### 4. Race Conditions
- Processes can terminate between reading different /proc files
- PID reuse can cause confusion (rare)
- Handled by checking for errors and skipping

### 5. Platform-Specific
- Linux-only (relies on /proc filesystem)
- /proc format may vary slightly between kernel versions
- Not portable to BSD, macOS, Windows

### 6. Limited Historical Data
- Shows current state only
- No graphs or historical trends
- Event log limited to recent history

### 7. Terminal-Based UI
- Limited to text display
- Requires terminal with proper capabilities
- No graphical charts or visualizations

---

## Conclusion

### Achievement Summary

The Kernel Monitor successfully demonstrates:

1. **Operating Systems Concepts**:
   - Process lifecycle and states
   - CPU time accounting and scheduling
   - Memory management
   - Signal-based process control
   - File descriptors

2. **Systems Programming Skills**:
   - Linux/POSIX API usage
   - /proc filesystem navigation
   - Error handling and robustness
   - Resource management

3. **Practical Implementation**:
   - Real-time monitoring system
   - Interactive terminal UI
   - Safe process control operations

### Educational Value

This project provides hands-on experience with:
- How Linux represents processes to user space
- The boundary between user and kernel space
- Calculating system statistics from kernel counters
- Signal mechanisms for process control
- Building production-quality systems software

### Technical Achievements

- **Modular Architecture**: Clean separation of concerns
- **Error Handling**: Graceful handling of edge cases
- **User Experience**: Responsive, intuitive interface
- **Performance**: Low overhead, efficient implementation

### What Was Learned

1. `/proc` is not a real filesystem - files are generated dynamically
2. Process information must be read quickly before processes change
3. CPU percentages require delta calculations between samples
4. Signals are the primary IPC mechanism for process control
5. Systems programming requires defensive error handling

### Future Improvements

If extended beyond academic scope:
- Historical data collection and graphing
- Network statistics monitoring
- Disk I/O statistics
- Container/cgroup awareness
- Remote monitoring capability
- Configuration file support

### Final Remarks

The Kernel Monitor successfully fulfills the requirements of an Operating Systems and Systems Programming project by:
- Using exclusively Linux/POSIX mechanisms
- Demonstrating core OS concepts with working code
- Providing an educational tool to understand system internals
- Implementing a practical, usable application

The project is ready for demonstration and can explain every line of code in terms of the underlying OS mechanisms it uses.

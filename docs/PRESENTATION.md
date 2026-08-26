# Kernel Monitor - Presentation Outline

## Slide 1: Title
- **Kernel Monitor: User-Space Linux System Monitoring**
- Operating Systems and Systems Programming Project
- Team members
- Date

---

## Slide 2: Problem Statement

**Why do we need system monitoring?**
- System administrators need to track resource usage
- Developers need to debug performance issues
- Understanding what's happening "under the hood"

**The Gap:**
- Tools like `top` and `htop` exist but are "black boxes"
- Students learn theory but not practical implementation
- Need to understand how Linux exposes kernel information

---

## Slide 3: Project Objectives

**Build a terminal-based system monitor that:**
1. Displays real-time CPU and memory usage
2. Lists and inspects running processes
3. Shows process hierarchy (tree view)
4. Allows basic process control (signals)
5. Demonstrates OS concepts with real code

**Key Constraint:** User-space only - no kernel modifications

---

## Slide 4: Operating Systems Concepts

**Process Management:**
- Process lifecycle (creation, execution, termination)
- Process states (Running, Sleeping, Stopped, Zombie)
- PID/PPID relationships

**CPU Scheduling:**
- CPU time accounting (user time, system time)
- Load averages
- Time slice allocation

**Memory Management:**
- Virtual memory vs. physical memory
- Resident Set Size (RSS)
- Page cache and buffers

**Signals:**
- Inter-process communication mechanism
- Process control (terminate, stop, continue)

---

## Slide 5: The /proc Filesystem

**What is /proc?**
- Pseudo-filesystem (not real disk files)
- Interface to kernel data structures
- Files generated dynamically on read

**Key Files:**
```
/proc/stat          → CPU statistics
/proc/meminfo       → Memory information
/proc/[pid]/stat    → Process statistics
/proc/[pid]/status  → Process status
/proc/[pid]/fd/     → Open file descriptors
```

**Live Demo:** `cat /proc/cpuinfo`, `cat /proc/meminfo`

---

## Slide 6: System Architecture

```
┌─────────────────────────────────────┐
│   User Interface (ncurses)          │
└────────────┬────────────────────────┘
             │
   ┌─────────┴─────────┐
   │                   │
┌──▼──────────┐  ┌────▼──────────┐
│   System    │  │   Process     │
│   Monitor   │  │   Monitor     │
└──┬──────────┘  └────┬──────────┘
   │                  │
   └────────┬─────────┘
            │
   ┌────────▼──────────┐
   │  /proc Reader     │
   │  (File I/O)       │
   └────────┬──────────┘
            │
   ┌────────▼──────────┐
   │  /proc Filesystem │
   │  (Linux Kernel)   │
   └───────────────────┘
```

**Modular Design:**
- Clear separation of concerns
- Each module has specific responsibility
- Testable components

---

## Slide 7: CPU Usage Calculation

**How to calculate CPU usage?**

1. Read `/proc/stat` at time T1:
   ```
   cpu  user nice system idle iowait irq softirq
   ```

2. Calculate total time and idle time

3. Wait (e.g., 1 second)

4. Read `/proc/stat` again at time T2

5. Calculate usage:
   ```
   CPU% = 100 × (Δtotal - Δidle) / Δtotal
   ```

**Key Insight:** Counters are cumulative; need delta between samples

**Live Demo:** Show actual /proc/stat content

---

## Slide 8: Process Information

**What do we track for each process?**

From `/proc/[pid]/stat`:
- PID, PPID (parent process ID)
- Process state (R, S, D, Z, T)
- CPU time (user + system)
- Priority, nice value
- Number of threads

From `/proc/[pid]/statm`:
- Virtual memory size
- Resident set size (physical memory)

From `/proc/[pid]/fd/`:
- Count of open file descriptors

**Challenge:** Parsing complex formats, handling race conditions

---

## Slide 9: Process Control with Signals

**Signal System:**
- Software interrupts for process communication
- Sent using `kill()` system call

**Signals Implemented:**
- **SIGTERM (15):** Graceful termination (catchable)
- **SIGKILL (9):** Forced termination (uncatchable)
- **SIGSTOP (19):** Suspend process
- **SIGCONT (18):** Resume process

**Safety Features:**
- Confirmation dialog for destructive operations
- Permission checking
- Error handling (EPERM, ESRCH)

**Code Example:**
```c++
int result = kill(pid, SIGTERM);
if (result == -1) {
    if (errno == EPERM) // Permission denied
    if (errno == ESRCH) // Process doesn't exist
}
```

---

## Slide 10: Linux/POSIX APIs Used

**System Calls:**
- `kill()` - Send signals
- `sysconf()` - System configuration
- `sysinfo()` - System information
- `readlink()` - Read symbolic links

**File Operations:**
- `open()`, `read()`, `close()` (via C++ streams)
- `opendir()`, `readdir()`, `closedir()`

**Signal Handling:**
- `sigaction()` - Setup signal handlers

**Why these matter:**
- Direct interaction with kernel
- Standard POSIX interfaces
- Portable across Linux systems

---

## Slide 11: User Interface

**Terminal-based UI using ncurses:**
- Multiple view modes (process list, details, tree, events)
- Color-coded display (green=normal, yellow=warning, red=critical)
- Keyboard-driven navigation

**Views:**
1. **Process List:** All processes sorted by CPU usage
2. **Process Details:** In-depth info on selected process
3. **Process Tree:** Parent-child hierarchy
4. **Events:** Recent process creation/termination

**Interactive Controls:**
- Arrow keys: Navigate
- Enter: View details
- K: Send SIGTERM
- S: Send SIGSTOP
- C: Send SIGCONT
- T: Tree view
- E: Events
- Q: Quit

---

## Slide 12: Live Demonstration

**Demo Script:**

1. **Start the monitor:**
   ```bash
   ./bin/kernel-monitor
   ```

2. **Show system information:**
   - CPU usage, memory usage, load average

3. **Navigate process list:**
   - Show sorting by CPU
   - Select a process

4. **View process details:**
   - Press Enter to see detailed info
   - Show PID, PPID, threads, memory, etc.

5. **Process tree:**
   - Press T to show hierarchy
   - Find init/systemd at root

6. **Process control:**
   - Start test process: `sleep 1000 &`
   - Select it in monitor
   - Send SIGSTOP (pause)
   - Send SIGCONT (resume)
   - Send SIGTERM (terminate)

7. **Event tracking:**
   - Press E to view events
   - Create/kill processes to see events

---

## Slide 13: Testing

**Test Categories:**

1. **System Monitoring:**
   - CPU calculation under load (`stress --cpu 4`)
   - Memory reporting accuracy

2. **Process Monitoring:**
   - Process creation/termination detection
   - State transitions
   - Race condition handling

3. **Process Control:**
   - Signal delivery
   - Permission handling
   - Error cases

4. **Stability:**
   - Long-running operation (5+ minutes)
   - Rapid process churn
   - Terminal resize

**Automated Test Suite:**
```bash
cd tests
./run_tests.sh
```

---

## Slide 14: Performance Analysis

**Resource Overhead:**
- CPU Usage: 0.5-2% (typical)
- Memory: 3-5 MB RSS
- Refresh Rate: 1 second

**Refresh Cycle (200 processes):**
- Process scan: 30-40ms
- Parsing: 5-10ms
- UI rendering: 5-10ms
- **Total: 50-75ms**

**Scalability:**
- Linear O(n) with process count
- Tested up to 500 processes
- Acceptable performance up to ~1000 processes

**Comparison:**
- Similar overhead to `top`
- More efficient than `htop` for basic monitoring

---

## Slide 15: Error Handling

**Types of Errors Handled:**

1. **Expected (Normal Operation):**
   - Process disappears during scan
   - Permission denied for other users' processes
   - → Skip gracefully, continue

2. **Recoverable:**
   - File read fails
   - Parse error
   - → Use default values, show error message

3. **Fatal:**
   - Cannot initialize ncurses
   - /proc not mounted
   - → Clean exit with error message

**Example:**
```cpp
auto procInfo = procMonitor.getProcessInfo(pid);
if (!procInfo) {
    // Process disappeared - not an error
    ui.setStatusMessage("Process no longer exists");
    return;
}
```

---

## Slide 16: Challenges and Solutions

**Challenge 1: Process Race Conditions**
- **Problem:** Process can terminate between reading different /proc files
- **Solution:** Check for errors on every read, skip if process disappeared

**Challenge 2: Parsing /proc/[pid]/stat**
- **Problem:** Process name can contain spaces and parentheses
- **Solution:** Find first '(' and last ')' to extract name correctly

**Challenge 3: CPU Usage Calculation**
- **Problem:** Counters are cumulative, not instantaneous
- **Solution:** Store previous values, calculate delta

**Challenge 4: Permission Handling**
- **Problem:** Cannot read all process info as normal user
- **Solution:** Gracefully skip restricted info, show what's available

---

## Slide 17: What We Learned

**Technical Skills:**
- Linux system programming with POSIX APIs
- /proc filesystem structure and usage
- Signal mechanisms and process control
- Terminal UI development with ncurses
- Error handling in systems software

**OS Concepts:**
- How Linux represents processes to user space
- CPU time accounting and scheduling
- Memory management from user perspective
- Process lifecycle and states
- Kernel-userspace boundary

**Software Engineering:**
- Modular architecture design
- Defensive programming
- Resource management
- Testing strategies

---

## Slide 18: Limitations

**By Design:**
- User-space only (cannot access internal kernel structures)
- Polling-based (not real-time kernel events)
- Terminal-only interface
- Linux-specific (/proc is not POSIX standard)

**Practical:**
- Cannot catch processes that live < 1 second
- Permission restrictions for other users' processes
- No historical data or graphs
- PID reuse edge cases

**Trade-offs:**
- Simplicity vs. feature richness
- Educational clarity vs. production features
- Performance vs. detailed monitoring

---

## Slide 19: Comparison with Existing Tools

| Feature | Kernel Monitor | top | htop | ps |
|---------|---------------|-----|------|-----|
| Real-time display | ✓ | ✓ | ✓ | ✗ |
| Process tree | ✓ | ✗ | ✓ | ✓ |
| Process control | ✓ | ✓ | ✓ | ✗ |
| Event tracking | ✓ | ✗ | ✗ | ✗ |
| Educational code | ✓ | ✗ | ✗ | ✗ |
| Resource usage | Low | Low | Medium | N/A |

**Unique Features:**
- Event log (process creation/termination)
- Clean, educational codebase
- Full source code walkthrough possible

---

## Slide 20: Future Enhancements

**Possible Extensions:**

1. **More Statistics:**
   - Network I/O monitoring
   - Disk I/O statistics
   - GPU usage

2. **Historical Data:**
   - Time-series graphs
   - Data logging
   - Trend analysis

3. **Advanced Features:**
   - Process filtering and search
   - Custom signal sending
   - CPU affinity display
   - Container/cgroup awareness

4. **User Experience:**
   - Configuration file
   - Color themes
   - Mouse support

**None required for the project - just possibilities!**

---

## Slide 21: Code Walkthrough

**Quick tour of key implementations:**

**1. Reading /proc/stat:**
```cpp
auto content = ProcReader::readProcStat();
unsigned long long user, nice, system, idle, ...;
Parser::parseCPULine(content, user, nice, system, 
                     idle, iowait, irq, softirq);
```

**2. Sending signals:**
```cpp
int result = kill(pid, SIGTERM);
if (result == -1) {
    return Result{false, strerror(errno)};
}
```

**3. Building process tree:**
```cpp
for (auto& proc : processes) {
    tree.children[proc.ppid].push_back(proc.pid);
}
```

**Demo:** Navigate through actual code

---

## Slide 22: Lessons Learned

**What Worked Well:**
- Modular architecture made development easier
- ncurses provided good UI abstraction
- /proc interface is surprisingly consistent
- Error handling prevented crashes

**What Was Challenging:**
- Parsing complex /proc formats
- Handling race conditions gracefully
- Calculating accurate CPU percentages
- Terminal resize handling

**Best Practices Applied:**
- Check every system call return value
- Handle expected errors gracefully
- Clean up resources properly
- Write testable code

---

## Slide 23: Educational Value

**For Students:**
- Demystifies system monitoring tools
- Shows practical application of OS theory
- Demonstrates systems programming practices
- Provides working code to study and extend

**For Instructors:**
- Complete project example
- Covers multiple OS concepts
- Clean, well-documented codebase
- Suitable for lab assignments or extensions

**Hands-on Learning:**
- Read and understand /proc files directly
- Experiment with process control
- Modify and extend functionality
- Debug real systems programming issues

---

## Slide 24: Conclusion

**Project Summary:**
- ✓ Built complete system monitoring tool
- ✓ Demonstrated core OS concepts with working code
- ✓ Used exclusively Linux/POSIX mechanisms
- ✓ Created educational resource for systems programming

**Deliverables:**
- Source code (all modules)
- Makefile (clean build system)
- Documentation (README, test cases, project doc)
- Test suite
- Working demonstration

**Success Criteria Met:**
- Monitors system and process state
- Uses /proc filesystem correctly
- Implements process control with signals
- Handles errors gracefully
- Explainable in OS viva

---

## Slide 25: Questions & Discussion

**Prepared to discuss:**
- Any part of the implementation
- OS concepts demonstrated
- Design decisions made
- Trade-offs considered
- Potential improvements

**Live Code Review:**
- Walk through any module
- Explain system calls used
- Show error handling
- Demonstrate testing

**Q&A Session**

---

## Backup Slides

### Backup 1: /proc File Format Examples

**/proc/stat format:**
```
cpu  74608 2520 24433 1117073 6176 4054 0 0 0 0
cpu0 18902 645 6108 279014 1544 1013 0 0 0 0
...
```

**/proc/[pid]/stat format:**
```
1234 (process) S 1 1234 1234 0 -1 4194304 ...
     ^name     ^state
```

### Backup 2: Signal Reference

| Signal | Number | Default Action | Catchable |
|--------|--------|---------------|-----------|
| SIGTERM | 15 | Terminate | Yes |
| SIGKILL | 9 | Terminate | No |
| SIGSTOP | 19 | Stop | No |
| SIGCONT | 18 | Continue | Yes |

### Backup 3: Build Instructions

```bash
# Clone/download project
cd kernel-monitor

# Install dependencies (Ubuntu)
sudo apt-get install build-essential libncurses-dev

# Build
make

# Run
./bin/kernel-monitor

# Test
cd tests
./run_tests.sh
```

---

**End of Presentation**

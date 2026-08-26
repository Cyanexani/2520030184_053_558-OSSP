# Test Cases for Kernel Monitor

This directory contains test cases and testing documentation for the Kernel Monitor project.

## Test Categories

### 1. System Monitoring Tests

**Test 1.1: CPU Usage Calculation**
- **Objective**: Verify CPU usage is calculated correctly
- **Procedure**:
  1. Start kernel monitor
  2. Note idle CPU percentage
  3. In another terminal: `stress --cpu 4 --timeout 30`
  4. Observe CPU usage increase
- **Expected**: CPU usage should increase significantly (70-100% on 4+ core systems)
- **Actual**: [To be filled during testing]

**Test 1.2: Memory Reporting**
- **Objective**: Verify memory statistics are accurate
- **Procedure**:
  1. Start kernel monitor
  2. Compare memory values with: `free -h`
  3. Create memory load: `stress --vm 2 --vm-bytes 512M --timeout 30`
  4. Observe memory usage change
- **Expected**: Values should closely match `free` output
- **Actual**: [To be filled during testing]

**Test 1.3: System Uptime**
- **Objective**: Verify uptime display
- **Procedure**:
  1. Start kernel monitor
  2. Compare uptime with: `uptime`
- **Expected**: Uptime should match within a few seconds
- **Actual**: [To be filled during testing]

**Test 1.4: Load Average**
- **Objective**: Verify load average reporting
- **Procedure**:
  1. Start kernel monitor
  2. Compare with: `uptime` or `cat /proc/loadavg`
- **Expected**: 1-minute load average should match
- **Actual**: [To be filled during testing]

### 2. Process Monitoring Tests

**Test 2.1: Process List Display**
- **Objective**: Verify processes are listed correctly
- **Procedure**:
  1. Start kernel monitor
  2. Count processes in monitor
  3. Compare with: `ps aux | wc -l`
- **Expected**: Process count should be similar
- **Actual**: [To be filled during testing]

**Test 2.2: Process Creation Detection**
- **Objective**: Verify new processes are detected
- **Procedure**:
  1. Start kernel monitor
  2. Press 'E' to view events
  3. In another terminal: `sleep 1000 &`
  4. Check if event appears
- **Expected**: "Process created" event should appear
- **Actual**: [To be filled during testing]

**Test 2.3: Process Termination Detection**
- **Objective**: Verify process termination is detected
- **Procedure**:
  1. Start kernel monitor with events view
  2. Start a process: `sleep 5 &`
  3. Wait for it to terminate naturally
  4. Check if termination event appears
- **Expected**: "Process terminated" event should appear
- **Actual**: [To be filled during testing]

**Test 2.4: Process State Accuracy**
- **Objective**: Verify process states are correct
- **Procedure**:
  1. Start kernel monitor
  2. Start process: `sleep 1000 &` (note PID)
  3. Find process in monitor - should be 'Sleeping'
  4. In terminal: `kill -STOP [PID]`
  5. Process state should change to 'Stopped'
  6. In terminal: `kill -CONT [PID]`
  7. State should return to 'Sleeping'
  8. Cleanup: `kill [PID]`
- **Expected**: State transitions should be visible
- **Actual**: [To be filled during testing]

**Test 2.5: Short-lived Process Handling**
- **Objective**: Verify monitor doesn't crash with rapidly changing processes
- **Procedure**:
  1. Start kernel monitor
  2. In another terminal: `for i in {1..100}; do sleep 0.1 & done`
  3. Monitor should remain stable
- **Expected**: No crashes, processes appear/disappear smoothly
- **Actual**: [To be filled during testing]

### 3. Process Details Tests

**Test 3.1: Process Details View**
- **Objective**: Verify detailed process information
- **Procedure**:
  1. Start kernel monitor
  2. Select a process
  3. Press Enter to view details
  4. Verify: PID, PPID, CPU%, Memory, Threads, etc.
- **Expected**: All fields populated with reasonable values
- **Actual**: [To be filled during testing]

**Test 3.2: File Descriptor Count**
- **Objective**: Verify FD count is accurate
- **Procedure**:
  1. Start kernel monitor
  2. Find bash process
  3. View details, note FD count
  4. In terminal: `ls -l /proc/[bash-pid]/fd | wc -l`
  5. Compare counts
- **Expected**: Counts should match (±1 for the ls itself)
- **Actual**: [To be filled during testing]

**Test 3.3: Command Line Display**
- **Objective**: Verify cmdline is shown correctly
- **Procedure**:
  1. Start a process with arguments: `sleep 9999 test arg &`
  2. Find it in monitor
  3. View details
  4. Check command line includes "test arg"
- **Expected**: Full command line visible
- **Actual**: [To be filled during testing]

### 4. Process Control Tests

**Test 4.1: SIGTERM**
- **Objective**: Test graceful termination
- **Procedure**:
  1. Start kernel monitor
  2. Start test process: `sleep 1000 &` (note PID)
  3. Select process in monitor
  4. Press 'K' (SIGTERM)
  5. Confirm action
  6. Verify process terminates
  7. In terminal: `ps -p [PID]` should show "no such process"
- **Expected**: Process terminates gracefully
- **Actual**: [To be filled during testing]

**Test 4.2: SIGSTOP and SIGCONT**
- **Objective**: Test process suspension and resumption
- **Procedure**:
  1. Start kernel monitor
  2. Start test process: `sleep 1000 &`
  3. Select process
  4. Press 'S' (SIGSTOP)
  5. Confirm - process state should change to 'Stopped'
  6. Press 'C' (SIGCONT)
  7. Process state should return to 'Sleeping'
- **Expected**: State changes correctly
- **Actual**: [To be filled during testing]

**Test 4.3: Permission Denied Handling**
- **Objective**: Test error handling for privileged processes
- **Procedure**:
  1. Start kernel monitor (as normal user)
  2. Select a root-owned process (e.g., init)
  3. Try to send SIGTERM
- **Expected**: Error message "Permission denied"
- **Actual**: [To be filled during testing]

**Test 4.4: Non-existent Process**
- **Objective**: Test handling of process that disappears
- **Procedure**:
  1. Start kernel monitor
  2. Start: `sleep 2 &` (note PID)
  3. Wait for it to terminate
  4. Try to send signal to it
- **Expected**: Error message "Process does not exist"
- **Actual**: [To be filled during testing]

### 5. Process Tree Tests

**Test 5.1: Tree View**
- **Objective**: Verify parent-child relationships
- **Procedure**:
  1. Start kernel monitor
  2. Press 'T' for tree view
  3. Verify init/systemd is at top
  4. Start: `bash -c 'sleep 1000' &`
  5. Find bash and its child sleep process in tree
- **Expected**: Tree shows correct hierarchy
- **Actual**: [To be filled during testing]

**Test 5.2: Multiple Levels**
- **Objective**: Test multi-level hierarchy
- **Procedure**:
  1. Start kernel monitor with tree view
  2. Run: `bash -c 'bash -c "sleep 1000"' &`
  3. Find the chain: bash → bash → sleep
- **Expected**: Three-level hierarchy visible
- **Actual**: [To be filled during testing]

### 6. UI Tests

**Test 6.1: Navigation**
- **Objective**: Test keyboard controls
- **Procedure**:
  1. Start kernel monitor
  2. Test: Up/Down arrows, Page Up/Down
  3. Test: Enter (details), ESC/B (back)
  4. Test: T (tree), E (events)
  5. Test: Q (quit)
- **Expected**: All controls work smoothly
- **Actual**: [To be filled during testing]

**Test 6.2: Terminal Resize**
- **Objective**: Test dynamic resizing
- **Procedure**:
  1. Start kernel monitor
  2. Resize terminal window (smaller and larger)
  3. Verify display adjusts correctly
- **Expected**: UI adapts without crashing
- **Actual**: [To be filled during testing]

**Test 6.3: Color Display**
- **Objective**: Test color coding
- **Procedure**:
  1. Start kernel monitor
  2. Verify high CPU usage shows in red/yellow
  3. Verify normal usage in green
- **Expected**: Appropriate colors for different levels
- **Actual**: [To be filled during testing]

### 7. Stability Tests

**Test 7.1: Long Running**
- **Objective**: Test stability over time
- **Procedure**:
  1. Start kernel monitor
  2. Let it run for 5+ minutes
  3. Observe for memory leaks or crashes
- **Expected**: Stable operation, no increasing memory usage
- **Actual**: [To be filled during testing]

**Test 7.2: Rapid Process Creation**
- **Objective**: Test under heavy process churn
- **Procedure**:
  1. Start kernel monitor
  2. Run: `while true; do (sleep 0.1 &); sleep 0.05; done` (in another terminal)
  3. Let run for 1 minute
  4. Stop the loop
  5. Monitor should remain stable
- **Expected**: No crashes, smooth operation
- **Actual**: [To be filled during testing]

**Test 7.3: Signal Interruption**
- **Objective**: Test graceful exit on interrupt
- **Procedure**:
  1. Start kernel monitor
  2. Press Ctrl+C
  3. Terminal should return to normal state
- **Expected**: Clean exit, cursor visible, no leftover artifacts
- **Actual**: [To be filled during testing]

### 8. Performance Tests

**Test 8.1: Resource Usage**
- **Objective**: Measure monitor's own resource usage
- **Procedure**:
  1. Start kernel monitor
  2. In another terminal: `ps aux | grep kernel-monitor`
  3. Note CPU% and memory usage
- **Expected**: CPU < 3%, Memory < 10MB
- **Actual**: [To be filled during testing]

**Test 8.2: Refresh Latency**
- **Objective**: Measure refresh speed
- **Procedure**:
  1. Start kernel monitor
  2. Time how long updates take
  3. On a system with ~100-200 processes
- **Expected**: Updates complete in < 100ms
- **Actual**: [To be filled during testing]

## Running Tests

### Automated Tests
```bash
cd tests
chmod +x run_tests.sh
./run_tests.sh
```

### Manual Tests
Follow each test procedure above and record results.

## Test Results Template

For each test, record:
- **Date**: [Test date]
- **System**: [OS version, kernel version]
- **Expected**: [What should happen]
- **Actual**: [What actually happened]
- **Status**: PASS / FAIL
- **Notes**: [Any observations]

## Known Issues

[Document any known issues discovered during testing]

## Test Environment

Recommended test environment:
- **OS**: Ubuntu 20.04 or later
- **Kernel**: 5.x or later
- **Terminal**: xterm, gnome-terminal, or similar
- **Privileges**: Normal user (some tests require root)

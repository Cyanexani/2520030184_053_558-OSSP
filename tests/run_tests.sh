#!/bin/bash

# Test script for Kernel Monitor
# This script performs basic validation tests

echo "=================================="
echo "Kernel Monitor - Test Suite"
echo "=================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0

# Helper functions
pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASSED=$((PASSED + 1))
}

fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAILED=$((FAILED + 1))
}

info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Test 1: Check if /proc is mounted
echo "Test 1: /proc filesystem availability"
if [ -d "/proc" ] && [ -r "/proc/stat" ]; then
    pass "/proc filesystem is accessible"
else
    fail "/proc filesystem is not accessible"
fi
echo ""

# Test 2: Check if ncurses is installed
echo "Test 2: ncurses library"
if ldconfig -p | grep -q libncurses; then
    pass "ncurses library is installed"
else
    fail "ncurses library is not installed"
    info "Install with: sudo apt-get install libncurses-dev"
fi
echo ""

# Test 3: Check compiler
echo "Test 3: C++ compiler"
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    pass "g++ is available: $GCC_VERSION"
else
    fail "g++ compiler not found"
fi
echo ""

# Test 4: Build the project
echo "Test 4: Build process"
cd ..
make clean > /dev/null 2>&1
if make > /dev/null 2>&1; then
    pass "Project builds successfully"
else
    fail "Build failed"
    info "Run 'make' manually to see errors"
fi
echo ""

# Test 5: Check executable
echo "Test 5: Executable creation"
if [ -f "bin/kernel-monitor" ]; then
    pass "Executable created: bin/kernel-monitor"

    # Check if executable has proper permissions
    if [ -x "bin/kernel-monitor" ]; then
        pass "Executable has execute permissions"
    else
        fail "Executable lacks execute permissions"
    fi
else
    fail "Executable not found"
fi
echo ""

# Test 6: Test /proc reading functions
echo "Test 6: /proc data availability"

# Test /proc/stat
if [ -r "/proc/stat" ]; then
    pass "/proc/stat is readable"
else
    fail "/proc/stat is not readable"
fi

# Test /proc/meminfo
if [ -r "/proc/meminfo" ]; then
    pass "/proc/meminfo is readable"
else
    fail "/proc/meminfo is not readable"
fi

# Test /proc/uptime
if [ -r "/proc/uptime" ]; then
    pass "/proc/uptime is readable"
else
    fail "/proc/uptime is not readable"
fi

# Test /proc/loadavg
if [ -r "/proc/loadavg" ]; then
    pass "/proc/loadavg is readable"
else
    fail "/proc/loadavg is not readable"
fi

# Test process directory
if [ -d "/proc/$$" ]; then
    pass "/proc/[pid] directories exist (tested with $$)"
else
    fail "/proc/[pid] directories not accessible"
fi
echo ""

# Test 7: Process information files
echo "Test 7: Process-specific files"
PID=$$

if [ -r "/proc/$PID/stat" ]; then
    pass "/proc/$PID/stat is readable"
else
    fail "/proc/$PID/stat is not readable"
fi

if [ -r "/proc/$PID/status" ]; then
    pass "/proc/$PID/status is readable"
else
    fail "/proc/$PID/status is not readable"
fi

if [ -r "/proc/$PID/cmdline" ]; then
    pass "/proc/$PID/cmdline is readable"
else
    fail "/proc/$PID/cmdline is not readable"
fi

if [ -d "/proc/$PID/fd" ]; then
    pass "/proc/$PID/fd directory exists"
else
    fail "/proc/$PID/fd directory not accessible"
fi
echo ""

# Test 8: Signal capabilities
echo "Test 8: Signal operations"

# Create a test process
sleep 300 &
TEST_PID=$!

sleep 1

# Test if process exists
if kill -0 $TEST_PID 2>/dev/null; then
    pass "Created test process (PID: $TEST_PID)"

    # Test SIGSTOP
    if kill -STOP $TEST_PID 2>/dev/null; then
        pass "Can send SIGSTOP"

        # Test SIGCONT
        if kill -CONT $TEST_PID 2>/dev/null; then
            pass "Can send SIGCONT"
        else
            fail "Cannot send SIGCONT"
        fi
    else
        fail "Cannot send SIGSTOP"
    fi

    # Cleanup test process
    kill -TERM $TEST_PID 2>/dev/null
    pass "Cleaned up test process"
else
    fail "Failed to create test process"
fi
echo ""

# Test 9: Memory and CPU statistics
echo "Test 9: System statistics format"

# Check /proc/stat format
STAT_LINE=$(head -n1 /proc/stat)
if echo "$STAT_LINE" | grep -q "^cpu "; then
    pass "/proc/stat has expected format"
else
    fail "/proc/stat format is unexpected"
fi

# Check /proc/meminfo format
if grep -q "MemTotal:" /proc/meminfo; then
    pass "/proc/meminfo contains MemTotal"
else
    fail "/proc/meminfo missing MemTotal"
fi

if grep -q "MemFree:" /proc/meminfo; then
    pass "/proc/meminfo contains MemFree"
else
    fail "/proc/meminfo missing MemFree"
fi
echo ""

# Test 10: Terminal requirements
echo "Test 10: Terminal environment"

if [ -t 1 ]; then
    pass "Running in a terminal"
else
    info "Not running in a terminal (batch mode)"
fi

if [ ! -z "$TERM" ]; then
    pass "TERM environment variable set: $TERM"
else
    fail "TERM environment variable not set"
fi
echo ""

# Summary
echo "=================================="
echo "Test Summary"
echo "=================================="
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    echo ""
    echo "You can now run the kernel monitor:"
    echo "  cd .."
    echo "  ./bin/kernel-monitor"
    exit 0
else
    echo -e "${RED}Some tests failed. Please fix the issues before running.${NC}"
    exit 1
fi

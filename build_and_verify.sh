#!/bin/bash

# Kernel Monitor - Build and Verification Script
# This script builds the project and runs basic verification

set -e  # Exit on error

echo "=========================================="
echo "Kernel Monitor - Build & Verify"
echo "=========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    echo -e "${RED}Error: Makefile not found. Please run from project root.${NC}"
    exit 1
fi

echo -e "${BLUE}Step 1: Checking dependencies...${NC}"

# Check for g++
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗ g++ not found${NC}"
    echo "Install with: sudo apt-get install build-essential"
    exit 1
else
    echo -e "${GREEN}✓ g++ found${NC}"
fi

# Check for ncurses
if ! ldconfig -p | grep -q libncurses; then
    echo -e "${RED}✗ ncurses library not found${NC}"
    echo "Install with: sudo apt-get install libncurses-dev"
    exit 1
else
    echo -e "${GREEN}✓ ncurses library found${NC}"
fi

# Check for /proc
if [ ! -d "/proc" ]; then
    echo -e "${RED}✗ /proc filesystem not found${NC}"
    exit 1
else
    echo -e "${GREEN}✓ /proc filesystem accessible${NC}"
fi

echo ""
echo -e "${BLUE}Step 2: Cleaning previous build...${NC}"
make clean > /dev/null 2>&1 || true
echo -e "${GREEN}✓ Clean complete${NC}"

echo ""
echo -e "${BLUE}Step 3: Building project...${NC}"
if make; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Step 4: Verifying executable...${NC}"
if [ -f "bin/kernel-monitor" ]; then
    echo -e "${GREEN}✓ Executable created: bin/kernel-monitor${NC}"

    # Check if executable
    if [ -x "bin/kernel-monitor" ]; then
        echo -e "${GREEN}✓ Executable has proper permissions${NC}"
    else
        echo -e "${RED}✗ Executable lacks execute permission${NC}"
        exit 1
    fi

    # Get file size
    SIZE=$(du -h bin/kernel-monitor | cut -f1)
    echo -e "${GREEN}✓ Executable size: $SIZE${NC}"
else
    echo -e "${RED}✗ Executable not found${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Step 5: Verifying file structure...${NC}"

REQUIRED_DIRS=("src" "src/ui" "src/proc" "src/process" "src/system" "src/signals" "src/utils" "tests" "docs")
for dir in "${REQUIRED_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓ Directory exists: $dir${NC}"
    else
        echo -e "${RED}✗ Missing directory: $dir${NC}"
        exit 1
    fi
done

REQUIRED_FILES=("README.md" "Makefile" "QUICKSTART.md" "PROJECT_SUMMARY.md")
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓ File exists: $file${NC}"
    else
        echo -e "${RED}✗ Missing file: $file${NC}"
        exit 1
    fi
done

echo ""
echo -e "${BLUE}Step 6: Quick functionality check...${NC}"

# Check if /proc/stat is readable
if [ -r "/proc/stat" ]; then
    echo -e "${GREEN}✓ Can read /proc/stat${NC}"
else
    echo -e "${RED}✗ Cannot read /proc/stat${NC}"
fi

# Check if /proc/meminfo is readable
if [ -r "/proc/meminfo" ]; then
    echo -e "${GREEN}✓ Can read /proc/meminfo${NC}"
else
    echo -e "${RED}✗ Cannot read /proc/meminfo${NC}"
fi

# Check if we can list PIDs
if [ -d "/proc/$$" ]; then
    echo -e "${GREEN}✓ Can access process directories (/proc/$$)${NC}"
else
    echo -e "${RED}✗ Cannot access process directories${NC}"
fi

echo ""
echo "=========================================="
echo -e "${GREEN}Build Verification Complete!${NC}"
echo "=========================================="
echo ""
echo "Next steps:"
echo ""
echo "1. Run the monitor:"
echo "   ./bin/kernel-monitor"
echo ""
echo "2. Run tests:"
echo "   cd tests && ./run_tests.sh"
echo ""
echo "3. Read documentation:"
echo "   - QUICKSTART.md (getting started)"
echo "   - README.md (full documentation)"
echo "   - PROJECT_SUMMARY.md (project overview)"
echo ""
echo "4. View keyboard controls in the monitor:"
echo "   - Q: Quit"
echo "   - ↑/↓: Navigate"
echo "   - Enter: Details"
echo "   - T: Tree view"
echo "   - E: Events"
echo "   - K: Kill process"
echo ""
echo -e "${YELLOW}Note: The monitor requires a terminal with ncurses support.${NC}"
echo -e "${YELLOW}Minimum terminal size: 80x24 characters${NC}"
echo ""
echo "Happy monitoring! 🚀"

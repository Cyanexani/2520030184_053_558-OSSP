#!/bin/bash

# Kernel Monitor - Diagnostic Script
# Run this to find out what's wrong

echo "🔍 Kernel Monitor - Diagnostic Check"
echo "===================================="
echo ""

# Check 1: Is project installed?
echo "1. Checking if project exists..."
if [ -d "$HOME/kernel-monitor" ]; then
    echo "   ✓ Found: $HOME/kernel-monitor"
    cd "$HOME/kernel-monitor"
else
    echo "   ✗ NOT FOUND: $HOME/kernel-monitor"
    echo "   → Run: curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash"
    exit 1
fi

echo ""

# Check 2: Is builder.sh there?
echo "2. Checking builder.sh..."
if [ -f "builder.sh" ]; then
    echo "   ✓ Found: builder.sh"
else
    echo "   ✗ NOT FOUND: builder.sh"
    exit 1
fi

echo ""

# Check 3: Is builder.sh executable?
echo "3. Checking permissions..."
if [ -x "builder.sh" ]; then
    echo "   ✓ builder.sh is executable"
else
    echo "   ✗ builder.sh is NOT executable"
    echo "   → Fixing: chmod +x builder.sh"
    chmod +x builder.sh
    echo "   ✓ Fixed!"
fi

echo ""

# Check 4: Is executable already built?
echo "4. Checking if executable exists..."
if [ -f "bin/kernel-monitor" ]; then
    echo "   ✓ Found: bin/kernel-monitor"
    SIZE=$(du -h bin/kernel-monitor | cut -f1)
    echo "   Size: $SIZE"
else
    echo "   ✗ NOT FOUND: bin/kernel-monitor"
    echo "   → Need to build. Run: make"
fi

echo ""

# Check 5: Is ncurses installed?
echo "5. Checking ncurses..."
if ldconfig -p 2>/dev/null | grep -q libncurses; then
    echo "   ✓ ncurses library found"
else
    echo "   ✗ ncurses NOT installed"
    echo "   → Install: sudo apt-get install libncurses-dev"
fi

echo ""

# Check 6: Is make available?
echo "6. Checking build tools..."
if command -v make &> /dev/null; then
    echo "   ✓ make found"
else
    echo "   ✗ make NOT found"
    echo "   → Install: sudo apt-get install make"
fi

echo ""
echo "===================================="
echo ""

# Summary and next steps
if [ -f "bin/kernel-monitor" ] && [ -x "bin/kernel-monitor" ]; then
    echo "✅ Everything looks good!"
    echo ""
    echo "To run the monitor, choose one:"
    echo ""
    echo "Option 1 (Easiest):"
    echo "  ./builder.sh --run"
    echo ""
    echo "Option 2 (Direct):"
    echo "  ./bin/kernel-monitor"
    echo ""
else
    echo "⚠️ Need to build first"
    echo ""
    echo "Run:"
    echo "  ./builder.sh"
    echo ""
    echo "Then:"
    echo "  ./builder.sh --run"
fi

echo ""

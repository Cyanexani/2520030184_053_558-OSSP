#!/bin/bash

# Kernel Monitor - Complete Uninstall Script

echo "🗑️  Kernel Monitor - Complete Uninstall"
echo "========================================"
echo ""

# Remove project directory
if [ -d "$HOME/kernel-monitor" ]; then
    echo "Removing: $HOME/kernel-monitor"
    rm -rf "$HOME/kernel-monitor"
    echo "✓ Removed"
else
    echo "ℹ Not found: $HOME/kernel-monitor"
fi

echo ""

# Remove system-wide installation
if command -v kernel-monitor &> /dev/null; then
    echo "Removing system-wide installation..."
    sudo rm -f /usr/local/bin/kernel-monitor
    echo "✓ Removed"
else
    echo "ℹ No system-wide installation found"
fi

echo ""
echo "========================================"
echo "✓ Uninstall complete!"
echo ""
echo "To reinstall fresh, run:"
echo "  curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash"
echo ""

#!/bin/bash

# Kernel Monitor - Simple Direct Installer
# This downloads, builds, and runs the latest version

set -e

echo "🚀 Kernel Monitor - Fresh Install"
echo "=================================="
echo ""

# Install directory
INSTALL_DIR="$HOME/kernel-monitor"

# Remove old if exists
if [ -d "$INSTALL_DIR" ]; then
    echo "Removing old installation..."
    rm -rf "$INSTALL_DIR"
fi

# Create fresh directory
mkdir -p "$INSTALL_DIR"
cd "$INSTALL_DIR"

echo "Downloading latest code..."

# Try git first
if command -v git &> /dev/null; then
    git clone --depth 1 https://github.com/Cyanexani/2520030184_053_558-OSSP.git .
else
    # Fallback to wget/curl
    if command -v wget &> /dev/null; then
        wget -q -O code.zip https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
    else
        curl -fsSL -o code.zip https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
    fi
    unzip -q code.zip
    mv 2520030184_053_558-OSSP-main/* .
    rm -rf 2520030184_053_558-OSSP-main code.zip
fi

echo "Installing dependencies..."
sudo apt-get update -qq
sudo apt-get install -qq -y build-essential libncurses-dev 2>/dev/null || true

echo "Building from scratch..."
chmod +x builder.sh
make clean
make

echo ""
echo "=================================="
echo "✅ Installation Complete!"
echo ""
echo "Run: ./bin/kernel-monitor"
echo ""
cd "$INSTALL_DIR"
./bin/kernel-monitor

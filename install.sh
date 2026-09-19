#!/bin/bash

# Kernel Monitor - Simple Direct Installer
# This downloads, builds, and runs the latest version

set -e

echo "🚀 Kernel Monitor - Fresh Install"
echo "=================================="
echo ""

# Install directory
INSTALL_DIR="$HOME/kernel-monitor"
REPO_URL="https://github.com/Cyanexani/2520030184_053_558-OSSP"

# Remove old if exists. Only wipe a directory that actually looks like a
# previous install of this project, so a stray path with the same name is
# never deleted by accident.
if [ -d "$INSTALL_DIR" ]; then
    if [ -f "$INSTALL_DIR/Makefile" ] && [ -d "$INSTALL_DIR/src" ]; then
        echo "Removing old installation..."
        rm -rf "$INSTALL_DIR"
    else
        echo "ERROR: $INSTALL_DIR exists but does not look like a Kernel Monitor"
        echo "       install (no Makefile and src/). Refusing to delete it."
        echo "       Move it aside, or set INSTALL_DIR to another path."
        exit 1
    fi
fi

# Create fresh directory
mkdir -p "$INSTALL_DIR"
cd "$INSTALL_DIR"

echo "Downloading latest code..."

# Try git first
if command -v git &> /dev/null; then
    git clone --depth 1 "$REPO_URL.git" .
else
    # Fallback to wget/curl
    if command -v wget &> /dev/null; then
        wget -q -O code.zip "$REPO_URL/archive/main.zip"
    else
        curl -fsSL -o code.zip "$REPO_URL/archive/main.zip"
    fi
    unzip -q code.zip
    mv 2520030184_053_558-OSSP-main/* .
    rm -rf 2520030184_053_558-OSSP-main code.zip
fi

echo "Installing dependencies..."
# libncurses-dev carries the wide-character library (libncursesw) that the UI
# links against; it is what makes the arrow keys and the tree glyphs render as
# single characters instead of one broken cell per byte.
sudo apt-get update -qq
sudo apt-get install -qq -y build-essential libncurses-dev

# Fail early with a readable message rather than a confusing linker error.
if ! ldconfig -p 2>/dev/null | grep -q libncursesw; then
    echo "ERROR: the wide-character ncurses library is missing."
    echo "       Install libncursesw-dev (Debian/Ubuntu) or"
    echo "       ncurses-devel (Fedora/RHEL) and re-run this script."
    exit 1
fi

echo "Building from scratch..."
chmod +x builder.sh
make clean
make

# make builds the monitor and the two demo programs; check all three.
for prog in kernel-monitor zombie orphan; do
    if [ ! -x "bin/$prog" ]; then
        echo "ERROR: build did not produce bin/$prog"
        exit 1
    fi
done

echo ""
echo "=================================="
echo "✅ Installation Complete!"
echo ""
echo "Run:   ./bin/kernel-monitor      (Q quits, T tree, +/- update rate)"
echo "Demos: ./bin/zombie 30           then: ps -eo pid,ppid,stat,comm | grep -w Z"
echo "       ./bin/orphan 25           watch the ppid change"
echo "       see demo/README.md for what each one shows"
echo ""
cd "$INSTALL_DIR"

# Only launch the UI when there is a real terminal. Piping this script through
# `curl ... | bash` leaves stdin attached to the pipe, and starting a full-screen
# ncurses app on a non-tty garbles the display and cannot read the quit key.
if [ -t 0 ] && [ -t 1 ]; then
    ./bin/kernel-monitor
else
    echo "Not running on a terminal, so the UI was not launched."
    echo "Start it yourself with:  cd $INSTALL_DIR && ./bin/kernel-monitor"
fi

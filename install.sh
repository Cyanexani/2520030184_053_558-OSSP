#!/bin/bash

# Kernel Monitor - ONE COMMAND Installation
# Usage: curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
# Or: bash <(curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh)
# Or: ./install.sh (if you have the file)

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║${NC}     🚀 Kernel Monitor - One Command Installation       ${BLUE}║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if git is available
if command -v git &> /dev/null; then
    echo -e "${GREEN}✓ Git found${NC}"
    USE_GIT=true
else
    echo -e "${YELLOW}⚠ Git not found - will use ZIP download${NC}"
    USE_GIT=false
fi

# Install to home directory
INSTALL_DIR="$HOME/kernel-monitor"

echo -e "${CYAN}▶ Installing to: $INSTALL_DIR${NC}"

# Remove existing installation if it exists
if [ -d "$INSTALL_DIR" ]; then
    echo -e "${YELLOW}⚠ Found existing installation, removing...${NC}"
    rm -rf "$INSTALL_DIR"
fi

# Create install directory
mkdir -p "$INSTALL_DIR"
cd "$INSTALL_DIR"

if [ "$USE_GIT" = true ]; then
    echo -e "${CYAN}▶ Cloning repository...${NC}"
    git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git .
else
    echo -e "${CYAN}▶ Downloading ZIP...${NC}"
    TEMP_ZIP=$(mktemp)
    trap "rm -f $TEMP_ZIP" EXIT

    if command -v wget &> /dev/null; then
        wget -q -O "$TEMP_ZIP" https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
    elif command -v curl &> /dev/null; then
        curl -fsSL -o "$TEMP_ZIP" https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
    else
        echo -e "${RED}✗ Neither wget nor curl found${NC}"
        exit 1
    fi

    echo -e "${CYAN}▶ Extracting...${NC}"
    unzip -q "$TEMP_ZIP"
    mv 2520030184_053_558-OSSP-main/* .
    rm -rf 2520030184_053_558-OSSP-main
fi

echo -e "${CYAN}▶ Making builder executable...${NC}"
chmod +x builder.sh

echo -e "${CYAN}▶ Running builder...${NC}"
./builder.sh

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║${NC}          ✨ Installation Complete!                      ${GREEN}║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}Project installed at: $INSTALL_DIR${NC}"
echo ""
echo -e "${CYAN}Run the monitor:${NC}"
echo "  cd $INSTALL_DIR"
echo "  ./builder.sh --run"
echo ""
echo -e "${CYAN}Or directly:${NC}"
echo "  $INSTALL_DIR/builder.sh --run"
echo ""
echo -e "${CYAN}Or if installed system-wide:${NC}"
echo "  kernel-monitor"
echo ""

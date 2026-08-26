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

# Create temporary directory
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

cd $TEMP_DIR

if [ "$USE_GIT" = true ]; then
    echo -e "${CYAN}▶ Cloning repository...${NC}"
    git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git project
else
    echo -e "${CYAN}▶ Downloading ZIP...${NC}"
    if command -v wget &> /dev/null; then
        wget -q https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
    elif command -v curl &> /dev/null; then
        curl -fsSL -o main.zip https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
    else
        echo -e "${RED}✗ Neither wget nor curl found${NC}"
        exit 1
    fi

    echo -e "${CYAN}▶ Extracting...${NC}"
    unzip -q main.zip
    mv 2520030184_053_558-OSSP-main project
fi

cd project

echo -e "${CYAN}▶ Making builder executable...${NC}"
chmod +x builder.sh

echo -e "${CYAN}▶ Running builder...${NC}"
./builder.sh

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║${NC}          ✨ Installation Complete!                      ${GREEN}║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}Run the monitor:${NC}"
echo "  ./builder.sh --run"
echo ""
echo -e "${CYAN}Or if installed system-wide:${NC}"
echo "  kernel-monitor"
echo ""

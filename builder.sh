#!/bin/bash

# Kernel Monitor - Automated Builder & Installer
# This script handles everything: dependencies, build, install

set -e  # Exit on error

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_NAME="Kernel Monitor"
EXECUTABLE="bin/kernel-monitor"

# Functions
print_header() {
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC} $1"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_step() {
    echo -e "${CYAN}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${CYAN}ℹ $1${NC}"
}

# Detect OS/Distro
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
    elif [ -f /etc/lsb-release ]; then
        . /etc/lsb-release
        OS=$(echo $DISTRIB_ID | tr '[:upper:]' '[:lower:]')
    else
        OS="unknown"
    fi
    echo $OS
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install dependencies
install_dependencies() {
    print_header "INSTALLING DEPENDENCIES"

    DISTRO=$(detect_distro)
    print_info "Detected OS: $DISTRO"

    case $DISTRO in
        ubuntu|debian)
            print_step "Installing for Ubuntu/Debian..."
            sudo apt-get update
            sudo apt-get install -y build-essential libncurses-dev git
            print_success "Ubuntu/Debian dependencies installed"
            ;;
        fedora)
            print_step "Installing for Fedora..."
            sudo dnf install -y gcc-c++ ncurses-devel git
            print_success "Fedora dependencies installed"
            ;;
        rhel|centos)
            print_step "Installing for RHEL/CentOS..."
            sudo yum install -y gcc-c++ ncurses-devel git
            print_success "RHEL/CentOS dependencies installed"
            ;;
        arch|manjaro)
            print_step "Installing for Arch Linux..."
            sudo pacman -Sy --noconfirm base-devel ncurses git
            print_success "Arch Linux dependencies installed"
            ;;
        alpine)
            print_step "Installing for Alpine Linux..."
            sudo apk add --no-cache build-base ncurses-dev git
            print_success "Alpine Linux dependencies installed"
            ;;
        *)
            print_warning "Unknown distro: $DISTRO"
            print_info "Please install manually:"
            echo "  - build-essential / gcc-c++"
            echo "  - libncurses-dev / ncurses-devel"
            echo "  - git"
            ;;
    esac
}

# Verify dependencies
verify_dependencies() {
    print_header "VERIFYING DEPENDENCIES"

    local all_good=true

    # Check g++
    if command_exists g++; then
        GCC_VERSION=$(g++ --version | head -n1)
        print_success "C++ Compiler: $GCC_VERSION"
    else
        print_error "g++ not found"
        all_good=false
    fi

    # Check make
    if command_exists make; then
        MAKE_VERSION=$(make --version | head -n1)
        print_success "Build tool: $MAKE_VERSION"
    else
        print_error "make not found"
        all_good=false
    fi

    # Check ncurses
    if ldconfig -p 2>/dev/null | grep -q libncurses; then
        print_success "ncurses library found"
    else
        print_error "ncurses library not found"
        all_good=false
    fi

    # Check /proc
    if [ -d "/proc/stat" ] || [ -f "/proc/stat" ]; then
        print_success "/proc filesystem accessible"
    else
        print_error "/proc filesystem not accessible"
        all_good=false
    fi

    if [ "$all_good" = false ]; then
        print_error "Some dependencies are missing!"
        return 1
    fi

    return 0
}

# Clean build
clean_build() {
    print_step "Cleaning previous build..."
    make clean 2>/dev/null || true
    print_success "Clean complete"
}

# Build project
build_project() {
    print_header "BUILDING PROJECT"

    print_step "Compiling..."
    if make; then
        print_success "Build successful"
        return 0
    else
        print_error "Build failed!"
        return 1
    fi
}

# Verify executable
verify_executable() {
    print_header "VERIFYING EXECUTABLE"

    if [ -f "$EXECUTABLE" ]; then
        print_success "Executable created: $EXECUTABLE"

        if [ -x "$EXECUTABLE" ]; then
            print_success "Executable permissions correct"
            SIZE=$(du -h "$EXECUTABLE" | cut -f1)
            print_info "Size: $SIZE"
            return 0
        else
            print_error "Executable not executable"
            return 1
        fi
    else
        print_error "Executable not found!"
        return 1
    fi
}

# Run tests
run_tests() {
    print_header "RUNNING TESTS"

    if [ -f "tests/run_tests.sh" ]; then
        print_step "Running automated tests..."
        bash tests/run_tests.sh
        print_success "Tests complete"
    else
        print_warning "Test script not found"
    fi
}

# Install system-wide
install_system_wide() {
    print_header "SYSTEM-WIDE INSTALLATION"

    read -p "Install to /usr/local/bin? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_step "Installing..."
        sudo make install
        print_success "Installation complete"
        print_info "Run 'kernel-monitor' from anywhere"
    else
        print_info "Skipping system-wide installation"
        print_info "Run with: $(pwd)/$EXECUTABLE"
    fi
}

# Show usage
show_usage() {
    echo -e "${CYAN}Usage:${NC}"
    echo "  ./builder.sh [OPTION]"
    echo ""
    echo -e "${CYAN}Options:${NC}"
    echo "  --full        Build everything (default)"
    echo "  --deps        Install dependencies only"
    echo "  --build       Build project only"
    echo "  --clean       Clean and rebuild"
    echo "  --test        Run tests only"
    echo "  --install     Install system-wide"
    echo "  --verify      Verify installation"
    echo "  --run         Run the monitor"
    echo "  --help        Show this help message"
    echo ""
    echo -e "${CYAN}Examples:${NC}"
    echo "  ./builder.sh                # Full build and install"
    echo "  ./builder.sh --deps         # Install dependencies"
    echo "  ./builder.sh --build        # Build only"
    echo "  ./builder.sh --run          # Run monitor"
}

# Main execution
main() {
    cd "$SCRIPT_DIR"

    case "${1:---full}" in
        --full)
            print_header "🚀 KERNEL MONITOR - FULL BUILD"
            install_dependencies && \
            verify_dependencies && \
            clean_build && \
            build_project && \
            verify_executable && \
            run_tests && \
            install_system_wide && \
            show_completion
            ;;
        --deps)
            install_dependencies
            ;;
        --build)
            build_project
            ;;
        --clean)
            clean_build && build_project && verify_executable
            ;;
        --test)
            run_tests
            ;;
        --install)
            install_system_wide
            ;;
        --verify)
            verify_dependencies && verify_executable
            ;;
        --run)
            if [ -f "$EXECUTABLE" ]; then
                print_header "RUNNING KERNEL MONITOR"
                print_info "Press Q to quit"
                ./$EXECUTABLE
            else
                print_error "Executable not found. Build first with: ./builder.sh"
            fi
            ;;
        --help)
            show_usage
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
}

# Completion message
show_completion() {
    print_header "✨ BUILD COMPLETE!"

    echo -e "${GREEN}Your ${PROJECT_NAME} is ready!${NC}"
    echo ""
    echo -e "${CYAN}Next steps:${NC}"
    echo "  1. Run the monitor:"
    echo "     ./builder.sh --run"
    echo "     or"
    echo "     kernel-monitor"
    echo ""
    echo "  2. View documentation:"
    echo "     cat QUICKSTART.md"
    echo "     cat README.md"
    echo ""
    echo "  3. Run tests:"
    echo "     ./builder.sh --test"
    echo ""
    echo -e "${CYAN}Keyboard controls:${NC}"
    echo "  Q       - Quit"
    echo "  ↑/↓     - Navigate"
    echo "  Enter   - Details"
    echo "  T       - Tree"
    echo "  E       - Events"
    echo "  K       - Kill"
    echo "  S       - Stop"
    echo "  C       - Continue"
    echo ""
    echo -e "${GREEN}Happy monitoring! 🚀${NC}"
    echo ""
}

# Run main
main "$@"

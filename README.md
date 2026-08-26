# Kernel Monitor

A **user-space Linux systems programming application** that provides real-time monitoring of Linux system and process states using Linux/POSIX APIs and the `/proc` filesystem.

---

## ⚡ Quick Start

### One Command Installation

```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### Then Run

```bash
./builder.sh --run
```

**Press Q to quit.**

---

## 📚 Documentation

| Guide | Purpose |
|-------|---------|
| **HOW_TO_RUN.md** | ⭐ How to run after installing |
| **INSTALL_SIMPLE.md** | Installation (3 easy steps) |
| **ONE_COMMAND.md** | One-command installation |
| **BUILDER.md** | Builder script reference |
| **QUICKSTART.md** | Quick keyboard reference |
| **WSL2_NO_GIT.md** | WSL2 without git |

---

## ✨ Features

### System Monitoring
- Real-time CPU usage (aggregate and per-core)
- Memory and swap usage
- Load averages, uptime, kernel version

### Process Monitoring
- Live process list with sorting
- Detailed process inspection
- Process tree visualization
- Event tracking (process creation/termination)
- File descriptor counting

### Process Control
- Send signals: SIGTERM, SIGKILL, SIGSTOP, SIGCONT
- Safety confirmations for destructive operations
- Permission and error handling

### User Interface
- Terminal-based with ncurses
- Multiple views: list, details, tree, events
- Color-coded display
- Keyboard navigation

---

## 🎮 Keyboard Controls

| Key | Action |
|-----|--------|
| **Q** | Quit |
| **↑/↓** | Navigate |
| **Enter** | Process details |
| **T** | Process tree |
| **E** | Events log |
| **K** | Kill process (SIGTERM) |
| **S** | Stop process (SIGSTOP) |
| **C** | Continue process (SIGCONT) |
| **R** | Refresh |

---

## 🔧 Requirements

- **OS**: Linux (any distribution)
- **Kernel**: 3.x or later
- **Dependencies**: build-essential, libncurses-dev
- **Compiler**: g++ with C++17 support

All dependencies are installed automatically by the builder script.

---

## 🛠️ Build Commands

```bash
# Build
make

# Clean
make clean

# Debug build
make debug

# Install system-wide
sudo make install
```

---

## 📋 How It Works

**Kernel Monitor uses Linux/POSIX APIs to:**

1. Read system statistics from `/proc` filesystem
2. Calculate CPU usage, memory usage, load averages
3. List and inspect processes
4. Send signals for process control
5. Display real-time updates in terminal UI

**Key Files Used:**
- `/proc/stat` - CPU statistics
- `/proc/meminfo` - Memory information
- `/proc/uptime` - System uptime
- `/proc/[pid]/stat` - Process statistics
- `/proc/[pid]/fd/` - File descriptors

---

## 🎯 Linux/POSIX APIs

- `kill()` - Send signals
- `sysconf()` - System configuration
- `sysinfo()` - System information
- `opendir()`, `readdir()`, `closedir()` - Directory operations
- `sigaction()` - Signal handling
- File I/O via C++ streams

---

## 📊 Project Structure

```
src/
├── main.cpp              # Application entry point
├── ui/                   # Terminal UI (ncurses)
├── proc/                 # /proc filesystem reader
├── process/              # Process monitoring
├── system/               # System statistics
├── signals/              # Signal operations
└── utils/                # Parsing utilities

tests/                     # Test suite
docs/                      # Technical documentation
builder.sh                 # Automated installer
install.sh                 # One-command installer
Makefile                   # Build system
```

---

## 🚀 Installation Methods

### Method 1: One Command (Easiest)
```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### Method 2: With Git
```bash
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
./builder.sh
```

### Method 3: Without Git (ZIP)
1. Download ZIP: https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
2. Extract ZIP
3. Run: `./builder.sh`

### Method 4: WSL2 Without Git
See **WSL2_NO_GIT.md**

---

## 🧪 Testing

```bash
# Run automated tests
cd tests
./run_tests.sh
```

Tests verify:
- System monitoring accuracy
- Process detection
- Process control operations
- UI functionality
- Stability under load

---

## 📈 Performance

- **CPU Usage**: 0.5-2% (typical)
- **Memory**: 3-5 MB RSS
- **Refresh Rate**: 1 second
- **Scalability**: Handles 500+ processes

---

## 🎓 Educational Value

Demonstrates:
- Process management and lifecycle
- CPU scheduling and time accounting
- Memory management from user space
- Signal mechanisms and IPC
- File descriptors and I/O
- `/proc` filesystem architecture
- Systems programming best practices

---

## ✅ What's Inside

- **2,500+ lines** of C++ code
- **7 modular components**
- **15+ Linux/POSIX APIs**
- **40+ test scenarios**
- **Complete documentation**
- **Automated build system**
- **One-command installer**

---

## 🔗 Repository

https://github.com/Cyanexani/2520030184_053_558-OSSP

---

## 📞 Need Help?

1. **How to run?** → See **HOW_TO_RUN.md**
2. **How to install?** → See **INSTALL_SIMPLE.md**
3. **Quick reference?** → See **QUICKSTART.md**
4. **WSL2 issues?** → See **WSL2_NO_GIT.md**
5. **Builder questions?** → See **BUILDER.md**

---

## 🎉 Quick Example

```bash
# Install
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash

# Run
./builder.sh --run

# Use
# Navigate with arrow keys
# Press K to kill a process
# Press Q to quit
```

---

## 📄 License

Educational project for Operating Systems and Systems Programming course.

---

**Ready to monitor your Linux system!** 🚀

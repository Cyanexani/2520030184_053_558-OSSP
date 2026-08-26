# 🔨 Kernel Monitor Builder

Automated build and installation script for Kernel Monitor.

## Quick Start

```bash
# Make executable
chmod +x builder.sh

# Run full build (automatic dependency installation)
./builder.sh

# Or just build
./builder.sh --build

# Or run the monitor
./builder.sh --run
```

---

## Commands

| Command | What It Does |
|---------|-------------|
| `./builder.sh` | **Full build** - Install deps, build, test, install system-wide |
| `./builder.sh --full` | Same as above |
| `./builder.sh --deps` | Install dependencies only |
| `./builder.sh --build` | Build project only |
| `./builder.sh --clean` | Clean and rebuild |
| `./builder.sh --test` | Run test suite |
| `./builder.sh --install` | Install to /usr/local/bin |
| `./builder.sh --verify` | Verify dependencies and executable |
| `./builder.sh --run` | Build and run the monitor |
| `./builder.sh --help` | Show help message |

---

## Examples

### Full Installation (One Command)
```bash
./builder.sh
```
This will:
- ✓ Detect your Linux distribution
- ✓ Install dependencies automatically (may ask for sudo password)
- ✓ Verify all dependencies
- ✓ Clean previous builds
- ✓ Build the project
- ✓ Verify executable
- ✓ Run tests
- ✓ Ask if you want system-wide installation

### Just Build
```bash
./builder.sh --build
```

### Run the Monitor
```bash
./builder.sh --run
```

### Install System-Wide
```bash
./builder.sh --install
# Then run from anywhere: kernel-monitor
```

### Clean Build
```bash
./builder.sh --clean
```

---

## Supported Distributions

✓ Ubuntu / Debian  
✓ Fedora / RHEL / CentOS  
✓ Arch Linux / Manjaro  
✓ Alpine Linux  
✓ Any Linux with package manager  

---

## What It Does

### Step 1: Detect Distro
Identifies your Linux distribution (Ubuntu, Fedora, Arch, etc.)

### Step 2: Install Dependencies
Installs required packages:
- `build-essential` / `gcc-c++` - C++ compiler
- `libncurses-dev` / `ncurses-devel` - Terminal UI library
- `git` - Version control

### Step 3: Verify Dependencies
Checks that all tools are installed correctly

### Step 4: Clean Build
Removes old build artifacts

### Step 5: Build Project
Compiles all source files

### Step 6: Verify Executable
Confirms the binary was created successfully

### Step 7: Run Tests
Executes automated test suite

### Step 8: Install System-Wide
Optionally copies executable to `/usr/local/bin`

---

## System Requirements

- **OS**: Linux (any distribution)
- **Kernel**: 3.x or later
- **Internet**: Only needed for dependency installation
- **sudo**: May ask for password during installation

---

## Troubleshooting

### Script won't run
```bash
chmod +x builder.sh
```

### Build fails
```bash
# Try clean rebuild
./builder.sh --clean
```

### Dependency installation fails
```bash
# Try manual installation based on your distro
# Ubuntu:
sudo apt-get install build-essential libncurses-dev

# Fedora:
sudo dnf install gcc-c++ ncurses-devel

# Arch:
sudo pacman -S base-devel ncurses
```

### Need sudo password
The script may ask for sudo password during:
- Dependency installation
- System-wide installation

This is normal and required.

---

## Output Example

```
╔════════════════════════════════════════════════════════════╗
║ 🚀 KERNEL MONITOR - FULL BUILD                            ║
╚════════════════════════════════════════════════════════════╝

▶ Installing dependencies...
ℹ Detected OS: ubuntu
✓ Ubuntu/Debian dependencies installed

▶ Verifying dependencies...
✓ C++ Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.1)
✓ Build tool: GNU Make 4.3
✓ ncurses library found
✓ /proc filesystem accessible

▶ Building project...
✓ Build successful

✓ Executable created: bin/kernel-monitor
✓ Executable permissions correct
ℹ Size: 245K

▶ Running tests...
✓ All tests passed!

╔════════════════════════════════════════════════════════════╗
║ ✨ BUILD COMPLETE!                                         ║
╚════════════════════════════════════════════════════════════╝
```

---

## After Build

Your executable is at: `bin/kernel-monitor`

### Run Locally
```bash
./bin/kernel-monitor
```

### Run System-Wide (if installed)
```bash
kernel-monitor
```

### View Documentation
```bash
cat QUICKSTART.md
cat README.md
```

---

## Features

✨ **Automatic distro detection** - Works on any Linux  
✨ **One-command build** - Everything automated  
✨ **Colorized output** - Easy to read  
✨ **Error checking** - Detects problems early  
✨ **Multiple options** - Build only, test only, etc.  
✨ **Professional** - Production-quality builder  

---

## Notes

- The builder script requires internet for package downloads
- First run may take 2-5 minutes depending on internet speed
- Subsequent builds are much faster (seconds)
- To uninstall: `sudo make uninstall`

---

**Ready to build? Run: `./builder.sh`** 🚀

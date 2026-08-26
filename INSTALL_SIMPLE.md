# ⚡ INSTALLATION - SUPER SIMPLE

**This guide will take you from zero to running the Kernel Monitor in 5 minutes.**

---

## 🎯 The Simplest Way - 3 Steps

### Step 1: Open Terminal and Copy-Paste This

```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
```

**What it does:** Downloads your project to your computer.

---

### Step 2: Run the Builder (Does Everything)

```bash
./builder.sh
```

**What it does:** 
- Installs all software you need
- Builds the project
- Runs tests
- Installs the monitor

**Note:** You'll see `[sudo] password for username:` - type your password and press Enter.

---

### Step 3: Run the Monitor

```bash
./builder.sh --run
```

**Done!** You're now running Kernel Monitor.

```
┌──────────────────── KERNEL MONITOR ────────────────────┐
│ CPU: 34%     RAM: 61%     SWAP: 4%     LOAD: 1.32      │
│ Kernel: 6.x.x      Uptime: 04:21:17                    │
├────────────────────────────────────────────────────────┤
│ PID     PROCESS       CPU%     MEM%     STATE    THR    │
│ ...                                                     │
└────────────────────────────────────────────────────────┘
```

Press **Q** to quit.

---

## 🎮 Use the Monitor

| Key | What It Does |
|-----|-------------|
| **Q** | Exit/Quit |
| **↑ / ↓** | Move up/down in process list |
| **Enter** | See details of selected process |
| **T** | Show process tree (family view) |
| **E** | Show events (process birth/death) |
| **K** | Kill selected process |
| **S** | Stop (pause) selected process |
| **C** | Continue (resume) stopped process |
| **R** | Refresh display |

---

## ⚠️ If Something Goes Wrong

### "git: command not found"
**Solution:** Git is not installed.
```bash
# Ubuntu/Debian
sudo apt-get install git

# Fedora
sudo dnf install git

# Arch
sudo pacman -S git
```

Then try again:
```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

---

### "Permission denied: builder.sh"
**Solution:** Make it executable.
```bash
chmod +x builder.sh
./builder.sh
```

---

### "builder.sh: command not found"
**Solution:** You need to be in the right folder.
```bash
cd 2520030184_053_558-OSSP
./builder.sh
```

---

### "fatal: 'origin' does not appear to be a 'git' repository"
**Solution:** You're in the wrong folder.
```bash
cd 2520030184_053_558-OSSP
```

---

### Builder asks for password
**This is normal!** Type your password (you won't see it) and press Enter.

---

### "ncurses: command not found" during build
**Solution:** ncurses library is missing. The builder should have installed it, but try manually:

```bash
# Ubuntu/Debian
sudo apt-get install libncurses-dev

# Fedora
sudo dnf install ncurses-devel

# Arch
sudo pacman -S ncurses
```

Then rebuild:
```bash
./builder.sh --build
```

---

## 📱 Different Ways to Install

### Way 1: FASTEST (Recommended)
```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
./builder.sh
./builder.sh --run
```

### Way 2: Step by Step
```bash
# Install dependencies manually
sudo apt-get install build-essential libncurses-dev
# Or for Fedora: sudo dnf install gcc-c++ ncurses-devel
# Or for Arch: sudo pacman -S base-devel ncurses

# Get the code
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP

# Build
make

# Run
./bin/kernel-monitor
```

### Way 3: Without Git (If you can't use git)
1. Visit: https://github.com/Cyanexani/2520030184_053_558-OSSP
2. Click **Code** (green button)
3. Click **Download ZIP**
4. Unzip the file
5. Open terminal in unzipped folder
6. Run: `./builder.sh`

---

## ✅ Verify Installation Works

After running builder, check if the monitor works:

```bash
./bin/kernel-monitor
```

You should see the Kernel Monitor display.

Press **Q** to exit.

---

## 🎯 If You Want System-Wide Installation

After successful build, run:

```bash
./builder.sh --install
```

Now you can run from anywhere:
```bash
kernel-monitor
```

---

## 📍 Where Are Things?

After installation:

| Item | Location |
|------|----------|
| **Executable** | `bin/kernel-monitor` |
| **Source Code** | `src/` folder |
| **Documentation** | `README.md` |
| **Quick Start** | `QUICKSTART.md` |
| **Builder Script** | `builder.sh` |

---

## 🔍 Test It's Working

```bash
# Check if executable exists
ls -lh bin/kernel-monitor

# Should show something like:
# -rwxr-xr-x 1 user user 245K Aug 26 10:50 bin/kernel-monitor
```

---

## 🆘 Still Stuck?

### Option 1: Check the Docs
```bash
cat README.md          # Full documentation
cat QUICKSTART.md      # Quick reference
cat BUILDER.md         # Builder script help
```

### Option 2: Check System
```bash
# Check if you have git
git --version

# Check if you have g++
g++ --version

# Check if you have make
make --version

# Check if ncurses is installed
ldconfig -p | grep ncurses
```

### Option 3: Full Clean Install
```bash
# Go to project folder
cd 2520030184_053_558-OSSP

# Clean everything
rm -rf bin build

# Full rebuild
./builder.sh --clean
./builder.sh --build
./builder.sh --run
```

---

## ⏱️ How Long Does It Take?

| Task | Time |
|------|------|
| Download project | 30 seconds |
| Install dependencies (first time) | 2-5 minutes |
| Build project | 30 seconds |
| Run tests | 1 minute |
| **Total First Time** | **5-10 minutes** |
| **Rebuild (no deps)** | **30 seconds** |

---

## 🎉 Quick Command Reference

```bash
# Download project
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP

# Make builder executable (if needed)
chmod +x builder.sh

# Run everything
./builder.sh

# Just build
./builder.sh --build

# Run monitor
./builder.sh --run

# Run tests
./builder.sh --test

# Clean and rebuild
./builder.sh --clean

# Install system-wide
./builder.sh --install
```

---

## 🚀 That's It!

You now have Kernel Monitor running on your system!

**To run it again:**
```bash
./bin/kernel-monitor
```

**To uninstall:**
```bash
make clean
# Or if system-wide installed:
sudo make uninstall
```

---

**Enjoy monitoring your Linux system!** 🎊

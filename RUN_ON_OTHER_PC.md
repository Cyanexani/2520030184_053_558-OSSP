# 🖥️ Run on Another PC - Complete Guide

**Step-by-step: Get Kernel Monitor working on a different computer.**

---

## ⚡ FASTEST WAY (ONE COMMAND)

On the other PC, open terminal and paste:

```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

**That's it!** It will:
- ✅ Download everything
- ✅ Install dependencies
- ✅ Build the project
- ✅ Ask if you want system-wide install

Then run:
```bash
./builder.sh --run
```

---

## 📋 Step-by-Step (If One Command Doesn't Work)

### Step 1: Check Prerequisites

**Open terminal and check:**

```bash
# Check internet
ping github.com

# Check git
git --version

# Check curl
curl --version
```

### Step 2: Choose Installation Method

#### Method A: With Git (Recommended)

```bash
# Clone the repository
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Enter folder
cd 2520030184_053_558-OSSP

# Make builder executable
chmod +x builder.sh

# Run builder (installs dependencies + builds)
./builder.sh

# Follow on-screen prompts
```

#### Method B: Without Git (Use ZIP)

```bash
# Download ZIP using curl
curl -L -o kernel-monitor.zip https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip

# Extract
unzip kernel-monitor.zip

# Enter folder
cd 2520030184_053_558-OSSP-main

# Make builder executable
chmod +x builder.sh

# Run builder
./builder.sh
```

---

## 🎯 What the Builder Script Does

When you run `./builder.sh`, it automatically:

1. **Detects your Linux distro** (Ubuntu, Fedora, Arch, etc.)
2. **Installs dependencies:**
   - C++ compiler (g++)
   - ncurses library
   - Build tools (make)
3. **Builds the project**
4. **Runs tests**
5. **Offers system-wide installation**

---

## ▶️ After Installation - How to Run

### Option 1: Local Run
```bash
cd 2520030184_053_558-OSSP
./builder.sh --run
```

### Option 2: System-Wide (if installed)
```bash
# From anywhere:
kernel-monitor
```

### Option 3: Direct Executable
```bash
./bin/kernel-monitor
```

---

## 🎮 Using Kernel Monitor

Once running, you'll see the monitor screen:

```
┌──────────────────── KERNEL MONITOR ────────────────────┐
│ CPU: 34%     RAM: 61%     SWAP: 4%     LOAD: 1.32      │
│ Kernel: 6.x.x      Uptime: 04:21:17                    │
├────────────────────────────────────────────────────────┤
│ PID     PROCESS       CPU%     MEM%     STATE    THR    │
│ ...                                                     │
└────────────────────────────────────────────────────────┘
```

### Keyboard Controls

| Key | Action |
|-----|--------|
| **Q** | Quit |
| **↑/↓** | Navigate |
| **Enter** | Process details |
| **T** | Process tree |
| **E** | Events |
| **K** | Kill process |
| **S** | Stop process |
| **C** | Continue process |

---

## 🆘 Common Issues on Other PC

### Issue 1: "curl: command not found"

**Install curl:**
```bash
# Ubuntu/Debian
sudo apt-get install curl

# Fedora
sudo dnf install curl

# Arch
sudo pacman -S curl
```

Then try one-command installation again.

### Issue 2: "git: command not found"

**Solution:** Use ZIP method instead (see Method B above)

Or install git:
```bash
sudo apt-get install git  # Ubuntu
sudo dnf install git      # Fedora
sudo pacman -S git        # Arch
```

### Issue 3: "Permission denied: builder.sh"

```bash
chmod +x builder.sh
./builder.sh
```

### Issue 4: Builder asks for sudo password

**This is normal!** Type your password (you won't see it) and press Enter.

### Issue 5: "libncurses.so.6: cannot open shared object file"

**Install ncurses library:**
```bash
# Ubuntu/Debian
sudo apt-get install libncurses6

# Fedora
sudo dnf install ncurses-libs

# Arch
sudo pacman -S ncurses
```

Then rebuild:
```bash
./builder.sh --clean
./builder.sh --build
```

### Issue 6: Build still fails

**Try manual steps:**
```bash
# 1. Install dependencies manually
sudo apt-get update
sudo apt-get install build-essential libncurses-dev

# 2. Build manually
make clean
make

# 3. Run
./bin/kernel-monitor
```

---

## 📊 Quick Reference for Different Linux Distros

### Ubuntu/Debian
```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### Fedora/RHEL
```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### Arch Linux
```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### WSL2 (Windows Subsystem for Linux)
```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

**All work the same!** ✅

---

## ✅ Verification Checklist

After installation, verify it works:

```bash
# 1. Check executable exists
ls -lh bin/kernel-monitor
# Should show: -rwxr-xr-x ... bin/kernel-monitor

# 2. Run it
./bin/kernel-monitor
# Should show Kernel Monitor screen

# 3. Check if installed system-wide
which kernel-monitor
# If successful, shows: /usr/local/bin/kernel-monitor
```

---

## 🎯 Summary: What to Tell Someone on Another PC

**Just give them this:**

```
1. Open terminal
2. Copy-paste this command:
   curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash

3. Wait for it to finish (5-10 minutes first time)

4. Run:
   ./builder.sh --run

5. Press Q to quit
```

---

## 📞 Still Not Working?

Check the following guides on GitHub:

1. **ONE_COMMAND.md** - One-command installation
2. **INSTALL_SIMPLE.md** - Simple 3-step guide
3. **CLONE_TROUBLESHOOTING.md** - Cloning issues
4. **PERMISSIONS_SETUP.md** - Access/permission issues
5. **WSL2_NO_GIT.md** - WSL2 specific help

Visit: https://github.com/Cyanexani/2520030184_053_558-OSSP

---

**Now you can install on any PC!** 🚀

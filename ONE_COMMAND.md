# ⚡ ONE COMMAND INSTALLATION

**Copy and paste ONE command to get everything working.**

---

## 🚀 THE ONE COMMAND

### Option 1: Using curl (Recommended)

```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### Option 2: Using wget

```bash
wget -qO- https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

### Option 3: Local install script

```bash
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
bash install.sh
```

---

## ⏱️ What Happens

**The script automatically:**

1. ✅ Checks if git is installed
2. ✅ Clones repo (or downloads ZIP if no git)
3. ✅ Makes builder executable
4. ✅ Runs `./builder.sh` (installs dependencies + builds)
5. ✅ Done!

**Total time: 5-10 minutes (first time)**

---

## 🎮 After Installation

```bash
# Run the monitor
./builder.sh --run

# Or if system-wide installed
kernel-monitor
```

**Press Q to quit**

---

## ✅ That's It!

**One command. No hassle. Just works!** 🎉

---

## 🆘 If It Fails

The script handles:
- ✅ Missing git (uses ZIP)
- ✅ Missing curl/wget (tells you)
- ✅ All Linux distros (auto-detects)
- ✅ WSL2 (works perfectly)

If something goes wrong, it will tell you clearly.

---

## 📋 What Gets Installed

- All dependencies (build-essential, ncurses, etc.)
- Kernel Monitor executable
- All documentation
- Test suite
- Ready to use!

---

## 🔗 Repository

https://github.com/Cyanexani/2520030184_053_558-OSSP

---

**Just run the command and relax!** ☕

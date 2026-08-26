# 🪟 WSL2 Installation (Without Git)

**If git is not available on your WSL2, use these methods instead.**

---

## ⚡ Method 1: Install Git First (EASIEST)

If git is not installed on WSL2, install it:

```bash
sudo apt-get update
sudo apt-get install git
```

Then follow normal installation:
```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
./builder.sh
```

---

## 📥 Method 2: Download ZIP (No Git Needed)

### Step 1: Download from GitHub
1. Open browser and go to:
   https://github.com/Cyanexani/2520030184_053_558-OSSP

2. Click the green **Code** button (top right)

3. Click **Download ZIP**

4. A file named `2520030184_053_558-OSSP-main.zip` downloads

### Step 2: Move to WSL2

**Option A: Using Windows (Easiest)**
```bash
# In WSL2 terminal, go to Windows Downloads folder
cd /mnt/c/Users/YourUsername/Downloads

# Check if ZIP is there
ls -lh *.zip

# You should see: 2520030184_053_558-OSSP-main.zip
```

**Option B: Move file to WSL2 home**
```bash
# Copy from Downloads to WSL2 home folder
cp /mnt/c/Users/YourUsername/Downloads/2520030184_053_558-OSSP-main.zip ~

# Go to home folder
cd ~
```

### Step 3: Extract ZIP

```bash
# Extract the ZIP file
unzip 2520030184_053_558-OSSP-main.zip

# Enter the folder
cd 2520030184_053_558-OSSP-main

# Make builder executable
chmod +x builder.sh

# Run builder
./builder.sh
```

---

## 🎯 Complete Step-by-Step for WSL2 (No Git)

### Step 1: Download ZIP on Windows
- Visit: https://github.com/Cyanexani/2520030184_053_558-OSSP
- Click **Code** → **Download ZIP**
- Wait for download to finish

### Step 2: Open WSL2 Terminal
```bash
# Open Windows Terminal or PowerShell
# Type: wsl
# You're now in WSL2
```

### Step 3: Navigate to Downloads
```bash
cd /mnt/c/Users/YourUsername/Downloads

# Replace "YourUsername" with your actual Windows username
# Example: cd /mnt/c/Users/john/Downloads
```

### Step 4: Verify ZIP is There
```bash
ls -lh *.zip
```

Should show something like:
```
-rw-r--r-- 1 user user 245K Aug 26 2024 2520030184_053_558-OSSP-main.zip
```

### Step 5: Extract ZIP
```bash
unzip 2520030184_053_558-OSSP-main.zip
```

### Step 6: Enter Folder
```bash
cd 2520030184_053_558-OSSP-main
```

### Step 7: Make Builder Executable
```bash
chmod +x builder.sh
```

### Step 8: Install Dependencies
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev
```

### Step 9: Build Project
```bash
./builder.sh
```

### Step 10: Run Monitor
```bash
./builder.sh --run
```

---

## 🛠️ If unzip Command Not Found

```bash
# Install unzip
sudo apt-get install unzip

# Then try again
unzip 2520030184_053_558-OSSP-main.zip
```

---

## 🔄 Alternative: Copy from Windows to WSL2

### If you have the ZIP file in Windows already:

**Terminal (PowerShell as Admin):**
```powershell
# On Windows (PowerShell)
$wslHome = "\\wsl$\Ubuntu\home\$env:USERNAME"
Copy-Item "C:\Users\YourUsername\Downloads\2520030184_053_558-OSSP-main.zip" $wslHome
```

**Then in WSL2:**
```bash
unzip 2520030184_053_558-OSSP-main.zip
cd 2520030184_053_558-OSSP-main
./builder.sh
```

---

## ⚠️ Common WSL2 Issues

### "Permission denied: builder.sh"
```bash
chmod +x builder.sh
./builder.sh
```

### "unzip: command not found"
```bash
sudo apt-get install unzip
```

### "No space left on device"
Your WSL2 storage is full. Check disk:
```bash
df -h

# Clean up
sudo apt-get clean
sudo apt-get autoclean
```

### "sudo: apt-get: command not found"
Your WSL2 doesn't have apt. Try:
```bash
sudo apk update
sudo apk add build-base ncurses-dev
```

Or reinstall WSL2 Ubuntu:
```powershell
# On Windows PowerShell (Admin)
wsl --unregister Ubuntu
wsl --install Ubuntu
```

---

## ✅ Verify WSL2 is Ready

Before installing Kernel Monitor, check:

```bash
# Check WSL2 version
wsl --version

# Check Ubuntu version
lsb_release -a

# Check gcc installed
gcc --version

# Check make installed
make --version

# Check ncurses
ldconfig -p | grep ncurses
```

---

## 📁 Folder Structure After Extraction

After extracting ZIP, you should have:

```
2520030184_053_558-OSSP-main/
├── builder.sh          ← Run this!
├── Makefile
├── README.md
├── INSTALL_SIMPLE.md
├── src/
├── tests/
└── docs/
```

---

## 🚀 Quick Commands for WSL2

```bash
# Go to Downloads
cd /mnt/c/Users/YourUsername/Downloads

# Extract
unzip 2520030184_053_558-OSSP-main.zip

# Enter folder
cd 2520030184_053_558-OSSP-main

# Make executable
chmod +x builder.sh

# Build (installs dependencies automatically)
./builder.sh

# Run
./builder.sh --run
```

---

## 🎯 WSL2 + GitHub Desktop (Alternative)

If you have GitHub Desktop on Windows:

1. Open GitHub Desktop
2. Click **File** → **Clone Repository**
3. Search for: `Cyanexani/2520030184_053_558-OSSP`
4. Click **Clone**
5. Choose local path
6. Open in Terminal
7. Run: `./builder.sh`

---

## 📝 Path Reference for Common Users

| Windows User | WSL2 Path |
|--------------|-----------|
| C:\Users\john | /mnt/c/Users/john |
| Downloads folder | /mnt/c/Users/john/Downloads |
| Documents folder | /mnt/c/Users/john/Documents |

---

## ✨ After Installation

```bash
# Run monitor
./bin/kernel-monitor

# Or use builder
./builder.sh --run

# View documentation
cat README.md
cat INSTALL_SIMPLE.md

# Run tests
cd tests && ./run_tests.sh
```

---

## 🆘 Still Having Issues?

### Check WSL2 is Enabled
```powershell
# On Windows PowerShell (Admin)
wsl -l -v
```

Should show something like:
```
NAME      STATE           VERSION
Ubuntu    Running         2
```

### Check WSL2 Storage
```bash
# In WSL2
df -h
# Should show ~20GB+ available
```

### Check Build Tools
```bash
g++ --version
make --version
gcc --version
```

If any are missing:
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev
```

---

## 🎉 Summary

**Without Git on WSL2:**

1. ✅ Download ZIP from GitHub website
2. ✅ Extract in WSL2
3. ✅ Run `./builder.sh`
4. ✅ Done!

**All methods work exactly the same after extraction!**

---

**Your Kernel Monitor works perfectly on WSL2!** 🚀

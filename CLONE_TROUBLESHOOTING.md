# 🆘 Cannot Clone Repository - Troubleshooting

**If you're having trouble cloning the repository, here are solutions.**

---

## ❌ Common Clone Errors & Solutions

### Error 1: "Permission denied (publickey)"

**What it means:** SSH key is not set up for GitHub.

**Solution:**

#### Option A: Use HTTPS Instead (Easier)
```bash
# Use this instead of SSH
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
./builder.sh
```

#### Option B: Set Up SSH Key

**Step 1: Check if you have SSH key**
```bash
ls -la ~/.ssh/id_*.pub
```

**Step 2: If no key, generate one**
```bash
ssh-keygen -t ed25519 -C "your.email@gmail.com"
# Press Enter 3 times (no passphrase)
```

**Step 3: Copy your public key**
```bash
cat ~/.ssh/id_ed25519.pub
```

**Step 4: Add to GitHub**
- Go to: https://github.com/settings/keys
- Click: **New SSH key**
- Paste your key
- Click: **Add SSH key**

**Step 5: Test connection**
```bash
ssh -T git@github.com
# Should say: "Hi username! You've successfully authenticated..."
```

**Step 6: Try cloning again**
```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

---

### Error 2: "fatal: could not read Username..."

**What it means:** Git is asking for password but can't read it (usually in automated environments).

**Solution: Use Personal Access Token (PAT)**

**Step 1: Create GitHub Token**
- Go to: https://github.com/settings/tokens
- Click: **Generate new token**
- Select: `repo` scope
- Click: **Generate token**
- Copy the token (save it somewhere safe!)

**Step 2: Clone with token**
```bash
git clone https://YOUR_TOKEN@github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
```

Replace `YOUR_TOKEN` with your actual token.

---

### Error 3: "Repository not found"

**What it means:** Repository URL is wrong or not accessible.

**Solution:**

**Check the URL is correct:**
```bash
# This is the correct URL:
https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Or with SSH:
git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

**Try HTTPS version:**
```bash
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
```

---

### Error 4: "fatal: unable to access ... Could not resolve host"

**What it means:** No internet connection or DNS is down.

**Solution:**

**Check internet connection:**
```bash
ping google.com
```

If ping doesn't work:
- Check WiFi is connected
- Try restarting router
- Try using different DNS: `8.8.8.8` (Google)

**Check if GitHub is accessible:**
```bash
curl -I https://github.com
# Should show: HTTP/1.1 200 OK
```

---

### Error 5: "fatal: protocol error: expected flush, got..."

**What it means:** Network issue or corrupted connection.

**Solution:**

**Try again:**
```bash
rm -rf 2520030184_053_558-OSSP  # Remove partial clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
```

**Or try SSH:**
```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

---

## ✅ If Cloning Doesn't Work - Use ZIP Instead

**This ALWAYS works:**

### Step 1: Visit GitHub
https://github.com/Cyanexani/2520030184_053_558-OSSP

### Step 2: Download ZIP
- Click green **Code** button
- Click **Download ZIP**
- Wait for download

### Step 3: Extract ZIP
```bash
# On Windows PowerShell or any terminal
unzip 2520030184_053_558-OSSP-main.zip
cd 2520030184_053_558-OSSP-main
```

### Step 4: Run Builder
```bash
chmod +x builder.sh
./builder.sh
```

**Done!** 🎉

---

## 🔧 Step-by-Step Cloning (HTTPS - Most Reliable)

```bash
# Step 1: Check git is installed
git --version

# Step 2: Check internet connection
ping github.com

# Step 3: Clone with HTTPS (most reliable)
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Step 4: Enter folder
cd 2520030184_053_558-OSSP

# Step 5: Make builder executable
chmod +x builder.sh

# Step 6: Build
./builder.sh

# Step 7: Run
./builder.sh --run
```

---

## 📊 Comparison: Clone vs ZIP

| Method | Pros | Cons |
|--------|------|------|
| **Clone (HTTPS)** | Easier updates | Needs credentials |
| **Clone (SSH)** | More secure | SSH key setup |
| **Download ZIP** | ✅ Always works | ✅ No setup needed |

---

## 🎯 Recommended Method

### If You Just Want It Working

```bash
# Download ZIP - No complications!
# Go to: https://github.com/Cyanexani/2520030184_053_558-OSSP
# Click Code → Download ZIP
# Extract and run: ./builder.sh
```

### If You Want to Use Git

```bash
# Use HTTPS - More reliable than SSH
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
./builder.sh
```

---

## 🆘 Diagnostic Commands

**Check what's wrong:**

```bash
# Is git installed?
git --version

# Can you reach GitHub?
ping github.com

# Can you access HTTPS?
curl -I https://github.com

# What's your current network?
ifconfig
# or
ip addr

# Are you behind a proxy?
echo $HTTP_PROXY
echo $HTTPS_PROXY
```

---

## 🔐 Git Credential Caching

**If you keep getting asked for password:**

### Store credentials (safer)
```bash
git config --global credential.helper store
# Then clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
# Enter username/password once
# It will be remembered
```

### Or use token caching
```bash
git config --global credential.helper cache
# Remembers for 15 minutes
```

---

## 🌐 Behind Corporate Proxy?

**If your company uses a proxy:**

```bash
# Set proxy temporarily
git config --global http.proxy [user:password@]proxyhost[:port]

# Example:
git config --global http.proxy http://proxy.example.com:8080

# Then clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Remove proxy after
git config --global --unset http.proxy
```

---

## 📱 Different Operating Systems

### Linux/Mac
```bash
# HTTPS (recommended)
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Or SSH
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

### Windows (PowerShell)
```powershell
# HTTPS (recommended)
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Or SSH
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

### WSL2
```bash
# Same as Linux
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Or see WSL2_NO_GIT.md
```

---

## 🎯 QUICKEST SOLUTION

**If cloning is giving you problems, use ZIP:**

```bash
# 1. Download ZIP from:
# https://github.com/Cyanexani/2520030184_053_558-OSSP

# 2. Extract it

# 3. Open terminal in extracted folder

# 4. Run:
./builder.sh
```

**That's it! No git needed!** ✨

---

## 📞 Still Can't Clone?

### Try These URLs in Order

```bash
# URL 1: HTTPS (most reliable)
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# URL 2: SSH
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git

# URL 3: If neither works, use ZIP
# https://github.com/Cyanexani/2520030184_053_558-OSSP/archive/main.zip
```

---

## ✅ Verification

**After cloning (or extracting ZIP), verify it worked:**

```bash
cd 2520030184_053_558-OSSP

# Should see these files:
ls -la | grep -E "builder|Makefile|README|src"

# Should output something like:
# builder.sh
# Makefile
# README.md
# src/
```

If you see these files, you're good to go!

```bash
./builder.sh
```

---

**Still stuck? Use the ZIP method - it ALWAYS works!** 🚀

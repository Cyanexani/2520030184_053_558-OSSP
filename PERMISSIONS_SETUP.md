# 🔐 Fix Repository Access Permissions from Another PC

**If you can't clone/push from a different computer, this guide will fix it.**

---

## ❌ Common Permission Errors

### Error 1: "Permission denied (publickey)"

**What it means:** The new PC doesn't have SSH keys for GitHub.

**Solution: Use HTTPS Instead (Easiest)**

```bash
# Use this instead of SSH
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
```

---

### Error 2: "fatal: could not read Username for github.com"

**What it means:** Git is asking for credentials but can't authenticate.

**Solution: Set Up Git Credentials**

```bash
# Method 1: Cache credentials (simplest)
git config --global credential.helper cache

# Then clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# It will ask for username/password once, then remember for 15 minutes

# Method 2: Store credentials permanently (more secure)
git config --global credential.helper store

# Then clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# It will ask for username/password once, then remember permanently
```

---

### Error 3: "Repository not found"

**What it means:** Wrong URL or repository doesn't exist.

**Check the URL:**

```bash
# This is correct:
https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# NOT this:
https://github.com/Cyanexani/2520030184_053_558-OSSP (missing .git)
```

---

## ✅ QUICK FIX FOR NEW PC (4 Steps)

### Step 1: Install Git

**Ubuntu/Debian:**
```bash
sudo apt-get install git
```

**Fedora:**
```bash
sudo dnf install git
```

**Arch:**
```bash
sudo pacman -S git
```

**macOS:**
```bash
# Install from https://git-scm.com/download/mac
# Or use Homebrew:
brew install git
```

**Windows:**
```
Download from: https://git-scm.com/download/win
```

### Step 2: Configure Git (First Time Only)

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@gmail.com"
```

### Step 3: Set Up Credentials (Choose One)

**Option A: HTTPS with cache (Recommended)**
```bash
git config --global credential.helper cache
```

**Option B: HTTPS with permanent storage**
```bash
git config --global credential.helper store
```

**Option C: SSH Key (More complex but more secure)**
```bash
# Skip to "SSH Setup" section below
```

### Step 4: Clone the Repository

```bash
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
```

**It will ask for:**
- Username: `Cyanexani`
- Password: Your GitHub password (or Personal Access Token)

---

## 🔑 SSH Setup (For Advanced Users)

**If you want to use SSH keys instead:**

### Step 1: Generate SSH Key on New PC

```bash
ssh-keygen -t ed25519 -C "your.email@gmail.com"
# Press Enter 3 times (no passphrase)
```

### Step 2: Copy Your Public Key

```bash
cat ~/.ssh/id_ed25519.pub
# Copy the entire output
```

### Step 3: Add to GitHub

1. Go to: https://github.com/settings/keys
2. Click: **New SSH key**
3. Paste your key
4. Click: **Add SSH key**

### Step 4: Test SSH Connection

```bash
ssh -T git@github.com
# Should say: "Hi Cyanexani! You've successfully authenticated..."
```

### Step 5: Clone with SSH

```bash
git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP
```

---

## 🎯 SIMPLEST METHOD (Recommended for Most Users)

### On Your New PC:

```bash
# 1. Install git
sudo apt-get install git  # (or your distro's command)

# 2. Configure git
git config --global user.name "Your Name"
git config --global user.email "your.email@gmail.com"

# 3. Set up credential caching
git config --global credential.helper cache

# 4. Clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
cd 2520030184_053_558-OSSP

# 5. Enter credentials when prompted
# Username: Cyanexani
# Password: Your GitHub password or PAT
```

**Done!** 🎉

---

## 🆘 GitHub Personal Access Token (If Password Doesn't Work)

**GitHub no longer accepts passwords for HTTPS - use a Personal Access Token instead.**

### Step 1: Create Token

1. Go to: https://github.com/settings/tokens
2. Click: **Generate new token (classic)**
3. Check: **repo** (Full control of private repositories)
4. Click: **Generate token**
5. Copy the token (save it somewhere safe!)

### Step 2: Use Token as Password

```bash
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# When asked for password, paste your token
# Username: Cyanexani
# Password: ghp_xxxxxxxxxxxxx (your token)
```

---

## 📋 Troubleshooting Steps

### Check Git Installation

```bash
git --version
# Should show version number
```

### Check Network Connection

```bash
ping github.com
# Should show responses
```

### Check GitHub Accessibility

```bash
curl -I https://github.com
# Should show: HTTP/1.1 200 OK
```

### Test SSH (if using SSH)

```bash
ssh -T git@github.com
```

### Check Stored Credentials

```bash
# On macOS (Keychain):
security find-internet-password -s github.com

# On Windows (Credential Manager):
# Settings → Credential Manager → Windows Credentials
```

---

## ⚙️ Advanced: Multiple SSH Keys

**If you have multiple GitHub accounts:**

### Create separate SSH keys

```bash
ssh-keygen -t ed25519 -C "work@gmail.com" -f ~/.ssh/id_work
ssh-keygen -t ed25519 -C "personal@gmail.com" -f ~/.ssh/id_personal
```

### Configure ~/.ssh/config

```bash
cat >> ~/.ssh/config << 'EOF'
Host github-work
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_work

Host github-personal
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_personal
EOF
```

### Use in clone

```bash
# Work account
git clone git@github-work:Cyanexani/2520030184_053_558-OSSP.git

# Personal account
git clone git@github-personal:Cyanexani/2520030184_053_558-OSSP.git
```

---

## 🔄 Copy SSH Keys Between PCs

**If you want to use the same SSH key on multiple PCs:**

### On Original PC

```bash
# Copy your SSH keys
cat ~/.ssh/id_ed25519
cat ~/.ssh/id_ed25519.pub
```

### On New PC

```bash
# Create .ssh directory
mkdir -p ~/.ssh
chmod 700 ~/.ssh

# Create the keys (paste the content from above)
nano ~/.ssh/id_ed25519
# Paste private key, save

nano ~/.ssh/id_ed25519.pub
# Paste public key, save

# Fix permissions (IMPORTANT!)
chmod 600 ~/.ssh/id_ed25519
chmod 644 ~/.ssh/id_ed25519.pub

# Test
ssh -T git@github.com
```

---

## 📊 Comparison: SSH vs HTTPS

| Method | Pros | Cons | Security |
|--------|------|------|----------|
| **HTTPS + Cache** | Easy setup | Need password/token | Good |
| **HTTPS + Store** | Remember forever | Password stored | OK |
| **SSH** | One-time setup | Complex | Best |

**Recommended for most users: HTTPS + Cache** ✅

---

## ✅ Verify It Works

After setting up, test with:

```bash
# Clone
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Check contents
cd 2520030184_053_558-OSSP
ls -la

# Should show:
# builder.sh, README.md, src/, tests/, docs/, etc.
```

---

## 🎯 Quick Checklist

- [ ] Git installed? (`git --version`)
- [ ] Git configured? (`git config --global user.name`)
- [ ] Credentials set up? (`git config --global credential.helper`)
- [ ] GitHub accessible? (`ping github.com`)
- [ ] Can clone? (`git clone https://...`)

If all checked, you're good! ✅

---

## 📞 Still Having Issues?

### Check error message carefully

```bash
# Most errors start with:
# - "Permission denied" → SSH/auth issue
# - "Repository not found" → URL wrong
# - "Could not resolve host" → Network issue
# - "fatal: authentication" → Credentials wrong
```

### Try HTTPS if SSH fails

```bash
# If SSH doesn't work:
git clone https://github.com/Cyanexani/2520030184_053_558-OSSP.git
```

### Check GitHub status

If nothing works, GitHub might be down:
https://www.githubstatus.com

---

**Now you should be able to access the repo from any PC!** 🚀

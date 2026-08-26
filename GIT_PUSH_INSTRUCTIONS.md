# Git Push Instructions

## Push Kernel Monitor to GitHub

Your repository: `git@github.com:Cyanexani/2520030184_053_558-OSSP.git`

---

## Quick Commands

```bash
# Navigate to project directory
cd /home/cyanexani/kernel-monitor

# Initialize git repository
git init

# Add remote repository
git remote add origin git@github.com:Cyanexani/2520030184_053_558-OSSP.git

# Add all files
git add .

# Commit with message
git commit -m "Initial commit: Complete Kernel Monitor implementation

- User-space Linux system monitoring application
- Real-time CPU and memory monitoring
- Process monitoring and control
- Process tree visualization
- Signal-based process control (SIGTERM, SIGKILL, SIGSTOP, SIGCONT)
- Terminal UI with ncurses
- Comprehensive documentation
- Test suite included
- Built with C++17 and Linux/POSIX APIs"

# Push to GitHub (main branch)
git branch -M main
git push -u origin main
```

---

## Alternative: Step by Step

### 1. Initialize Repository
```bash
cd /home/cyanexani/kernel-monitor
git init
```

### 2. Configure Git (if not already done)
```bash
git config user.name "Your Name"
git config user.email "your.email@example.com"
```

### 3. Add Remote
```bash
git remote add origin git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

### 4. Check Status
```bash
git status
# Should show all your untracked files
```

### 5. Add Files
```bash
# Add all files
git add .

# Or add selectively
git add src/ tests/ docs/ *.md Makefile
```

### 6. Commit
```bash
git commit -m "Initial commit: Complete Kernel Monitor project"
```

### 7. Push
```bash
# Set main as default branch and push
git branch -M main
git push -u origin main
```

---

## If Repository Already Exists on GitHub

If the repository already has content:

```bash
# Pull first to merge
git pull origin main --allow-unrelated-histories

# Then push
git push origin main
```

Or force push (⚠️ will overwrite remote):
```bash
git push -f origin main
```

---

## SSH Key Setup (if needed)

If you get "Permission denied" error, you need to set up SSH key:

### Check if SSH key exists
```bash
ls -la ~/.ssh/id_*.pub
```

### Generate new SSH key (if needed)
```bash
ssh-keygen -t ed25519 -C "your.email@example.com"
# Press Enter for default location
# Press Enter for no passphrase (or set one)
```

### Copy public key
```bash
cat ~/.ssh/id_ed25519.pub
# Copy the output
```

### Add to GitHub
1. Go to GitHub.com
2. Click your profile → Settings
3. SSH and GPG keys → New SSH key
4. Paste your public key
5. Save

### Test connection
```bash
ssh -T git@github.com
# Should say: "Hi username! You've successfully authenticated..."
```

---

## Using HTTPS Instead of SSH

If you prefer HTTPS:

```bash
# Remove SSH remote
git remote remove origin

# Add HTTPS remote
git remote add origin https://github.com/Cyanexani/2520030184_053_558-OSSP.git

# Push (will ask for username/password or token)
git push -u origin main
```

---

## Verify Push

After pushing, check:

1. Visit: https://github.com/Cyanexani/2520030184_053_558-OSSP
2. Verify all files are there
3. Check README.md renders properly

---

## Files That Will Be Pushed

```
kernel-monitor/
├── .gitignore                    ✓ Excludes build artifacts
├── Makefile                      ✓ Build system
├── README.md                     ✓ Main documentation
├── QUICKSTART.md                 ✓ Quick start guide
├── OVERVIEW.md                   ✓ Project overview
├── PROJECT_SUMMARY.md            ✓ Summary
├── CHECKLIST.md                  ✓ Checklist
├── build_and_verify.sh           ✓ Build script
├── src/
│   ├── main.cpp                  ✓ Source code
│   ├── ui/                       ✓ UI module
│   ├── proc/                     ✓ Proc reader
│   ├── process/                  ✓ Process monitor
│   ├── system/                   ✓ System monitor
│   ├── signals/                  ✓ Signal control
│   └── utils/                    ✓ Utilities
├── tests/
│   ├── run_tests.sh              ✓ Test suite
│   └── TEST_CASES.md             ✓ Test documentation
└── docs/
    ├── PROJECT_DOCUMENTATION.md  ✓ Technical docs
    └── PRESENTATION.md           ✓ Presentation
```

**Files that won't be pushed (in .gitignore):**
- build/ - Build artifacts
- bin/ - Compiled executables
- *.o - Object files

---

## Common Issues

### "fatal: not a git repository"
```bash
git init
```

### "remote origin already exists"
```bash
git remote remove origin
git remote add origin git@github.com:Cyanexani/2520030184_053_558-OSSP.git
```

### "Permission denied (publickey)"
Set up SSH key (see section above)

### "failed to push some refs"
```bash
git pull origin main --allow-unrelated-histories
git push origin main
```

### "Nothing to commit"
```bash
git add .
git commit -m "Your message"
```

---

## After Successful Push

1. ✅ Visit your repository on GitHub
2. ✅ Check if README.md is displaying
3. ✅ Verify all folders are present
4. ✅ Test clone on another machine:
   ```bash
   git clone git@github.com:Cyanexani/2520030184_053_558-OSSP.git
   cd 2520030184_053_558-OSSP
   make
   ```

---

## Quick One-Liner

```bash
cd /home/cyanexani/kernel-monitor && git init && git remote add origin git@github.com:Cyanexani/2520030184_053_558-OSSP.git && git add . && git commit -m "Initial commit: Complete Kernel Monitor" && git branch -M main && git push -u origin main
```

---

## Need Help?

Run these diagnostic commands:

```bash
# Check git version
git --version

# Check current directory
pwd

# Check files
ls -la

# Check git status
git status

# Check remotes
git remote -v

# Check SSH connection
ssh -T git@github.com
```

---

**Ready to push! Copy the commands above and run them in your terminal.** 🚀

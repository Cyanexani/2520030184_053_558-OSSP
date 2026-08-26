# ▶️ HOW TO RUN - SUPER SIMPLE

**After installation, here's how to run Kernel Monitor.**

---

## 🎯 THE ABSOLUTE SIMPLEST WAY

After `./builder.sh` finishes, just type:

```bash
./builder.sh --run
```

**Press Q to quit.**

Done! 🎉

---

## 3 Ways to Run

### Way 1: Using Builder (Easiest)
```bash
./builder.sh --run
```

### Way 2: Direct Executable
```bash
./bin/kernel-monitor
```

### Way 3: If System-Wide Installed
```bash
kernel-monitor
```

---

## 🎮 That's It!

You should see:
```
CPU: XX%  RAM: XX%  SWAP: X%
┌─────────────────────────────┐
│ PID  PROCESS  CPU%  MEM% ... │
│ ...                         │
└─────────────────────────────┘
```

**Press Q to exit.**

---

## 🆘 If It Doesn't Show

**Check if executable exists:**
```bash
ls bin/kernel-monitor
```

If not found, rebuild:
```bash
make clean
make
./bin/kernel-monitor
```

---

**That's all you need to know!** 🚀

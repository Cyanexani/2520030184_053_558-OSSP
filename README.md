# Kernel Monitor

A user-space Linux systems programming application that monitors system and process
state in real time through the `/proc` filesystem and POSIX APIs. No kernel module,
no driver, no root.

## Quick start

One command, which downloads, installs dependencies, builds and runs:

```bash
curl -fsSL https://raw.githubusercontent.com/Cyanexani/2520030184_053_558-OSSP/main/install.sh | bash
```

Then, from the install directory:

```bash
./bin/kernel-monitor
```

Press `Q` to quit.

## Keyboard controls

| Key | Action |
|-----|--------|
| `Q` | Quit |
| `↑` `↓` | Move the selection |
| `Page Up` `Page Down` | Move the selection ten rows |
| `Enter` | Open process details |
| `T` | Process tree view |
| `E` | Event log view |
| `B` or `Esc` | Back to the process list |
| `R` | Refresh immediately |
| `+` `-` | Change the update interval, 500 ms to 10 s |
| `K` | Terminate the selected process, `SIGTERM` |
| `X` | Kill the selected process, `SIGKILL` |
| `S` | Suspend the selected process, `SIGSTOP` |
| `C` | Resume the selected process, `SIGCONT` |

`K`, `X` and `S` ask for confirmation first. The selection follows the process it is
on, not the row number, so a process climbing the CPU list cannot slide out from
under the highlight between the keypress and the confirmation.

## Features

**System monitoring.** CPU usage, memory and swap, load averages, uptime, kernel
version and CPU count.

**Process monitoring.** Live process list sorted by CPU with PID tie-breaking, a
detail view per process, a process tree rebuilt from every process's PPID, and an
event log of process creation and termination retaining the last 100 entries.

**Process control.** `SIGTERM`, `SIGKILL`, `SIGSTOP` and `SIGCONT` through `kill()`,
with confirmation prompts and permission error reporting.

**Interface.** ncurses, four views, colour-coded, keyboard driven. Redraws only the
cells that changed rather than repainting the screen each tick.

## Requirements

- Linux, any distribution. Also runs under WSL2, with the caveat noted below.
- `build-essential` and `libncurses-dev`
- `gcc` with C11 support (the build uses `-std=gnu11`)

The UI links against the **wide-character** ncurses library, `-lncursesw`, and calls
`setlocale()` before `initscr()`. Both are required: the interface draws multibyte
glyphs for the arrow keys and the tree branches, and the 8-bit library renders one
broken cell per byte instead. On Debian and Ubuntu `libncurses-dev` provides it; on
Fedora and RHEL install `ncurses-devel`.

The installer checks for it and fails with a readable message rather than a linker
error if it is missing.

## Building

```bash
make            # builds the monitor and both demo programs
make demo       # builds only the demo programs
make clean      # removes build artefacts
make debug      # unoptimised build with symbols
make run        # build, then run
sudo make install
```

## Demo programs

Two small POSIX programs that create a zombie and an orphan on purpose, so both
conditions can be observed in `ps` and in the monitor's own `STATE` column.

```bash
./bin/zombie 30     # holds a zombie for 30 seconds, then reaps it
./bin/orphan 25     # parent exits after 3 seconds, child reports its new PPID
```

While the zombie is held:

```bash
ps -eo pid,ppid,stat,comm | grep -w Z
```

`STAT` shows `Z` and `ps -ef` renders the command as `<defunct>`.

A zombie is dead with a live parent that has not called `wait()`. An orphan is alive
with a dead parent. The zombie is the defect, because those PIDs are never released;
the orphan is handled by the kernel automatically.

See `demo/README.md` for the full walkthrough, including why an orphan under WSL is
re-parented to a subreaper rather than to PID 1.

## How it works

Each refresh cycle reads `/proc`, parses the fields, computes rates, updates the
process list and tree, detects creation and exit events, and redraws the changed
cells. The default interval is 2 seconds, adjustable at runtime.

Files read:

| Path | Purpose |
|------|---------|
| `/proc/stat` | Aggregate CPU jiffy counters |
| `/proc/meminfo` | Memory and swap totals |
| `/proc/uptime` | System uptime |
| `/proc/loadavg` | Load averages |
| `/proc/cpuinfo` | CPU model and core enumeration |
| `/proc/version` | Kernel version string |
| `/proc/[pid]/stat` | Per-process state, CPU time, threads, priority |
| `/proc/[pid]/statm` | Per-process memory |
| `/proc/[pid]/status` | Per-process detail including PPID |
| `/proc/[pid]/fd/` | Open file descriptor count |

CPU percentage is a rate, computed as the difference between two timed samples of a
process's `utime` plus `stime`, divided by the **measured** interval between them
rather than an assumed one. A single-threaded process therefore reads at most 100%,
and the reading does not change when the update interval does.

## Linux and POSIX APIs used

| API | Purpose |
|-----|---------|
| `open()` `read()` `close()` | Read `/proc` files |
| `opendir()` `readdir()` `closedir()` | Enumerate numeric `/proc` entries as PIDs |
| `kill()` | Deliver a signal to a target PID |
| `sigaction()` | Install the monitor's own `SIGINT` and `SIGTERM` handler |
| `sysconf()` | Clock ticks per second and page size |
| `sysinfo()` | Uptime and total memory |
| `fork()` `wait()` `waitpid()` `getppid()` | Used by the demo programs |

The monitor handles signals as well as sending them. Its handler writes a
`volatile sig_atomic_t` flag and returns immediately, because a handler can interrupt
at any machine instruction and the flag must be written atomically and never cached
in a register.

## Project structure

```
src/
├── main.c              # entry point, signal handlers, the refresh loop
├── proc/               # raw /proc access
├── parser (utils/)     # turns /proc text into values
├── system/             # machine-wide CPU, memory, load
├── process/            # per-process model, tree, events
├── signals/            # the only module that calls kill()
└── ui/                 # ncurses rendering and input

demo/                   # zombie and orphan demonstration programs
tests/                  # automated checks and documented test cases
docs/                   # technical documentation and presentation notes
```

## Measured performance

Taken from the running binary under four busy-loop processes, inside WSL2 on a
12-CPU machine.

| Metric | Value |
|--------|-------|
| Own CPU usage | 0.40%, sampled over 10 s |
| Resident memory | 5.1 MB, 7.4 MB virtual |
| `/proc` scan, mean | 3.25 ms across 43 processes |
| `/proc` scan, worst of 20 runs | 11.4 ms |
| Threads / open descriptors | 1 / 3 |
| Binary size | 180 KB |

That scan cost divides to roughly **0.076 ms per process**, which extrapolates to
about 15 ms at 200 processes and 38 ms at 500. Those two figures are arithmetic, not
measurements: this environment runs 43 to 49 processes, so behaviour on a larger
machine has not been observed.

## Testing

```bash
cd tests
./run_tests.sh
```

Ten automated checks covering `/proc` availability, the ncurses library, the
compiler, the build, the produced executable, per-process files, signal operations,
statistics formats and the terminal environment. `tests/TEST_CASES.md` documents a
further eight categories of manual test cases.

## Running under WSL2

The project builds and runs normally under WSL2, but `/proc` there belongs to the
**WSL virtual machine**, not to Windows. The monitor shows Linux processes inside
WSL and cannot see Windows processes at all. A fresh WSL instance runs only around
40 to 50 processes, so start some workloads first if you want a populated list.

Orphan re-parenting also differs: WSL's `Relay` process registers as a child
subreaper, so an orphan is re-parented to it rather than to PID 1. See
`demo/README.md`.

## Educational scope

The project exercises the first three course outcomes of Operating Systems and
Systems Programming (25CS2104E):

- **CO-1**, the OS as a service layer: user space versus kernel space, system calls,
  and `/proc` as a kernel filesystem surfaced through the VFS.
- **CO-2**, processes and process control: the process abstraction, PID and PPID
  relationships, lifecycle and state transitions, user-level scheduling accounting,
  and both common pitfalls via the demo programs.
- **CO-3**, IPC and signals: asynchronous notification with `kill()`, POSIX signal
  handlers, and job control through `SIGSTOP` and `SIGCONT`.

Memory figures are reported as data the tool reads. Memory management as a subject
is CO-4 and is not claimed here.

## Other documentation

| Guide | Purpose |
|-------|---------|
| `HOW_TO_RUN.md` | Running it after installing |
| `INSTALL_SIMPLE.md` | Installation in three steps |
| `ONE_COMMAND.md` | The one-command installer |
| `BUILDER.md` | `builder.sh` reference |
| `QUICKSTART.md` | Keyboard reference |
| `WSL2_NO_GIT.md` | WSL2 without git |
| `demo/README.md` | Zombie and orphan walkthrough |
| `docs/PROJECT_DOCUMENTATION.md` | Technical documentation |

## Repository

https://github.com/Cyanexani/2520030184_053_558-OSSP

## License

Educational project for the Operating Systems and Systems Programming course.

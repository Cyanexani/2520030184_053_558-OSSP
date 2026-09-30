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

**Detail view.** Besides CPU, memory, threads, priority and descriptors, each process
shows its process group, session, controlling terminal and whether its group is the
terminal's foreground job, plus its minor and major page fault counts with live
rates per second.

**Two threads.** A collector thread reads `/proc` while the main thread handles the
keyboard and drawing, so a slow scan never delays a keypress. The header shows how
long the last scan took.

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
make            # builds the monitor and all nine demo programs
make demo       # builds only the demo programs
make clean      # removes build artefacts
make debug      # unoptimised build with symbols
make run        # build, then run
sudo make install
```

## Demo programs

Nine small C programs in `demo/`, each a single file that narrates what it does.
Most are meant to be run next to the monitor, so the effect shows up in its views.

| Program | Course outcome | What it shows |
|---------|----------------|---------------|
| `zombie` | CO-2 | A child that has exited but was never reaped |
| `orphan` | CO-2 | A child whose parent exits first, and who adopts it |
| `busy` | CO-2 | A CPU-bound process, to watch state and CPU% |
| `pipeline` | CO-1, CO-3 | What a shell does for `a \| b`: `pipe`, `fork`, `dup2`, `execvp`, one process group |
| `fifo` | CO-3 | A named pipe between two processes, in one or two terminals |
| `jobctl` | CO-3 | Process groups, sessions, `SIGSTOP` and `SIGCONT` to a whole job, `setsid()` |
| `vmem` | CO-4 | Address space layout, demand paging, page tables, copy-on-write, `SIGSEGV`, planted bugs for Valgrind and ASan |
| `fileio` | CO-5 | VFS, ext4, inodes and links, descriptors and `dup`, buffered versus unbuffered, `mmap` of a file |
| `threadsync` | CO-6 | Threads versus processes, a race, mutex, atomics, condition variables, a semaphore, deadlock, read-write lock, barrier |

```bash
./bin/zombie 30       # holds a zombie for 30 seconds, then reaps it
./bin/jobctl 6        # six seconds per phase, time to watch it in the monitor
./bin/vmem            # the memory tour; sudo ./bin/vmem also shows frame numbers
./bin/vmem leak       # run under valgrind or an ASan build to catch the leak
```

See `demo/README.md` for each program's walkthrough and the output to expect.

## How it works

Each refresh cycle reads `/proc`, parses the fields, computes rates, updates the
process list and tree, detects creation and exit events, and redraws the changed
cells. The default interval is 2 seconds, adjustable at runtime.

The work is split across two threads. The collector thread sleeps on a condition
variable until the interval runs out, or until `R` or `+` `-` wakes it early. It
builds the next sample **without** holding the lock, then takes the mutex only for
the moment it takes to swap the new sample in. The main thread takes the same mutex
only to draw. Signal delivery is steered to the main thread by blocking every signal
in the collector before it starts. There is a single lock, so no lock ordering
deadlock is possible, and no thread ever holds it while waiting for a key.

Files read:

| Path | Purpose |
|------|---------|
| `/proc/stat` | Aggregate CPU jiffy counters |
| `/proc/meminfo` | Memory and swap totals |
| `/proc/uptime` | System uptime |
| `/proc/loadavg` | Load averages |
| `/proc/cpuinfo` | CPU model and core enumeration |
| `/proc/version` | Kernel version string |
| `/proc/[pid]/stat` | Per-process state, CPU time, threads, priority, process group, session, terminal, page faults |
| `/proc/[pid]/statm` | Per-process memory |
| `/proc/[pid]/status` | Per-process detail including PPID |
| `/proc/[pid]/fd/` | Open file descriptor count |

CPU percentage is a rate, computed as the difference between two timed samples of a
process's `utime` plus `stime`, divided by the **measured** interval between them
rather than an assumed one. A single-threaded process therefore reads at most 100%,
and the reading does not change when the update interval does. Page fault rates are
computed the same way from the `minflt` and `majflt` counters.

## Linux and POSIX APIs used

| API | Purpose |
|-----|---------|
| `open()` `read()` `close()` | Read `/proc` files |
| `opendir()` `readdir()` `closedir()` | Enumerate numeric `/proc` entries as PIDs |
| `kill()` | Deliver a signal to a target PID |
| `sigaction()` | Install the monitor's own `SIGINT` and `SIGTERM` handler |
| `sysconf()` | Clock ticks per second and page size |
| `sysinfo()` | Uptime and total memory |
| `pthread_create()` `pthread_join()` | Start and stop the collector thread |
| `pthread_mutex_*` `pthread_cond_timedwait()` | Guard the shared sample, and sleep until the interval ends or a key wakes the collector |
| `pthread_condattr_setclock()` | Time the wait on `CLOCK_MONOTONIC`, immune to clock changes |
| `pthread_sigmask()` | Keep `SIGINT`, `SIGTERM` and `SIGWINCH` on the main thread |

The demo programs add these:

| API | Demo |
|-----|------|
| `fork()` `execvp()` `waitpid()` `getppid()` | zombie, orphan, pipeline, jobctl |
| `pipe()` `dup2()` | pipeline |
| `mkfifo()` | fifo |
| `setpgid()` `getpgrp()` `setsid()` `getsid()` `tcgetpgrp()` | pipeline, jobctl |
| `mmap()` `munmap()` `mprotect()` `madvise()` `msync()` | vmem, fileio |
| `getrusage()` `sigsetjmp()` `siglongjmp()` | vmem |
| `statfs()` `stat()` `lstat()` `link()` `symlink()` `unlink()` `dup()` `lseek()` `pread()` | fileio |
| `pthread_*` `sem_*` `stdatomic.h` | threadsync |

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
├── collector/          # the background thread that samples /proc
└── ui/                 # ncurses rendering and input

demo/                   # nine demonstration programs, one per concept
tests/                  # automated checks and documented test cases
docs/                   # technical documentation and presentation notes
```

## Measured performance

Taken from the running binary under four busy-loop processes, inside WSL2 on a
12-CPU machine.

| Metric | Value |
|--------|-------|
| Own CPU usage | 0.30%, sampled over 10 s |
| Resident memory | 3.6 MB, 77.5 MB virtual |
| `/proc` scan, mean | 2.82 ms across 37 processes |
| `/proc` scan, worst of 20 runs | 3.9 ms |
| Threads, open descriptors | 2 threads, 3 descriptors |
| Binary size | 60 KB |

That scan cost divides to roughly **0.076 ms per process**, which extrapolates to
about 15 ms at 200 processes and 38 ms at 500. Those two figures are arithmetic, not
measurements: this environment runs 37 to 49 processes, so behaviour on a larger
machine has not been observed.

The virtual size is large only because of the second thread. glibc reserves a 64 MB
malloc arena and an 8 MB stack for it, but reserving address space costs nothing
until a page is touched, which is why the resident figure stays at 3.6 MB. That is
demand paging, the same effect `./bin/vmem` demonstrates.

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

The project covers all six course outcomes of Operating Systems and Systems
Programming (25CS2104E). The source marks each place a concept is applied with a
`[CO-n]` comment.

- **CO-1**, the OS as a service layer: user space versus kernel space, system calls,
  `/proc` as a kernel filesystem surfaced through the VFS, and the command execution
  journey a shell follows (`pipeline`).
- **CO-2**, processes and process control: the process abstraction, PID and PPID
  relationships, lifecycle and state transitions, user-level scheduling accounting,
  and the zombie and orphan pitfalls (`zombie`, `orphan`, `busy`).
- **CO-3**, IPC, signals and job control: signals sent with `kill()` and handled with
  `sigaction()`, anonymous pipes (`pipeline`), named pipes (`fifo`), and process
  groups, sessions and job control, both in the detail view and in `jobctl`.
- **CO-4**, memory management: page fault counts and rates in the detail view, and
  the address space layout, demand paging, page tables, dynamic allocation,
  copy-on-write, `SIGSEGV` and memory debugging tools in `vmem`.
- **CO-5**, file systems and file I/O: the VFS, ext4, inodes and directory entries,
  descriptors and open file descriptions, buffered versus unbuffered I/O and
  memory-mapped files, all in `fileio`.
- **CO-6**, concurrency: the monitor's own collector thread, mutex and condition
  variable, plus races, atomics, semaphores, deadlock, read-write locks and
  barriers in `threadsync`.

## Other documentation

| Guide | Purpose |
|-------|---------|
| `HOW_TO_RUN.md` | Running it after installing |
| `INSTALL_SIMPLE.md` | Installation in three steps |
| `ONE_COMMAND.md` | The one-command installer |
| `BUILDER.md` | `builder.sh` reference |
| `QUICKSTART.md` | Keyboard reference |
| `WSL2_NO_GIT.md` | WSL2 without git |
| `demo/README.md` | Walkthrough of all nine demo programs |
| `docs/PROJECT_DOCUMENTATION.md` | Technical documentation |

## Repository

https://github.com/Cyanexani/2520030184_053_558-OSSP

## License

Educational project for the Operating Systems and Systems Programming course.

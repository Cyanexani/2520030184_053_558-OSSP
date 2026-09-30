# Demo Programs

Nine small C programs, each a single file that creates one operating system
concept on purpose and narrates what happens. Several are meant to be watched in
Kernel Monitor at the same time, in `ps`, or in `/proc`.

| Program | Course outcome | Concept |
|---------|----------------|---------|
| `zombie` | CO-2 | A terminated child nobody has reaped |
| `orphan` | CO-2 | A child that outlives its parent |
| `busy` | CO-2, CO-3 | A process that stays Running, and a target for signals |
| `pipeline` | CO-1, CO-3 | How a shell runs `ls \| grep` |
| `fifo` | CO-3 | Named pipes |
| `jobctl` | CO-3 | Process groups, sessions and job control |
| `vmem` | CO-4 | Virtual memory, paging, copy-on-write, memory errors and debugging |
| `fileio` | CO-5 | VFS, ext4, inodes, descriptors, buffering, memory-mapped files |
| `threadsync` | CO-6 | Threads, races, mutexes, condition variables, semaphores, deadlock |

Each source file starts with a comment that explains the concept, and every
place a syllabus topic is applied is tagged `[CO-n]`.

## Build

```bash
make demo        # or just `make`, which builds these too
```

Produces all nine in `bin/`. `threadsync` needs `-pthread`, which the Makefile
already passes.

## The two conditions

| | Zombie | Orphan |
|---|---|---|
| Child | has terminated | still running |
| Parent | still alive, has not called `wait()` | has terminated |
| Kernel holds | the exit status and the PID | nothing unusual |
| Resolved by | the parent calling `wait()` | re-parenting, automatically |
| Is it a bug? | yes, it leaks PIDs | no, the kernel handles it |

A zombie is dead with a live negligent parent. An orphan is alive with a dead
parent.

## Zombie

```bash
./bin/zombie 30
```

The child exits immediately; the parent deliberately sleeps for 30 seconds
before calling `waitpid()`. During that window the child is a zombie: its
memory, open files and threads are already released, but the kernel keeps the
process table entry because the exit status still has to be delivered.

Prove it from another terminal while it holds:

```bash
ps -eo pid,ppid,stat,comm | grep -w Z
ps -ef | grep defunct
```

`STAT` is `Z`, and `ps -ef` renders the command as `<defunct>`. In Kernel
Monitor the `STATE` column reads `Zombie` with 0.0% CPU and no memory of its
own. After the hold expires the parent reaps it and the entry disappears.

The failure this models is a parent that forks in a loop and never waits. Each
dead child keeps its PID forever, and a long-running server doing it will
eventually exhaust the process table.

## Orphan

```bash
./bin/orphan 25
```

The parent exits after 3 seconds while the child keeps running. The child
prints its `ppid` every second, so the re-parenting is visible as it happens.

### Expect a subreaper, not always PID 1

The textbook answer is that an orphan is re-parented to PID 1 (`init` or
`systemd`). What actually happens is that the kernel walks up the ancestry and
hands the orphan to the nearest ancestor that called
`prctl(PR_SET_CHILD_SUBREAPER)`, falling back to PID 1 only when there is none.

Under WSL the new parent is usually WSL's `Relay` process rather than PID 1,
because `Relay` registers as a subreaper. On a normal Linux install or a VM you
will see `ppid=1`. Press `T` in Kernel Monitor to see exactly where the orphan
landed in the tree.

Either way nothing leaks: the new parent reaps the orphan when it exits, so an
orphan never becomes a permanent zombie.

## busy: a process that stays Running

`zombie` and `orphan` both sit at 0% CPU, so neither demonstrates the Running
state or a live CPU reading. `busy` does.

```bash
./bin/busy            # runs until you stop it, 2 prints per second
./bin/busy 30         # stops itself after 30 seconds
./bin/busy 0 10       # runs until stopped, 10 prints per second
```

It prints its own PID on the first line, so there is no guesswork about which
row to watch. In Kernel Monitor its STATE reads `Running` and its CPU% sits near
100% of one core.

The throttling is deliberate. A loop that calls `printf()` as fast as it can
spends most of its life blocked on `write()`, so the kernel reports it as `S`,
sleeping on I/O, and the demonstration shows the opposite of what it means to.
`busy` burns a fixed slice of real CPU between prints instead.

### Using it to show the difference between SIGTERM and SIGKILL

`busy` installs a `sigaction` handler for `SIGINT` and `SIGTERM`, so:

- Press `K` in Kernel Monitor, or Ctrl+C. It catches the signal and exits with a
  summary line, because a catchable signal lets a process clean up first.
- Press `X` instead. `SIGKILL` cannot be caught, blocked or ignored, so the
  process disappears with no summary at all.

Running it once each way, and pointing at the missing summary line, is the
clearest demonstration of why those two signals are not interchangeable.

## pipeline: what the shell does for `ls | grep`

```bash
./bin/pipeline
```

This program plays the shell. It calls `pipe()` for a pair of descriptors,
forks twice, and in each child uses `dup2()` to put one end of the pipe on
stdout or stdin before calling `execvp()`. Both children join one new process
group, which is what makes the pipeline a single job. Expect output like:

```
shell   : pipe() gave read end fd 3 and write end fd 4
shell   : forked ls as pid 458 and grep as pid 459
shell   : both are in process group 458, mine is 321
shell   : closed my copies of both ends, so the reader can see EOF
```

The fourth line is the classic pitfall. `grep` only sees end-of-file when every
copy of the write end is closed, including the parent's. Forget to close it and
the pipeline hangs forever.

## fifo: a named pipe

```bash
./bin/fifo                 # one terminal: a child writes, the parent reads
./bin/fifo read            # two terminals: start the reader first
./bin/fifo write hello     # then the writer, in the other terminal
```

An anonymous pipe only works between relatives, because the descriptors are
inherited through `fork()`. `mkfifo()` gives the pipe a name in the file system,
`/tmp/km-demo.fifo`, so any two processes can meet there. `ls -l` shows it with a
leading `p` and a size of 0, since the data lives in a kernel buffer and never
touches the disk.

Watch the order of the first lines: each side's `open()` blocks until the other
side opens too. The reader ends when `read()` returns 0, which means every
writer has closed.

## jobctl: process groups, sessions and job control

```bash
./bin/jobctl         # one second per phase
./bin/jobctl 6       # six seconds per phase, enough time to watch the monitor
```

Three parts:

1. **Who am I.** Prints its pid, ppid, pgid and sid, decodes its controlling
   terminal from `/proc/self/stat`, and asks `tcgetpgrp()` whether it is in the
   foreground.
2. **A job.** Forks three workers into one new process group, then stops,
   resumes and ends the whole group with one `kill(-pgid, sig)` call each time.
   That is exactly what Ctrl+Z, `fg` and Ctrl+C do.
3. **A new session.** A child calls `setsid()`, becomes leader of a session with
   no controlling terminal, and shows that `open("/dev/tty")` now fails.

With `./bin/jobctl 6` running, open Kernel Monitor in another terminal and press
`Enter` on a worker. The detail view shows the workers' shared Process Group,
the Session and the Terminal, and reads `Foreground: no, group N has it`, where
N is `jobctl`'s own group. The STATE column of all three workers flips to
`Stopped` and back together. Start it with a trailing `&` and Part 1 reports
that it is in the background instead.

## vmem: virtual memory from inside one process

```bash
./bin/vmem                 # the tour
sudo ./bin/vmem            # the same, with physical frame numbers in Part 3
```

Five parts:

1. **Layout.** Addresses of code, `.data`, `.bss`, the `brk` heap, a large
   `malloc()` served by its own `mmap()`, libc and the stack, followed by
   `/proc/self/maps` so each address can be matched to its region.
2. **Demand paging.** `mmap()` of 64 MiB costs 0 faults and 0 KiB. Reading every
   page costs 16384 minor faults but still 0 KiB, because the kernel maps its
   shared zero page. Writing every page costs another 16384 faults and only now
   adds 65536 KiB of resident memory.
3. **Page tables.** Writes to pages 0, 3, 4 and 7 of an 8 page mapping, then
   reads `/proc/self/pagemap` to show exactly those four are present. Without
   root the frame numbers are hidden; with `sudo` they appear.
4. **Copy-on-write.** After `fork()` the child reads 4096 pages with 0 faults,
   then writes them with 4096 faults, one copy per page, while the parent still
   sees its original data.
5. **Memory errors.** A write to a page made read-only with `mprotect()` raises
   `SIGSEGV` with `SEGV_ACCERR`, and a write through `NULL` raises it with
   `SEGV_MAPERR`. A handler catches both and reports the faulting address.

### Memory debugging tools

Two modes plant bugs that the kernel never notices, because neither touches an
unmapped page. Both run to completion and print nothing wrong.

```bash
./bin/vmem leak            # 10 blocks of 100 bytes, never freed
./bin/vmem overflow        # 9 ints written into an array of 8
```

Build with AddressSanitizer to catch them:

```bash
gcc -g -fsanitize=address demo/vmem.c -o /tmp/vmem-asan
/tmp/vmem-asan leak        # LeakSanitizer: 1000 byte(s) leaked in 10 allocation(s)
/tmp/vmem-asan overflow    # heap-buffer-overflow, 0 bytes after 32-byte region
```

Valgrind finds the same two bugs with `valgrind --leak-check=full ./bin/vmem leak`
and `valgrind ./bin/vmem overflow`, after `sudo apt install valgrind`.

## fileio: file systems and file I/O

```bash
./bin/fileio
strace -c ./bin/fileio     # optional, counts the system calls in Part 5
```

Six parts:

1. **VFS.** The same `open()` and `read()` work on an ext4 file, a `/proc` file,
   a `/sys` file and a device. `statfs()` then names the file system behind each
   mount point by its magic number.
2. **ext4.** Block size, capacity, the fixed inode count, and the journal mode
   read from `/proc/fs/ext4`.
3. **Inodes and names.** A hard link shares the original's inode and raises the
   link count; a symlink has its own inode and only stores a path. After
   `unlink()` the hard link still reads the data and the symlink dangles. A file
   unlinked while open stays readable through its descriptor.
4. **Descriptors.** Two `open()` calls on one file get independent offsets, while
   `dup()` shares one. `/proc/self/fd` lists them.
5. **Buffering.** 200000 one-byte `write()` calls against 200000 `fputc()` calls.
   stdio sends the bytes in blocks of the file's `st_blksize`, so it makes about
   4000 times fewer system calls and runs roughly 200 times faster.
6. **mmap.** Maps a file, changes it through a pointer with no `write()` call,
   and reads the change back with `read()`.

It works in a private directory under `/tmp` and removes it at the end.

## threadsync: threads and synchronization

```bash
./bin/threadsync
```

Six parts:

1. **Threads versus processes.** A thread's write to a global is seen by `main`;
   a forked child's write is not.
2. **A race.** Four threads add 1 to a shared counter a million times each.
   Without a lock most updates are lost. A mutex and a C11 atomic both give the
   exact 4000000, and the timings show what each costs.
3. **Condition variables.** A bounded buffer with one producer and two
   consumers. The producer sleeps on a condition variable when the buffer is
   full instead of spinning.
4. **Counting semaphore.** Six threads share two permits, and never more than
   two are inside at once.
5. **Deadlock.** Two threads take two locks in opposite orders and each waits
   for the other. `pthread_mutex_timedlock()` lets both give up after a second
   instead of hanging. Taking the locks in one global order removes the cycle.
6. **Advanced tools.** A read-write lock lets three readers in together but a
   writer alone, and a barrier lines the threads up before they start.

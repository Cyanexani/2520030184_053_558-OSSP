# Zombie and Orphan Process Demos

Two small POSIX programs that create each condition deliberately, so the state
can be observed in `ps` and in Kernel Monitor's `STATE` column.

## Build

```bash
make demo        # or just `make`, which builds these too
```

Produces `bin/zombie` and `bin/orphan`.

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

# `dev/watchdog-orphans/`

One question about `ci-scripts/suite/mpiexec-filter`, answered by a process
count rather than by output: **does its watchdog outlive the test it guards?**

It did. `run.sh` is the measurement, and it discriminates — the same script over
the same three properties, before and after the one-line change:

| check | before | after |
|---|---|---|
| orphan watchdog `sleep`s after 5 fast runs | **5** | **0** |
| hung launcher: exit status | 143 | 143 |
| hung launcher: seconds to give up | 16 | 16 |
| hung launcher: ranks left behind | 0 | 0 |
| bytes of stdout+stderr from a fast run | 0 | 0 |
| fast run read to EOF: seconds | 0 | 0 |

```sh
dev/watchdog-orphans/run.sh                       # the committed filter
dev/watchdog-orphans/run.sh /path/to/older/copy   # e.g. git show <rev>:...
```

No MPI is involved: the launcher is a two-line stub, because the question is
about the filter's own process handling. It takes about 20 seconds, almost all
of it the deliberate 16-second wait in check 2.

## What leaked, and why one per test

The filter runs the launcher in a process group of its own and starts a watchdog
subshell beside it; when the launcher finishes first, the filter signals the
watchdog and moves on. The signal reached the *subshell*, not the `sleep` the
subshell was blocked in — bash's default `SIGTERM` disposition ends the shell
without touching its children — so every test that finished before its timeout
left one `sleep` behind, still counting down the full 195 seconds.

That is a leak that scales with the shard rather than a bounded one. The
openmpi/aarch64/rest leg of run 32887203728 ended with the runner reaping **304**
orphan `sleep` processes for **323** tests, which is the one-per-test rate this
script reproduces at small scale.

The fix is the same device the filter already used for the launcher one line
above: give the watchdog its own process group and signal the *group*.

## Why the other three checks are in here

The leak is easy to close in a way that quietly breaks something the watchdog
exists for, and two of those breakages are invisible in a green run:

- **Check 2** is the watchdog's whole purpose. Killing the subshell's group
  earlier, or trapping the signal wrongly, gives a filter that never fires and a
  hang that costs the shard its 180-second limit instead of being cut short.
- **Check 3** is the property the redirections in that script protect, and its
  comment records the measurement that found it: `runtests` reads the filter's
  stdout to EOF *before* looking at the exit status, so a watchdog holding the
  write end makes every test appear to take exactly the timeout — three tests at
  195 seconds each, all of them passing. A "fix" that drops `>/dev/null` from
  the subshell reintroduces that, and the suite still goes green while taking
  hours.
- **`set -m` has to stay quiet.** Job control in a non-interactive shell can
  print `[1] 12345` to stderr, and the suite counts stray output as a test
  failure -- so a fix for a leak nobody sees must not buy it with output
  everybody sees. **Check 4** is that check, and its bar is no bytes at all,
  since nothing else in a fast run writes any. It passes on this laptop's bash
  5.3, which is the only bash it has been run under; the runners are not measured
  here, which is what the check is for.

## What would retire this directory

Nothing pending. Unlike the other `dev/` measurements this one is not waiting on
an upstream fix or an open question — it is a regression test for a change
already made, kept because the three properties above are cheap to check and
expensive to notice breaking.

#!/usr/bin/env python3
"""Where a suite shard's wall clock went, read out of its own TAP file.

    dev/suite-timings/tap-timings.py summary.tap [summary.tap ...]

MPICH's runtests writes `# time=<seconds>` on every TAP line it produces, so a
shard's cost decomposes exactly, with no instrumentation and no re-running. What
this exists to surface is the share spent at the *time limit*: a hang costs the
full limit every run, and a shard can be almost entirely made of them without
anything in the log saying so. That is how run 32586710591 came to spend 83% of
the workflow's critical path on seventeen already-expected failures --
ci-scripts/suite/timelimit-ci-openmpi.txt is what that measurement produced.

Artifacts come from a run with:

    gh run download <run-id> -n suite-<mpi>-<arch>-<shard> -D /tmp/shard

**A test's limit is read from its own TAP entry, not guessed from the file.**
runtests writes `Timeout: N` into the YAML block of every failure, and since
ci-scripts/suite/timelimit-ci-openmpi.txt caps individual lines, one shard now
holds several different limits at once. An earlier version of this script called
a test "at the limit" if it reached 95% of the slowest time in the file, which
in a mixed run reported 1 test at the limit where there were 15: the single
uncapped 180-second hang set the bar and hid every 30-second one under it.
"""
import re, sys
from collections import defaultdict

RESULT = re.compile(r'^(not )?ok \d+ - \./(\S+) (\d+)(?: # TODO [^#]*)? # time=([0-9.]+)')
TIMEOUT = re.compile(r'^\s+Timeout: (\d+)\s*$')


def parse(path):
    """[(failed, name, np, seconds, limit_or_None)] in file order."""
    tests = []
    for line in open(path, encoding='utf-8', errors='replace'):
        m = RESULT.match(line)
        if m:
            tests.append([m.group(1) is not None, m.group(2),
                          int(m.group(3)), float(m.group(4)), None])
            continue
        m = TIMEOUT.match(line)
        if m and tests and tests[-1][4] is None:
            # The YAML block belongs to the result line above it. Only a failure
            # gets one, which is all this needs: a passing test is not at its limit.
            tests[-1][4] = int(m.group(1))
    return tests


def report(path):
    tests = parse(path)
    if not tests:
        print(f'{path}: no timed TAP lines'); return
    total = sum(t[3] for t in tests)
    # At the limit: a failure that used essentially all of the limit its own TAP
    # entry records. Falls back to the slowest time in the file only when no entry
    # carries a Timeout at all, which means nothing failed.
    limits = sorted({t[4] for t in tests if t[4]})
    at_limit = [t for t in tests if t[4] and t[3] >= 0.95 * t[4]]
    if not limits:
        slowest = max(t[3] for t in tests)
        at_limit = [t for t in tests if t[3] >= 0.95 * slowest]
    limit_cost = sum(t[3] for t in at_limit)

    by_dir = defaultdict(lambda: [0.0, 0])
    for _, name, _, secs, _ in tests:
        d = by_dir[name.split('/')[0]]
        d[0] += secs; d[1] += 1

    print(f'\n{path}')
    print(f'  {len(tests)} timed tests, {sum(1 for t in tests if t[0])} failed, '
          f'{total/60:.1f} min total')
    if limits:
        print(f'  time limits in force: {", ".join(f"{s}s" for s in limits)}')
    for d, (secs, n) in sorted(by_dir.items(), key=lambda kv: -kv[1][0]):
        print(f'    {d:<12} {secs/60:6.1f} min  {n:4d} tests')
    if at_limit:
        print(f'  at the limit: {len(at_limit)} tests, {limit_cost/60:.1f} min '
              f'= {100*limit_cost/total:.0f}% of the shard')
        for failed, name, np, secs, limit in sorted(at_limit, key=lambda t: t[1]):
            print(f'    {name:<40} np={np:<3} {secs:6.1f}s  of {limit}s')
    passing = [t for t in tests if not t[0]]
    if passing:
        _, name, np, secs, _ = max(passing, key=lambda t: t[3])
        print(f'  slowest test that PASSED: {name} np={np} at {secs:.1f}s '
              f'-- a cap must clear this')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        report(p)

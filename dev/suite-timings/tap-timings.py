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

A test is counted as "at the limit" when it took at least 95% of the largest
time in the file, which finds the limit without being told it -- runtests'
default is 180 s, the MPICH legs run at a multiplier of 2, and a testlist line
may carry a `timeLimit=` of its own.
"""
import re, sys
from collections import defaultdict

LINE = re.compile(r'^(not )?ok \d+ - \./(\S+) (\d+) # time=([0-9.]+)', re.M)


def report(path):
    text = open(path, encoding='utf-8', errors='replace').read()
    tests = [(m.group(1) is not None, m.group(2), int(m.group(3)), float(m.group(4)))
             for m in LINE.finditer(text)]
    if not tests:
        print(f'{path}: no timed TAP lines'); return
    total = sum(t[3] for t in tests)
    slowest = max(t[3] for t in tests)
    at_limit = [t for t in tests if t[3] >= 0.95 * slowest]
    limit_cost = sum(t[3] for t in at_limit)

    by_dir = defaultdict(lambda: [0.0, 0])
    for _, name, _, secs in tests:
        d = by_dir[name.split('/')[0]]
        d[0] += secs; d[1] += 1

    print(f'\n{path}')
    print(f'  {len(tests)} timed tests, {sum(1 for t in tests if t[0])} failed, '
          f'{total/60:.1f} min total')
    for d, (secs, n) in sorted(by_dir.items(), key=lambda kv: -kv[1][0]):
        print(f'    {d:<12} {secs/60:6.1f} min  {n:4d} tests')
    if at_limit:
        print(f'  at the limit (>= {0.95*slowest:.0f}s): {len(at_limit)} tests, '
              f'{limit_cost/60:.1f} min = {100*limit_cost/total:.0f}% of the shard')
        for failed, name, np, secs in sorted(at_limit, key=lambda t: t[1]):
            print(f'    {"FAIL" if failed else "pass"}  {name:<40} np={np:<3} {secs:6.1f}s')
    passing = [t for t in tests if not t[0]]
    if passing:
        failed, name, np, secs = max(passing, key=lambda t: t[3])
        print(f'  slowest test that PASSED: {name} np={np} at {secs:.1f}s '
              f'-- a cap must clear this')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        report(p)

#!/usr/bin/env python3

"""Gate this project's own ctest run against a committed expected-failure list.

    check-ctest.py <build-dir> <xfail-file>... [--failed-file=PATH] [--update]

**Why this exists, and why it is not ci-scripts/suite/check-tap.py.** That script
gates MPICH's C test suite, whose vocabulary is TAP and whose names are
"<dir>/<program> <np>"; this one gates the fifteen tests CMakeLists.txt declares,
whose names are ctest's. The two share a doctrine and nothing else, so they share
a grammar -- `<name> : why`, a reason required -- and stay separate programs
rather than one with a mode switch.

What the doctrine is worth spelling out again: before this file, a ctest row had
exactly two ways to describe a known failure, and both were bad. It could go
report-only (`continue-on-error`), which stops the row from ever failing and so
stops it from being a test -- the thing TODO.md forbids. Or the failing test
could be excluded with `ctest -E`, which deletes the coverage and never tells you
the day the implementation fixes it. The MVAPICH legs had taken the first road:
one upstream hang in MPI_Dist_graph_create made the whole row unable to report a
regression in the fourteen tests that pass.

Matching is checked in both directions, exactly as check-tap.py does it:

  * a failure that is not in the list fails the run -- the ordinary direction;
  * a listed failure that *passed* fails the run, because an expectation that has
    stopped firing is either fixed (delete the line) or was never about what the
    line says (rewrite it). Left alone it silently excuses a real failure later.
    This is the direction that makes a capped upstream hang self-retiring: the
    day MVAPICH fixes MPI_Dist_graph_create, this row goes red asking for its
    line back;
  * a listed test that does not exist in this build fails the run, so a line
    cannot outlive the test it names. There is no sharding here and so no
    "did not run" escape hatch -- every test CMakeLists.txt declares is offered
    to every ctest invocation this gates;
  * a line with no reason fails the run. --update writes exactly such lines, so a
    run that discovers new failures cannot be turned green by re-running with
    --update: the reasons have to be typed by a human.

**A capped test is still a tested test.** linux-test.sh's CTEST_TIMEOUT bounds
what a hang costs; it does not remove the test, so the hang arrives here as a
failure with a name and gets compared against the list like any other. That is
the same split ci-scripts/suite/timelimit-ci-openmpi.txt makes for the suite:
capping is about cost, listing is about meaning.

The failures come from ctest's own `Testing/Temporary/LastTestsFailed.log`, which
every ctest writes and which needs no CMake newer than this project's floor --
`--output-junit` would have been tidier and wants CMake 3.21, above the 3.20 that
NOTES.md #9 pins as the toolchain floor. The full test list comes from
`ctest -N`, which runs nothing.

**LastTestsFailed.log records the last run in that build directory**, so anything
that invokes ctest again before this script reads it destroys the evidence --
linux-test.sh's informational dlmopen probe is one such invocation, and it runs
*after* this gate for that reason. The file's absence means no test failed; the
caller is responsible for deleting a stale one before running ctest, which
linux-test.sh does.
"""

import os
import re
import subprocess
import sys

# "  Test #9: abi_arrays_test" -- ctest -N's one line per registered test.
CTEST_N_RE = re.compile(r"^\s*Test\s+#(\d+):\s+(\S.*?)\s*$")
# "9:abi_arrays_test" -- LastTestsFailed.log's index:name.
FAILED_RE = re.compile(r"^\s*(\d+):(.*?)\s*$")


def all_tests(build_dir):
    """Every test name ctest would offer in this build, in ctest's order."""
    try:
        out = subprocess.run(["ctest", "--test-dir", build_dir, "-N"],
                             capture_output=True, text=True, check=False)
    except FileNotFoundError:
        sys.exit("ctest: not on PATH -- cannot enumerate this build's tests")
    if out.returncode != 0:
        sys.exit(f"ctest -N failed in {build_dir}:\n{out.stdout}{out.stderr}")
    names = [m.group(2) for m in
             (CTEST_N_RE.match(line) for line in out.stdout.splitlines()) if m]
    if not names:
        # The empty-suite case that --no-tests=error catches for a plain ctest
        # run: a configure that registered nothing would otherwise gate green
        # against a list of things that "did not fail".
        sys.exit(f"{build_dir}: ctest -N reported no tests at all")
    return names


def failed_tests(path):
    """The names in LastTestsFailed.log; empty when the file is not there."""
    if not os.path.exists(path):
        return []
    names = []
    with open(path) as f:
        for line in f:
            if not line.strip():
                continue
            m = FAILED_RE.match(line)
            # A line this does not match is a format this script does not know,
            # and guessing would mean under-reporting failures.
            if not m:
                sys.exit(f"{path}: cannot parse line: {line.rstrip()}")
            names.append(m.group(2))
    return names


def parse_xfail(paths):
    """Return ({name: reason}, [complaints]) over one or more list files.

    Read as one file, and a name listed twice is an error rather than an
    override -- two reasons for one test means one of them is unmaintained.
    Deliberately the same rule, and the same wording, as check-tap.py's.
    """
    entries, problems, source = {}, [], {}
    for path in paths:
        try:
            f = open(path)
        except FileNotFoundError:
            problems.append(f"{path}: no expected-failure list")
            continue
        with f:
            for lineno, line in enumerate(f, 1):
                line = line.rstrip("\n")
                # A comment starts with #; a reason may contain one, so nothing
                # else is stripped.
                if not line.strip() or line.lstrip().startswith("#"):
                    continue
                if ":" not in line:
                    problems.append(f"{path}:{lineno}: no reason "
                                    f"(expected '<test name> : why')")
                    continue
                name, reason = line.split(":", 1)
                name, reason = name.strip(), reason.strip()
                if not name:
                    continue
                if not reason:
                    problems.append(f"{path}:{lineno}: {name} has no reason")
                if name in entries:
                    problems.append(f"{path}:{lineno}: {name} is also listed in "
                                    f"{source[name]}; one test, one reason")
                entries[name] = reason
                source[name] = path
    return entries, problems


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    opts = [a for a in argv[1:] if a.startswith("--")]
    if len(args) < 2:
        sys.exit(__doc__)
    build_dir, xfail_paths = args[0], args[1:]
    update = "--update" in opts
    failed_file = os.path.join(build_dir, "Testing", "Temporary",
                               "LastTestsFailed.log")
    for o in opts:
        if o.startswith("--failed-file="):
            failed_file = o[len("--failed-file="):]
        elif o not in ("--update",):
            sys.exit(f"unknown option: {o}")

    registered = all_tests(build_dir)
    failed = failed_tests(failed_file)
    expected, problems = parse_xfail(xfail_paths)

    known = set(registered)
    unexpected_fail = [n for n in failed if n not in expected]
    matched = [n for n in failed if n in expected]
    unexpected_pass = [n for n in sorted(expected)
                       if n in known and n not in failed]
    absent = [n for n in sorted(expected) if n not in known]

    print(f"  {len(registered)} tests registered, {len(failed)} failed")
    print(f"  {len(matched)} of the failures are expected and listed; "
          f"{len(unexpected_fail)} are not")

    if update:
        with open(xfail_paths[-1], "a") as f:
            if unexpected_fail:
                f.write("\n# added by --update; each line needs a reason\n")
            for name in unexpected_fail:
                f.write(f"{name} :\n")
        print(f"  wrote {len(unexpected_fail)} bare lines to {xfail_paths[-1]}")
        return 0

    ok = True
    for name in unexpected_fail:
        print(f"  UNEXPECTED FAILURE  {name}")
        ok = False
    for name in unexpected_pass:
        print(f"  EXPECTED FAILURE THAT PASSED  {name}   ({expected[name]})")
        ok = False
    for name in absent:
        print(f"  LISTED BUT NOT A TEST IN THIS BUILD  {name}   "
              f"({expected[name]})")
        ok = False
    for p in problems:
        print(f"  {p}")
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))

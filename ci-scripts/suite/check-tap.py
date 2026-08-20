#!/usr/bin/env python3

"""Gate a MPICH-test-suite run against this variant's expected-failure list.

    check-tap.py <summary.tap> <xfail-file>... [--flaky=FILE]...
                 [--dirs=a,b,c] [--update]

More than one list may be given, and they are read as one: a per-implementation
list that holds what every environment sees, plus a smaller per-environment one
beside it. That is what keeps a local run and a CI runner from needing two
copies of the same 40 reasons -- the two disagree about a handful of tests, not
about the implementation. A test listed in two of the files is an error rather
than an override, for the reason every other duplicate here is: two reasons for
one test means one of them is unmaintained. --update appends to the *last* file
named, which is the environment-specific one when there is one.

HISTORY.md S7: "the xfail list is committed with reasons; a variant's result
matching its list is the gate". Matching is checked in both directions, the
same discipline dev/check-c-bindings.py applies to the standard's Appendix
A.3 and dev/check_prototype.py to S1's reference bodies:

  * a failure that is not in the list fails the run -- the ordinary direction;
  * a listed failure that *passed* fails the run too, because an expectation
    that has stopped firing is either fixed (delete the line) or was never
    about what the line says (rewrite it). Left alone it silently excuses a
    real failure later;
  * a listed failure whose test did not run at all fails the run, so that a
    line cannot outlive the test it names -- unless the run did not cover
    that directory, which this script is told about rather than guessing;
  * a line with no reason fails the run. --update writes exactly such lines,
    so a run that discovers new failures cannot be turned into a green one by
    re-running with --update: the reasons have to be typed by a human.

**--flaky names a third category, and it exists because the two above cannot
express one.** A test that fails in one run and passes in the next cannot be
listed as an expected failure -- that fails the runs it passes -- and cannot be
left unlisted either -- that fails the runs it fails. Every such test was
therefore left out of the lists with a paragraph of prose beside it, which made
a red run something a human had to read a comment to interpret. A flaky entry
says "either outcome is acceptable", so the gate stays meaningful for everything
else.

The rules that still apply to a flaky line are the ones that keep it from
rotting: it needs a reason, it may not also appear in an xfail list, and it must
still name a test that ran (in a directory this run covered) or it is stale.
What it gives up is the ability to notice that the test has settled down, which
is the price of not having to guess which way it will go; a flaky line should
name what would let it be promoted back to xfail or deleted.

Flaky outcomes are printed either way, so "it flapped again" is visible in the
log rather than silently excused.

The TAP names are runtests' own -- "<dir>/<program> <np>" -- so the list
reads as the suite's own vocabulary and can be compared with an unwrapped
run by eye.
"""

import re
import sys

TAP_RE = re.compile(
    r"^(?P<ok>ok|not ok)\s+\d+\s+-\s+(?P<name>.*?)\s*"
    r"(?:#\s*(?P<directive>SKIP|TODO)\b(?P<dirtext>.*))?$")


def parse_tap(path):
    """Return {name: ('pass'|'fail'|'skip', detail)} in file order."""
    results = {}
    try:
        f = open(path)
    except FileNotFoundError:
        # A run killed before runtests wrote anything lands here, and a
        # traceback would read as a bug in this script rather than as the
        # missing file it is.
        sys.exit(f"{path}: no TAP file -- the run did not get that far")
    with f:
        for line in f:
            line = line.rstrip("\n")
            if not line.startswith(("ok ", "not ok ")):
                continue
            m = TAP_RE.match(line)
            if not m:
                sys.exit(f"{path}: cannot parse TAP line: {line}")
            name = m.group("name")
            # runtests appends "# time=..." to plain results and "# TODO
            # <reason>" to tests its own testlist marked xfail; neither is
            # part of the name.
            name = re.sub(r"\s*#.*$", "", name).strip()
            name = re.sub(r"\s+", " ", name)
            # runtests names a test by the path it descended to it through,
            # which always starts at "./"; the list reads better without it.
            if name.startswith("./"):
                name = name[2:]
            if m.group("directive") == "SKIP":
                outcome, detail = "skip", (m.group("dirtext") or "").strip()
            elif m.group("ok") == "ok":
                outcome, detail = "pass", ""
            else:
                outcome, detail = "fail", ""
            # One name can appear more than once: testlist.dtp runs the same
            # program at the same rank count over different datatype pools,
            # and runtests names it identically each time. A failure wins, so
            # that a passing repetition cannot hide a failing one.
            if results.get(name, ("", ""))[0] != "fail":
                results[name] = (outcome, detail)
    return results


def parse_xfail(paths):
    """Return ({name: reason}, [complaints]) over one or more list files."""
    entries, problems, source = {}, [], {}
    for path in paths:
        _parse_one(path, entries, problems, source)
    return entries, problems


def _parse_one(path, entries, problems, source):
    try:
        f = open(path)
    except FileNotFoundError:
        problems.append(f"{path}: no expected-failure list for this variant")
        return
    with f:
        for lineno, line in enumerate(f, 1):
            # A comment is a line that starts with #; a reason may contain
            # one, so nothing else is stripped.
            line = line.rstrip("\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            if ":" not in line:
                problems.append(f"{path}:{lineno}: no reason (expected "
                                f"'<dir>/<program> <np> : why')")
                continue
            name, reason = line.split(":", 1)
            name = re.sub(r"\s+", " ", name).strip()
            reason = reason.strip()
            if not name:
                continue
            if not reason:
                problems.append(f"{path}:{lineno}: {name} has no reason")
            if name in entries:
                problems.append(f"{path}:{lineno}: {name} is also listed in "
                                f"{source[name]}; one test, one reason")
            entries[name] = reason
            source[name] = path


def directory_of(name):
    return name.rsplit("/", 1)[0] if "/" in name else ""


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    opts = [a for a in argv[1:] if a.startswith("--")]
    if len(args) < 2:
        sys.exit(__doc__)
    tap_path, xfail_paths = args[0], args[1:]
    update = "--update" in opts
    dirs = None
    flaky_paths = []
    for o in opts:
        if o.startswith("--dirs="):
            dirs = {d.strip() for d in o[len("--dirs="):].split(",") if d.strip()}
        elif o.startswith("--flaky="):
            flaky_paths.append(o[len("--flaky="):])

    results = parse_tap(tap_path)
    expected, problems = parse_xfail(xfail_paths)
    flaky, flaky_problems = parse_xfail(flaky_paths) if flaky_paths else ({}, [])
    problems += flaky_problems

    # One test, one category: a name in both lists means nobody knows which it
    # is, and the xfail rule against duplicates applies across the pair for the
    # same reason it applies within a file.
    for name in sorted(set(expected) & set(flaky)):
        problems.append(f"{name} is listed as both an expected failure and "
                        f"flaky; one test, one category")

    covered = {directory_of(n).split("/")[0] for n in results}

    unexpected_fail = []
    unexpected_pass = []
    stale = []
    not_run = []
    matched = []
    flaked_fail = []
    flaked_pass = []

    for name, (outcome, _detail) in sorted(results.items()):
        if name in flaky:
            # Either way is acceptable; which way it went is still reported.
            if outcome == "fail":
                flaked_fail.append(name)
            elif outcome == "pass":
                flaked_pass.append(name)
            continue
        if outcome == "fail":
            (matched if name in expected else unexpected_fail).append(name)
        elif outcome == "pass" and name in expected:
            unexpected_pass.append(name)

    for name in sorted(set(expected) | set(flaky)):
        if name in results:
            continue
        top = directory_of(name).split("/")[0]
        if dirs is not None and top not in dirs:
            not_run.append(name)          # this run did not cover it
        elif top not in covered:
            not_run.append(name)
        else:
            stale.append(name)

    npass = sum(1 for o, _ in results.values() if o == "pass")
    nfail = sum(1 for o, _ in results.values() if o == "fail")
    nskip = sum(1 for o, _ in results.values() if o == "skip")

    print(f"  {len(results)} tests: {npass} passed, {nfail} failed, "
          f"{nskip} skipped by the suite")
    print(f"  {len(matched)} of the failures are expected and listed; "
          f"{len(unexpected_fail)} are not")
    if flaked_fail or flaked_pass:
        print(f"  {len(flaked_fail) + len(flaked_pass)} flaky tests ran: "
              f"{len(flaked_fail)} failed, {len(flaked_pass)} passed "
              f"(neither gates)")
    if not_run:
        print(f"  {len(not_run)} listed tests were not part of this run")

    if update:
        with open(xfail_paths[-1], "a") as f:
            if unexpected_fail:
                f.write("\n# added by --update; each line needs a reason\n")
            for name in unexpected_fail:
                f.write(f"{name} :\n")
        print(f"  wrote {len(unexpected_fail)} bare lines to {xfail_paths[-1]}")
        return 0

    for name in flaked_fail:
        print(f"  FLAKY, FAILED  {name}   ({flaky[name]})")
    for name in flaked_pass:
        print(f"  FLAKY, PASSED  {name}   ({flaky[name]})")

    ok = True
    for name in unexpected_fail:
        print(f"  UNEXPECTED FAILURE  {name}")
        ok = False
    for name in unexpected_pass:
        print(f"  EXPECTED FAILURE THAT PASSED  {name}"
              f"   ({expected[name]})")
        ok = False
    for name in stale:
        why = expected.get(name) or flaky.get(name)
        print(f"  LISTED BUT NOT RUN  {name}   ({why})")
        ok = False
    for p in problems:
        print(f"  {p}")
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))

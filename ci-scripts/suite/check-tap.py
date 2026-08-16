#!/usr/bin/env python3

"""Gate a MPICH-test-suite run against this variant's expected-failure list.

    check-tap.py <summary.tap> <xfail-file> [--dirs=a,b,c] [--update]

STAGES.md S7: "the xfail list is committed with reasons; a variant's result
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
    with open(path) as f:
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


def parse_xfail(path):
    """Return ({name: reason}, [complaints]).  Missing file means no list."""
    entries, problems = {}, []
    try:
        f = open(path)
    except FileNotFoundError:
        return entries, [f"{path}: no expected-failure list for this variant"]
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
                problems.append(f"{path}:{lineno}: {name} listed twice")
            entries[name] = reason
    return entries, problems


def directory_of(name):
    return name.rsplit("/", 1)[0] if "/" in name else ""


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    opts = [a for a in argv[1:] if a.startswith("--")]
    if len(args) != 2:
        sys.exit(__doc__)
    tap_path, xfail_path = args
    update = "--update" in opts
    dirs = None
    for o in opts:
        if o.startswith("--dirs="):
            dirs = {d.strip() for d in o[len("--dirs="):].split(",") if d.strip()}

    results = parse_tap(tap_path)
    expected, problems = parse_xfail(xfail_path)

    covered = {directory_of(n).split("/")[0] for n in results}

    unexpected_fail = []
    unexpected_pass = []
    stale = []
    not_run = []
    matched = []

    for name, (outcome, _detail) in sorted(results.items()):
        if outcome == "fail":
            (matched if name in expected else unexpected_fail).append(name)
        elif outcome == "pass" and name in expected:
            unexpected_pass.append(name)

    for name in sorted(expected):
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
    if not_run:
        print(f"  {len(not_run)} listed tests were not part of this run")

    if update:
        with open(xfail_path, "a") as f:
            if unexpected_fail:
                f.write("\n# added by --update; each line needs a reason\n")
            for name in unexpected_fail:
                f.write(f"{name} :\n")
        print(f"  wrote {len(unexpected_fail)} bare lines to {xfail_path}")
        return 0

    ok = True
    for name in unexpected_fail:
        print(f"  UNEXPECTED FAILURE  {name}")
        ok = False
    for name in unexpected_pass:
        print(f"  EXPECTED FAILURE THAT PASSED  {name}"
              f"   ({expected[name]})")
        ok = False
    for name in stale:
        print(f"  LISTED BUT NOT RUN  {name}   ({expected[name]})")
        ok = False
    for p in problems:
        print(f"  {p}")
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))

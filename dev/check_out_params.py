#!/usr/bin/env python3
"""Does every early return in a generated body define every out parameter?

NOTES.md #7 decision 6 promises a caller that ignores the return code still
reads defined memory. `emit_stub` keeps that promise -- it pairs
`null_out_handles` with `stub_out_zeros` -- but the *generated* bodies did not:
the length and extent-reject guards nulled the out handles and left the out
scalars, and the staging-failure `goto done` path emitted neither. This is the
measurement that found the 71 sites and the check that keeps them closed.

Run it on the committed output, or on any older copy, to get the count:

    python3 dev/check_out_params.py gen/mpiwrapper/wrappers.c

It classifies parameters through generate.py itself rather than re-deriving
which ones are out, so it cannot disagree with the generator about the set it
is auditing -- `p.cls` is assigned in `assign_status`, not in `load`, and an
audit that skips that step silently sees no out handles at all.

Scope rule, walking back from the return: a line is in scope only while its
indent does not exceed a running floor. A deeper line belongs to a sibling
block that has already closed -- the `*abi_x = ...` inside some *other*
`if (...) { ... return; }` does not define anything on the path that falls
through -- and a shallower line lowers the floor, being the opener of the
block we were just in. Both directions matter, measured against the 71 sites
this rule finds on the revision before the fix: an indent-blind scan finds 54,
and counting only body-level assignments finds 282.

Exit status is 1 when anything is undefined, so this can be a test.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate as g


def generated_arms(text):
    """(name, guard, lines) for every arm of every BODY_ macro worth auditing.

    A macro has one arm per implementation shape the entry point can be served
    by, each a separate `#define` of the same name: the primary body under
    `#ifdef MPIWRAPPER_HAVE_<name>`, then -- since the large-count fallback --
    a narrowing body under `#elif defined(MPIWRAPPER_HAVE_<small twin>)` for
    the 148 `_c` names that have one, and last `emit_stub`'s under `#else`.

    Both earlier revisions of this audit missed the narrowing arms twice over,
    and silently: the macro grew a second parameter, so `BODY_(\w+)\(TARGET\)`
    stopped matching those 148 macros altogether, and the arm is a *second*
    `#define` of a name already seen, which the old "the second one is the
    stub" rule discarded. It reported clean over code it had never read. The
    arm is identified by the directive that introduces it instead, which is
    what the generator actually keys the stub on.
    """
    arms, buf, guard, name = [], None, None, None
    for line in text.split("\n"):
        if line.startswith("#ifdef ") or line.startswith("#if "):
            guard = "if"
        elif line.startswith("#elif "):
            guard = "elif"
        elif line.startswith("#else"):
            guard = "else"
        m = re.match(r"#define BODY_(\w+)\((TARGET[^)]*)\)", line)
        if m:
            name = m.group(1)
            if guard == "else":          # emit_stub's arm, audited elsewhere
                buf = None
            else:
                buf = []
                arms.append((name, guard, buf))
            continue
        if buf is None:
            continue
        if line.startswith("#") or (line and not line.startswith(" ")):
            buf = None
        else:
            buf.append(line.rstrip("\\ ").rstrip())
    return arms


def assigns(line, abi):
    """Does `line` give `*abi` a value? Both the plain form and the guarded
    `if (abi) *abi = ...` that NULLABLE_OUT_ROUTINES produces."""
    return re.match(r"\s*(if \(" + abi + r"\) )?\*" + abi + r"\s*=[^=]", line)


def in_scope(lines, r, abi):
    """Is `*abi` defined on every path reaching line `r`?"""
    floor = len(lines[r]) - len(lines[r].lstrip())
    for line in reversed(lines[:r]):
        if not line.strip():
            continue
        indent = len(line) - len(line.lstrip())
        if indent > floor:
            continue
        floor = indent
        if assigns(line, abi):
            return True
    return False


def audit(path):
    protos = g.load(g.gh.apply_patch())
    g.assign_status(protos, g.parse_handwritten_h())
    arms = generated_arms(Path(path).read_text())

    findings, audited = [], 0
    for name, guard, lines in arms:
        ep = protos.get(name)
        if ep is None:
            continue
        owed = [a for a, _ in g.out_handle_nulls(ep)] + g.out_scalar_zeros(ep)
        if not owed:
            continue
        audited += 1
        # Everything before the implementation call is an early return: past
        # it the call has written the outs itself. The narrowing arm reaches
        # its implementation through FALLBACK rather than TARGET.
        cut = next((i for i, l in enumerate(lines)
                    if re.search(r"\b(TARGET|FALLBACK)\(", l)), len(lines))
        for r, line in enumerate(lines[:cut]):
            if not re.search(r"\breturn\b|goto done", line):
                continue
            missing = [a for a in owed if not in_scope(lines, r, a)]
            if missing:
                findings.append((name, guard, line.strip(), missing))
    return audited, len(arms), findings


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "gen/mpiwrapper/wrappers.c"
    audited, total, findings = audit(path)
    print(f"{path}: {total} generated arms, {audited} with an out handle or "
          f"out scalar")
    by_name = {}
    for name, guard, text, missing in findings:
        arm = "narrowing" if guard == "elif" else "primary"
        by_name.setdefault(name, []).append((arm, text, missing))
    for name in sorted(by_name):
        print(f"  {name}")
        for arm, text, missing in by_name[name]:
            print(f"      [{arm}] {text}   "
                  f"leaves {', '.join(missing)} undefined")
    print(f"  => {len(findings)} undefined-out early returns in "
          f"{len(by_name)} entry points")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())

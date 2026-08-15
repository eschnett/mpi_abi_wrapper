#!/usr/bin/env python3
"""S2's exit check: does the generator reproduce the S1 prototype?

`dev/s1-reference/` holds the four files S1 hand-wrote as stand-ins for
generated output, frozen exactly as S1 left them. This compares each against
what `dev/generate.py` now emits, item by item -- slot by slot, function by
function, body macro by body macro -- and requires every S1 item to be present
and identical, or to be named in EXEMPT below with the reason it is not.

REWRITES is the third possibility, for a change of *mechanism* S2 made
everywhere: the reference is rewritten and then compared exactly. There is one,
and it is narrower than exempting the dozen items it touches would be.

Comparison is over *normalized* text: comments removed, macro line
continuations joined, whitespace collapsed. The generator does not run
clang-format and does not write S1's per-function prose, so a byte comparison
would fail on formatting alone and say nothing; a token comparison fails only
when the code differs.

An exemption that stops firing is an error, not a pass. When S3 generates
MPI_Waitall, or if someone teaches the generator S1's abbreviation for a stack
buffer, this fails and asks for the exemption to be deleted -- which is the
property that keeps the list honest (the discipline mpif's binding checks use).

Usage: dev/check_prototype.py
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REF = ROOT / "dev" / "s1-reference"

PAIRS = [
    ("vtable slot", REF / "mpiwrapper_vtable.h",
     ROOT / "gen" / "include" / "mpiwrapper_vtable.h"),
    ("ABI entry point", REF / "entrypoints.c",
     ROOT / "gen" / "mpi_abi" / "entrypoints.c"),
    ("wrapper body", REF / "wrappers.c",
     ROOT / "gen" / "mpiwrapper" / "wrappers.c"),
    ("conversion table", REF / "constants.c",
     ROOT / "gen" / "mpiwrapper" / "constants.c"),
]

# (kind, name) -> why the generated form differs from S1's, or why there is
# none. Three entries, and each is a decision rather than an oversight.
EXEMPT = {
    ("wrapper body", "MPI_Comm_rank"):
        "S1 passed the out rank through, reasoning per-site that a process's "
        "rank in its own communicator is never a sentinel. That is true here "
        "and false one function away -- MPI_Group_rank answers MPI_UNDEFINED "
        "-- so the generator maps every out-rank uniformly. Uniformity is the "
        "correctness property (NOTES.md #3); the per-site reasoning is what a "
        "generator must not encode.",
    ("wrapper body", "MPI_Type_create_struct"):
        "Identical but for two identifiers: the generator names a staging "
        "buffer <local>_stack, where S1 wrote `typestack` (and `reqstack`, "
        "`ststack` in MPI_Waitall). Abbreviating per site is not a rule a "
        "generator can follow.",
    ("wrapper body", "MPI_Type_create_struct_c"):
        "The same staging-buffer name as its small form. The guard around the "
        "body also changed and this comparison does not see it: S1 wrote "
        "`#if MPI_VERSION >= 4`, and every generated body is now guarded on "
        "MPIWRAPPER_HAVE_<name> from dev/probe_impl.py instead. That is "
        "decision 6's `#ifdef` made exact -- Open MPI 5.0.10 reports MPI-3.1 "
        "and has sessions, so the version test both over- and under-reports.",
    ("wrapper body", "MPI_Waitall"):
        "S3 generates it, and the generated body is S1's code with four "
        "differences, all of them shape rather than substance: the ABI "
        "parameters are named after the ABI header's own parameters "
        "(abi_array_of_requests, not S1's abi_requests); the staging buffers "
        "are <local>_stack rather than S1's reqstack and ststack, which is "
        "the same abbreviation the MPI_Type_create_struct entry covers; "
        "`ierror` is declared const inside the inner block every generated "
        "staged body uses, where S1 declared it uninitialized in the outer "
        "group; and the two mpiwrapper_unstage calls run in declaration order "
        "rather than S1's reverse order, which is immaterial because "
        "mpiwrapper_unstage is independent per array. The staging, the "
        "MPI_STATUSES_IGNORE short circuit, the release-at-completion rule, "
        "the unconditional write-back and the single goto-done cleanup are "
        "token-for-token S1's. What checks the body itself is behavioural: "
        "test/abi_prototype_test.c exercises MPI_Waitall over both "
        "implementations, including the staged-temporary release that "
        "MPI_Ialltoallw sets up.",
}


# Rewrites applied to the *reference* before comparing, each a change of
# mechanism that S2 made everywhere rather than a change to any one item.
# Declaring one here is much narrower than exempting the dozen items it touches:
# an exemption stops checking an item entirely, so a second, unintended change
# to the same table would ride along unnoticed.
REWRITES = [
    (re.compile(r"#ifdef (MPI[A-Z_]*_[A-Za-z0-9_]+)"),
     r"#ifdef MPIWRAPPER_HAVE_\1",
     "S1 guarded an optional constant on the implementation's own spelling of "
     "it. `#ifdef` sees macros and not enumerators, and implementations use "
     "both, so S2 moved every such guard onto dev/probe_impl.py's "
     "MPIWRAPPER_HAVE_<name>. Which cases are guarded, and what each maps to, "
     "is still compared exactly."),
    (re.compile(r"default: return MPIABI_ERR_OTHER;"),
     "default: return mpiwrapper_errorcode_dynamic_toabi(ierror);",
     "S1's error-code switch answered MPIABI_ERR_OTHER for anything it did "
     "not recognize, and said in a comment beside it that the registry for "
     "codes handed out at run time was S4's. S4b built it "
     "(src/mpiwrapper/errorcodes.c), so the default arm asks it instead -- "
     "and still answers MPIABI_ERR_OTHER for a code it never issued, which "
     "is the case S1 was covering. Every predefined case is compared "
     "exactly, as before."),
    (re.compile(r"default: return MPI_ERR_OTHER;"),
     "default: return mpiwrapper_errorcode_dynamic_fromabi(abi_ierror);",
     "The same change in the other direction."),
]


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", " ", text)


def normalize(text):
    return re.sub(r"\s+", "", strip_comments(text.replace("\\\n", "\n")))


def blocks(text, start_re):
    """Named macro definitions, from `#define X(TARGET)` to the last
    continued line."""
    out = {}
    lines = text.splitlines()
    for i, line in enumerate(lines):
        m = start_re.match(line.strip())
        if not m:
            continue
        j = i
        while j < len(lines) and lines[j].rstrip().endswith("\\"):
            j += 1
        out.setdefault(m.group(1), "\n".join(lines[i + 1:j + 1]))
    return out


def wrapper_bodies(text):
    return {k: normalize(v) for k, v in
            blocks(text, re.compile(r"#\s*define\s+BODY_(\w+)\(TARGET\)")).items()}


def vtable_slots(text):
    body = text[text.index("struct mpiwrapper_vtable {"):]
    body = body[:body.index("\n};")]
    body = strip_comments(body)
    out = {}
    for decl in re.finditer(r"[A-Za-z_][\w ]*\(\*(P?MPI_\w+)\)\([^;]*\);", body):
        out[decl.group(1)] = re.sub(r"\s+", "", decl.group(0))
    return out


def c_functions(text, name_re):
    """Top-level function definitions, keyed by name."""
    text = strip_comments(text)
    out = {}
    for m in re.finditer(
            r"^[A-Za-z_][\w \*]*?\b(" + name_re + r")\s*\(([^;{]*)\)\s*\{",
            text, re.MULTILINE):
        depth, i = 0, m.end() - 1
        while i < len(text):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        out[m.group(1)] = re.sub(r"\s+", "", m.group(2) + text[m.end():i])
    return out


def items(kind, text):
    if kind == "wrapper body":
        return wrapper_bodies(text)
    if kind == "vtable slot":
        return vtable_slots(text)
    if kind == "ABI entry point":
        return c_functions(text, r"P?MPI_\w+")
    return c_functions(text, r"(?:mpiwrapper|predef|toabi_bits|fromabi_bits)_\w+")


def main():
    failures, checked, exempt_fired = [], 0, set()
    rewrite_fired = set()

    for kind, ref_path, gen_path in PAIRS:
        reference = ref_path.read_text()
        for i, (pattern, replacement, _) in enumerate(REWRITES):
            reference, n = pattern.subn(replacement, reference)
            if n:
                rewrite_fired.add(i)
        ref = items(kind, reference)
        gen = items(kind, gen_path.read_text())
        if not ref:
            raise SystemExit(f"{ref_path}: nothing extracted -- the reference "
                             "parser and the file have drifted apart")
        for name, text in sorted(ref.items()):
            checked += 1
            same = name in gen and gen[name] == text
            exemption = EXEMPT.get((kind, name))
            if same and exemption:
                failures.append(
                    f"{kind} {name}: reproduces S1 exactly, but is still "
                    f"exempted. Delete the entry from EXEMPT.")
            elif not same and not exemption:
                why = ("absent from the generated file"
                       if name not in gen else "differs from S1's")
                failures.append(f"{kind} {name}: {why}, and is not exempted.")
            elif exemption:
                exempt_fired.add((kind, name))

    stale = set(EXEMPT) - exempt_fired
    for kind, name in sorted(stale):
        failures.append(f"{kind} {name}: exempted, but the S1 reference has no "
                        "such item. Delete the entry from EXEMPT.")

    # A rewrite that matches nothing is the same failure as an exemption that
    # stops firing: it claims a change of mechanism the reference no longer
    # shows, and it would go on silently not applying to whatever replaced it.
    for i, (pattern, _, _) in enumerate(REWRITES):
        if i not in rewrite_fired:
            failures.append(f"rewrite /{pattern.pattern}/ matched nothing in "
                            "the S1 reference. Delete it from REWRITES.")

    if failures:
        print("\n".join("  " + f for f in failures), file=sys.stderr)
        raise SystemExit(f"{len(failures)} difference(s) from the S1 prototype "
                         "that no exemption covers")

    print(f"OK: {checked} S1 items, {checked - len(exempt_fired)} reproduced "
          f"exactly, {len(exempt_fired)} exempted with a reason, "
          f"{len(REWRITES)} declared rewrite(s) of the reference")


if __name__ == "__main__":
    main()

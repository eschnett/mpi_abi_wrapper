#!/usr/bin/env python3
"""Check gen/include/mpi.h's C signatures against MPI-5.0 Appendix A.3.

Oracle 4 of NOTES.md #10: an independent route from the same LaTeX that
produced apis.json. Where apis.json and the generator could share a
misreading of the standard, `pdftotext -layout` on the standard's own PDF
report cannot -- it never touches apis.json or the vendored mpi.h this
project generates from, so the two routes can only agree by actually being
right.

Two properties carried over from mpif's dev/check-f08-bindings.jl, which set
the pattern for this kind of cross-check:

  - the parse validates itself: the appendix's open- and close-paren counts
    must match the number of signatures found, one pair per signature and
    nothing left over, or the text was misread and no comparison built on it
    can be trusted;
  - exemptions are named, explained, and fail the run when they stop firing,
    so the list cannot go stale.

Usage: dev/check-c-bindings.py [doc/mpi50-report.pdf]

doc/mpi50-report.pdf is committed (unlike mpif's git-ignored copy), so no
setup is needed beyond having `pdftotext` (poppler-utils) on PATH.

Exits 0 when the only divergences from gen/include/mpi.h are the ones named
in EXPECTED below.
"""

import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
HEADER = REPO / "gen" / "include" / "mpi.h"

START_HEADING = "A.3 C Bindings"
END_HEADING = "A.4 Fortran 2008 Bindings with the mpi_f08 Module"

# A header declaration's return type is always the first whitespace-separated
# token; PMPI_/MPI_ handle-converters return a handle type (MPI_Comm, and so
# on) rather than the usual int/void/double, so this is not a fixed set.
DECL = re.compile(
    r"^(?P<ret>[A-Za-z_][A-Za-z0-9_]*)\s+"
    r"(?P<name>(?:MPI|PMPI)_[A-Za-z0-9_]+)"
    r"\((?P<args>.*)\)\s*;?\s*$"
)

# The same signature start, for scanning a whole blob of joined appendix
# lines rather than one declaration per line -- ^/$ above anchor to the
# *string's* start and end without re.MULTILINE, which is right for a single
# header line and wrong for a multi-signature blob.
SIG_START = re.compile(
    r"(?P<ret>[A-Za-z_][A-Za-z0-9_]*)\s+"
    r"(?P<name>(?:MPI|PMPI)_[A-Za-z0-9_]+)\s*\("
)

# A.3 documents each predefined callback constant (MPI_COMM_NULL_COPY_FN and
# so on) with the same "prototype" syntax as a real entry point, since a user
# supplies a function matching that shape -- but the constant itself is a
# value, not a callable ABI entry point, and none of them are in the 688.
# Every real entry point is MPI_Mixed_case; a name that is all caps after the
# prefix cannot be one, so the rule catches the whole family without having
# to name each one.
PREDEFINED_CALLBACK_CONSTANT = re.compile(r"^(?:MPI|PMPI)_[A-Z0-9_]+$")


def stripped_lines(text):
    """Every line of `pdftotext -layout`'s output, margin numbers gone.

    The standard prints a line number in the margin of every code line: on
    some pages trailing the text with two or more spaces, on others leading
    it the same way, and on others alone on its own line (dropped below by
    the pure-digit-line filter, the same one mpif's parser uses).
    """
    out = []
    for ln in text.split("\n"):
        ln = re.sub(r"\s{2,}\d+\s*$", "", ln)
        ln = re.sub(r"^\s*\d+\s{2,}", "", ln)
        out.append(ln.strip())
    return out


def appendix_lines(pdf_path):
    """The lines of Appendix A.3, running heads and margin numbers gone."""
    text = subprocess.run(
        ["pdftotext", "-layout", str(pdf_path), "-"],
        check=True, capture_output=True, text=True,
    ).stdout
    lines = stripped_lines(text)

    # The heading and the running head above it (page number stripped) read
    # the same; either will do as the start. The table of contents does not,
    # its dot leaders surviving the strip.
    start = end = None
    for n, s in enumerate(lines):
        if start is None:
            if s == START_HEADING:
                start = n
        elif s == END_HEADING:
            end = n
            break
    if start is None or end is None:
        raise SystemExit(f"{pdf_path}: could not find Appendix A.3 (C Bindings)")

    clean = []
    for s in lines[start + 1:end]:
        if not s:
            continue
        if re.match(r"^\d+$", s):
            continue
        if "Appendix A Language Bindings Summary" in s:
            continue
        if re.match(r"^A\.3(\.\d+)? [A-Z(]", s):
            continue
        if s == START_HEADING:
            continue
        clean.append(s)
    return clean


def split_type_name(arg):
    """An argument's ("type", "name") pair, folding an array suffix into `*`.

    The name is the last identifier in the declarator, so "char ***argv"
    splits as ("char ***", "argv") and "const int array_of_blocklengths[]"
    as ("const int *", "array_of_blocklengths") -- a function parameter's
    array declarator is indistinguishable from a pointer declarator in C, so
    folding "[]"/"[N]" into a trailing `*` before comparison is exactly the
    language's own equivalence, not an approximation. Both spellings appear
    on either side (e.g. MPI_Status[] in the appendix against MPI_Status* in
    the header), and normalizing them the same way is what keeps that from
    being reported as a type mismatch.
    """
    arg = arg.strip()
    if arg == "...":
        return "...", "..."
    m = re.search(r"([A-Za-z_]\w*)\s*((?:\[\w*\])*)\s*$", arg)
    if not m:
        raise ValueError(f"cannot find a declared name in {arg!r}")
    name = m.group(1)
    array = m.group(2)
    type_ = arg[:m.start()].rstrip()
    if array:
        type_ += " *"
    return type_, name


def normalize_type(t):
    return re.sub(r"\s+", "", t)


def parse_signature(name, args_text, source):
    """`([param_name, ...], {param_name: normalized_type})` from an arg list.

    Raises ValueError -- the self-validating half -- if an argument does not
    parse as "type name": every argument the appendix (or the header) writes
    has to come out as exactly one declared name, or the text was misread.
    """
    args_text = args_text.strip()
    if args_text in ("", "void"):
        return [], {}
    order = []
    params = {}
    for a in args_text.split(","):
        a = a.strip()
        if not a:
            raise ValueError(f"{source} {name}: empty argument in ({args_text})")
        type_, pname = split_type_name(a)
        if pname in params:
            raise ValueError(f"{source} {name}: {pname!r} declared twice")
        order.append(pname)
        params[pname] = normalize_type(type_)
    return order, params


def parse_appendix(pdf_path):
    """`{name: (ret, [param_names], {param_name: type})}` from Appendix A.3.

    The appendix is many signatures in a row rather than one per line, so
    finding each one is a scan for "RET NAME(" followed by matching the
    close paren by depth. `pdftotext` renders the standard's "..." varargs
    marker with the dots spaced out, so that is normalized first.
    """
    lines = appendix_lines(pdf_path)
    text = " ".join(lines)
    text = re.sub(r"\.\s*\.\s*\.", "...", text)

    routines = {}
    problems = []
    n_sigs = 0
    for m in SIG_START.finditer(text):
        n_sigs += 1
        name = m.group("name")
        open_idx = m.end() - 1
        depth, i, close_idx = 0, open_idx, None
        while i < len(text):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    close_idx = i
                    break
            i += 1
        if close_idx is None:
            problems.append(f"PARSE  appendix {name}: unbalanced parentheses")
            continue
        args_text = text[open_idx + 1:close_idx]
        if name in routines:
            problems.append(f"PARSE  {name}: declared twice in the appendix")
            continue
        try:
            order, params = parse_signature(name, args_text, "appendix")
        except ValueError as e:
            problems.append(f"PARSE  {e}")
            continue
        routines[name] = (m.group("ret"), order, params)

    # The parse checks itself: A.3 has nothing in it but signatures once the
    # heading/margin-number strip is done (verified by hand against the raw
    # `pdftotext` output when this script was written), so every "(" belongs
    # to exactly one signature found above and every ")" closes one. A count
    # that does not divide out means some text was missed or misread.
    opens, closes = text.count("("), text.count(")")
    if opens != n_sigs or closes != n_sigs:
        problems.append(
            f"PARSE  appendix: {opens} '(' and {closes} ')' for {n_sigs} "
            "signatures found -- the text has parentheses this parser did "
            "not expect, so nothing above can be trusted"
        )
    return routines, problems


def parse_header(path):
    """The same shape, from gen/include/mpi.h's single-line declarations."""
    routines = {}
    problems = []
    for lineno, raw in enumerate(path.read_text().split("\n"), start=1):
        line = re.sub(r"/\*.*?\*/", "", raw).strip()
        m = DECL.match(line)
        if m is None:
            continue
        name = m.group("name")
        if name in routines:
            problems.append(f"PARSE  {path}:{lineno}: {name} declared twice")
            continue
        try:
            order, params = parse_signature(name, m.group("args"), str(path))
        except ValueError as e:
            problems.append(f"PARSE  {path}:{lineno}: {e}")
            continue
        routines[name] = (m.group("ret"), order, params)
    return routines, problems


# Divergences that are known, deliberate and explained. Anything else is a
# finding. Each is counted as it fires, so that one which stops happening is
# noticed rather than silently kept -- the same discipline mpif's
# check-f08-bindings.jl uses for its own `expected` table.
EXPECTED = {
    "predefined callback constants, not entry points": """
        A.3 documents each predefined callback constant (MPI_COMM_NULL_COPY_FN,
        MPI_DUP_FN, MPI_CONVERSION_FN_NULL and so on) with a prototype, since a
        user-supplied function must match that shape -- but the constant itself
        is a value, not one of the 688 entry points, and gen/include/mpi.h has
        no business declaring it as a callable symbol.""",
    "missing entirely from A.3": """
        MPI_Wtime, MPI_Wtick, MPI_Aint_add and MPI_Aint_diff each have a C
        binding earlier in the standard body but no occurrence anywhere in
        Appendix A.3's text -- a gap in the standard's own summary, not in
        gen/include/mpi.h.""",
    "MPI_Status_f082f / MPI_Status_f2f08 not part of this ABI": """
        A.3 gives six status converters; doc/mpi.h.patch's vendored ABI
        (NOTES.md #1) exposes four -- c2f, f2c, c2f08, f082c -- and leaves out
        the direct Fortran-status/F08-status pair, which composes from the
        other four (f2c then c2f08, or f082c then c2f) and needs no ABI entry
        point of its own.""",
    "'index' renamed 'indx' to dodge libc's index()": """
        MPI_Graph_create, MPI_Graph_get, MPI_Graph_map, MPI_Request_get_status_any,
        MPI_T_enum_get_item, MPI_Testany and MPI_Waitany all take an `index` per
        A.3; gen/include/mpi.h spells it `indx` in every one, the same
        workaround essentially every MPI implementation's own headers use so
        that C++'s `using namespace std` (or a stray `#include <strings.h>`)
        cannot shadow the parameter with the libc function of that name.""",
    "MPI_T's 'pe_session' renamed 'session'": """
        The seven MPI_T_pvar_* functions call their MPI_T session argument
        `pe_session` in A.3 and `session` in gen/include/mpi.h -- a shorter
        spelling of the same parameter, unrelated to the direction or type of
        the argument a caller passes.""",
    "MPI_Status_get_error / _set_error's 'err' renamed 'error'": """
        A.3 spells the argument `err`; gen/include/mpi.h spells it `error`,
        matching every other status-field accessor's own naming
        (MPI_Status_get_source, _get_tag, and so on already say `source`/`tag`
        rather than an abbreviation).""",
    "MPI_Precv_init's 'source' kept its send-side template's 'dest'": """
        MPI_Precv_init and MPI_Psend_init are declared right next to each
        other in mpi-abi-stubs' vendored mpi.h (NOTES.md #1), and the receive
        form's rank parameter is still spelled `dest` there -- a copy-paste
        slip in the upstream ABI stub, not this project's own code, and
        harmless to a caller since C binds arguments positionally rather than
        by name.""",
    "MPI_F08_status (A.3) vs MPI_F08_Status (ours)": """
        MPI-5.0 Section 20.4 says outright that "the functions defined in
        Section 19.3.4 and Section 19.3.5 as well as MPI_F08_Status are not
        part of this ABI" -- so the type these two converters take is this
        project's own (NOTES.md #2 and doc/mpi.h.patch), coined for something
        the standard deliberately left out, and it owes the appendix's
        lowercase mpi_f08-module spelling nothing.""",
}

# TYPE mismatches scoped to one (entry point, parameter) pair rather than a
# blanket rule, since the naming difference genuinely is that narrow.
TYPE_EXEMPT = {
    ("MPI_Status_c2f08", "f08_status"): "MPI_F08_status (A.3) vs MPI_F08_Status (ours)",
    ("MPI_Status_f082c", "f08_status"): "MPI_F08_status (A.3) vs MPI_F08_Status (ours)",
}

# Parameter renames that hold everywhere they occur (the libc-collision
# dodge and MPI_T's shortened session name), applied before comparing a
# shared entry point's argument order. Keyed by the appendix's spelling.
PARAM_RENAMES = {
    "index": ("indx", "'index' renamed 'indx' to dodge libc's index()"),
    "pe_session": ("session", "MPI_T's 'pe_session' renamed 'session'"),
    "err": ("error", "MPI_Status_get_error / _set_error's 'err' renamed 'error'"),
}

# A rename scoped to one entry point rather than a blanket rule, for the one
# case that is not a naming convention but a specific upstream slip.
FUNC_PARAM_RENAMES = {
    ("MPI_Precv_init", "source"):
        ("dest", "MPI_Precv_init's 'source' kept its send-side template's 'dest'"),
}


def canon_param(func, name):
    """(header-spelling, exemption-key-or-None) for one appendix parameter."""
    over = FUNC_PARAM_RENAMES.get((func, name))
    if over:
        return over
    renamed = PARAM_RENAMES.get(name)
    if renamed:
        return renamed
    return name, None


def compare(std, ours, where, what):
    """Compare one {name: (ret, order, params)} map against the other's.

    Returns (problems, passed_over) the way mpif's `compare` does: a count of
    genuine divergences, and a dict of which named exemption absorbed which.
    """
    problems = 0
    passed_over = {}

    def over(kind):
        passed_over[kind] = passed_over.get(kind, 0) + 1

    shared = sorted(set(std) & set(ours))
    only_std = sorted(set(std) - set(ours))
    only_ours = sorted(set(ours) - set(std))
    print(f"{len(std)} in {where}, {len(ours)} {what}, {len(shared)} in both")

    for name in only_std:
        if PREDEFINED_CALLBACK_CONSTANT.match(name):
            over("predefined callback constants, not entry points")
        elif name in ("MPI_Status_f082f", "MPI_Status_f2f08"):
            over("MPI_Status_f082f / MPI_Status_f2f08 not part of this ABI")
        else:
            problems += 1
            print(f"MISSING {name}: in {where}, not {what}")

    for name in only_ours:
        if name in ("MPI_Wtime", "MPI_Wtick", "MPI_Aint_add", "MPI_Aint_diff"):
            over("missing entirely from A.3")
        else:
            problems += 1
            print(f"EXTRA   {name}: {what}, not in {where}")

    for name in shared:
        s_ret, s_order, s_params = std[name]
        o_ret, o_order, o_params = ours[name]

        canon = [canon_param(name, p) for p in s_order]
        s_order_mapped = [c[0] for c in canon]
        if s_order_mapped != o_order:
            problems += 1
            print(f"ARGS   {name}: {where} {s_order}, {what} {o_order}")
            continue
        for p, (_, kind) in zip(s_order, canon):
            if kind is not None:
                over(kind)

        if s_ret != o_ret:
            problems += 1
            print(f"RET    {name}: {where} says {s_ret}, {what} says {o_ret}")
        for appendix_name, header_name in zip(s_order, o_order):
            if s_params[appendix_name] != o_params[header_name]:
                exempt = TYPE_EXEMPT.get((name, header_name))
                if exempt is not None:
                    over(exempt)
                    continue
                problems += 1
                print(f"TYPE   {name} / {header_name}: {where} says "
                      f"{s_params[appendix_name]}, {what} says {o_params[header_name]}")

    return problems, passed_over


def main():
    pdf_path = Path(sys.argv[1]) if len(sys.argv) > 1 else REPO / "doc" / "mpi50-report.pdf"
    if not pdf_path.is_file():
        raise SystemExit(f"{pdf_path}: not found. Keep a copy of the MPI-5.0 standard there.")

    appendix, appendix_problems = parse_appendix(pdf_path)
    header, header_problems = parse_header(HEADER)

    problems = 0
    for p in appendix_problems + header_problems:
        print(p)
        problems += 1

    # MPI_* against the appendix directly; PMPI_* against the same appendix
    # entries under their MPI_ twin's name, since the appendix gives no PMPI
    # bindings at all -- "the same declarations as their twin" is the only
    # thing there is to check about the PMPI half, and it is the thing
    # MPI-5.0 §20.2.1 depends on.
    mpi_header = {n: v for n, v in header.items() if n.startswith("MPI_")}
    pmpi_header = {n[1:]: v for n, v in header.items() if n.startswith("PMPI_")}

    passed_over = {}
    for where, ours, what in [
        (appendix, mpi_header, "in gen/include/mpi.h"),
        (appendix, pmpi_header, "in gen/include/mpi.h's PMPI_ forms, P stripped"),
    ]:
        p, over = compare(where, ours, "A.3", what)
        problems += p
        for k, v in over.items():
            passed_over[k] = passed_over.get(k, 0) + v
        print()

    # The header's own MPI_/PMPI_ pair has to agree with itself: the appendix
    # gives no PMPI bindings, but the header's contract (MPI-5.0 §20.2.1) is
    # that a PMPI_ call is indistinguishable from its MPI_ twin.
    only_mpi = sorted(set(mpi_header) - set(pmpi_header))
    only_pmpi = sorted(set(pmpi_header) - set(mpi_header))
    if only_mpi or only_pmpi:
        problems += len(only_mpi) + len(only_pmpi)
        print(f"ASYMMETRIC MPI_ without PMPI_: {only_mpi}")
        print(f"ASYMMETRIC PMPI_ without MPI_: {only_pmpi}")
    for name in sorted(set(mpi_header) & set(pmpi_header)):
        if mpi_header[name] != pmpi_header[name]:
            problems += 1
            print(f"TWIN   {name}: MPI_ and PMPI_ forms disagree")

    print()
    for kind in sorted(passed_over):
        print(f"{passed_over[kind]} {kind}, as expected:{EXPECTED[kind]}")
    unexplained = sorted(set(EXPECTED) - set(passed_over))
    if unexplained:
        # An exemption that no longer fires is a stale exemption, and the
        # divergence it used to cover would go unreported if it came back
        # differently shaped.
        problems += len(unexplained)
        print(f"STALE  no longer diverges, so drop from EXPECTED: {unexplained}")

    if problems > 0:
        print(f"\n{problems} unexplained divergences")
        return 1
    print("\nno unexplained divergences")
    return 0


if __name__ == "__main__":
    sys.exit(main())

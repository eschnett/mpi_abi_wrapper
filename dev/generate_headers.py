#!/usr/bin/env python3
"""Generate gen/include/mpi.h and gen/include/mpiabi.h.

This is *not* the S2 generator (dev/ generator core, argument classes, wrapper
bodies) -- it is the narrower S0 step: apply doc/mpi.h.patch to the vendored
mpi-abi-stubs header and derive the MPIABI_ renamed view from the result, per
NOTES.md #2 ("Naming, and the renaming rules for mpiabi.h").

Usage: dev/generate_headers.py [--check]

Without --check, (re)writes gen/include/mpi.h, gen/include/mpiabi.h and
dev/entrypoints.txt. With --check, regenerates in memory and compares against
the committed files without writing anything, exiting non-zero on any
difference -- the "empty diff on regeneration" discipline of NOTES.md #3.
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VENDORED_STUB = ROOT / "dev" / "vendor" / "mpi-abi-stubs" / "mpi.h"
PATCH = ROOT / "doc" / "mpi.h.patch"
OUT_MPI_H = ROOT / "gen" / "include" / "mpi.h"
OUT_MPIABI_H = ROOT / "gen" / "include" / "mpiabi.h"
OUT_ENTRYPOINTS = ROOT / "dev" / "entrypoints.txt"

GENERATED_NOTICE_MPI_H = """\
/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate_headers.py from dev/vendor/mpi-abi-stubs/mpi.h
 * (see dev/vendor/mpi-abi-stubs/VERSION.md) with doc/mpi.h.patch applied.
 * Names are untouched -- this is the ABI's own mpi.h, byte-for-byte what an
 * application compiles and links against. See NOTES.md #2 and #3.
 */
"""

GENERATED_NOTICE_MPIABI_H = """\
/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate_headers.py from gen/include/mpi.h by renaming
 * typedef names, macro names and enumerator names from MPI_ to MPIABI_.
 * Struct/enum *tags* and struct *members* are left untouched, UNLESS a tag's
 * spelling is identical to its own typedef name (MPI_T_cb_safety,
 * MPI_T_source_order): those two are real MPI-standard tag names that a
 * conforming implementation's own <mpi.h> also declares, so leaving them
 * unrenamed would redeclare the same tag with different enumerators the
 * moment this header and an implementation's mpi.h are included in the same
 * translation unit (which is exactly what libmpiwrapper does). Renaming both
 * occurrences avoids that collision; see NOTES.md #2.
 *
 * Function prototypes are dropped entirely: libmpiwrapper calls the
 * implementation, never the ABI. See NOTES.md #2 and #3.
 */
"""

INCLUDE_GUARD = "MPIABI_H"

# One identifier per entry point, e.g. "MPI_Send" -- the "688 entry points"
# tally of NOTES.md #1. Populated by extract_entrypoints().
_ENTRYPOINT_RE = re.compile(
    r"^\s*[A-Za-z_][\w ]*?\**\s*"
    r"(?P<name>P?MPI_[A-Za-z0-9_]+)"
    r"\s*\([^;{]*\)\s*;\s*(/\*.*\*/)?\s*$"
)


def apply_patch() -> str:
    proc = subprocess.run(
        ["patch", "--quiet", "-p1", "-o", "-", str(VENDORED_STUB)],
        input=PATCH.read_bytes(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=ROOT,
    )
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr.decode())
        raise SystemExit(f"patch failed with exit code {proc.returncode}")
    return proc.stdout.decode()


def is_prototype_line(line: str) -> bool:
    stripped = line.strip()
    if not stripped or stripped.startswith(("typedef", "#", "/*", "*", "enum")):
        return False
    return _ENTRYPOINT_RE.match(line) is not None


def extract_entrypoints(mpi_h_text: str):
    """Return (mpi_names, pmpi_names), each a sorted list of base names."""
    mpi_names = set()
    pmpi_names = set()
    for line in mpi_h_text.splitlines():
        if not is_prototype_line(line):
            continue
        m = _ENTRYPOINT_RE.match(line)
        name = m.group("name")
        if name.startswith("PMPI_"):
            pmpi_names.add(name[len("PMPI_"):])
        else:
            mpi_names.add(name[len("MPI_"):])
    return sorted(mpi_names), sorted(pmpi_names)


def check_symmetry(mpi_names, pmpi_names):
    mpi_set, pmpi_set = set(mpi_names), set(pmpi_names)
    only_mpi = mpi_set - pmpi_set
    only_pmpi = pmpi_set - mpi_set
    if only_mpi or only_pmpi:
        msg = ["MPI_*/PMPI_* asymmetry:"]
        for n in sorted(only_mpi):
            msg.append(f"  MPI_{n} has no PMPI_{n}")
        for n in sorted(only_pmpi):
            msg.append(f"  PMPI_{n} has no MPI_{n}")
        raise SystemExit("\n".join(msg))
    if len(mpi_set) != 688:
        raise SystemExit(
            f"expected exactly 688 entry points, got {len(mpi_set)}"
        )


# --- mpiabi.h derivation -----------------------------------------------

_DEFINE_RE = re.compile(r"^#define\s+(MPI_[A-Za-z0-9_]+)")
# Function-pointer typedefs: `typedef RET (NAME)(args...);`
_TYPEDEF_FUNPTR_RE = re.compile(r"^typedef\b.*\(\s*(MPI_[A-Za-z0-9_]+)\s*\)\s*\(")
# `typedef enum [TAG] { ... } NAME;` on one line, or the closing `} NAME;` of
# a multi-line one -- this header only uses the one-line-per-member form, but
# the closing brace line is what carries the introduced typedef name.
_TYPEDEF_ENUM_CLOSE_RE = re.compile(r"^\}\s*(MPI_[A-Za-z0-9_]+)\s*;\s*$")
# Ordinary/alias typedefs: `typedef BASE NAME;` (BASE has no parens/braces).
_TYPEDEF_PLAIN_RE = re.compile(r"^typedef\s+[\w \*]+?\b(MPI_[A-Za-z0-9_]+)\s*;\s*$")
# Enumerator members: leading identifier of a line inside an `enum { ... };`
# block, before `=` or `,`. MPIX_ is the one non-"MPI_" spelling the header
# uses (MPIX_TYPECLASS_LOGICAL, a legacy alias sitting in the same anonymous
# enum as the MPI_TYPECLASS_* family) and still needs a distinct rename, or
# it would appear unrenamed in mpiabi.h and collide with mpi.h's own
# definition of the same enumerator when both are included together.
_ENUM_MEMBER_RE = re.compile(r"^\s*(MPI_[A-Za-z0-9_]+|MPIX_[A-Za-z0-9_]+)\s*(=|,|$)")

# Section-banner comments that introduce nothing but prototype blocks; with
# every prototype dropped they would be left as orphaned headings.
_SKIP_BANNERS = {
    "/* MPI functions */",
    "/* MPI_T functions */",
    "/* PMPI functions */",
    "/* PMPI_T functions */",
}


# Names that must be spelled *identically* in both views, because they
# coordinate the two rather than naming anything in the API. There is exactly
# one: the include guard around `struct MPI_ABI_Status`. Both headers define
# that struct -- it is the only ABI type whose members either side needs, since
# the handle tags stay incomplete -- and in a translation unit that includes
# both (src/mpi_abi/, and test/compile_both_headers.c on purpose) the second
# definition has to be skipped. Renaming the guard would give the two headers
# different guards, and then both would define the struct and neither would
# compile alongside the other.
KEEP_UNRENAMED = {"MPI_ABI_STATUS_DEFINED"}


def rename(name: str) -> str:
    """MPI_X -> MPIABI_X (so MPI_ABI_VERSION -> MPIABI_ABI_VERSION, unchanged
    but for the prefix -- no collision with plain MPI_VERSION ->
    MPIABI_VERSION, since the two source names differ); MPIX_X -> MPIABIX_X
    (NOTES.md #2)."""
    if name.startswith("MPIX_"):
        return "MPIABIX_" + name[len("MPIX_"):]
    assert name.startswith("MPI_")
    return "MPIABI_" + name[len("MPI_"):]


def collect_rename_map(lines):
    names = set()
    in_enum = False
    for line in lines:
        stripped = line.strip()
        # Matches both the plain `enum {` blocks and the two named
        # `typedef enum NAME {` ones (MPI_T_cb_safety, MPI_T_source_order).
        if stripped.endswith("{") and re.match(r"^(typedef\s+)?enum\b", stripped):
            in_enum = True
            continue
        if in_enum:
            m = _TYPEDEF_ENUM_CLOSE_RE.match(stripped)
            if m:
                names.add(m.group(1))
                in_enum = False
                continue
            if stripped.startswith("};"):
                in_enum = False
                continue
            m = _ENUM_MEMBER_RE.match(stripped)
            if m:
                names.add(m.group(1))
            continue

        m = _DEFINE_RE.match(stripped)
        if m:
            names.add(m.group(1))
            continue
        m = _TYPEDEF_FUNPTR_RE.match(stripped)
        if m:
            names.add(m.group(1))
            continue
        m = _TYPEDEF_ENUM_CLOSE_RE.match(stripped)
        if m:
            names.add(m.group(1))
            continue
        m = _TYPEDEF_PLAIN_RE.match(stripped)
        if m:
            names.add(m.group(1))
            continue
    return {name: rename(name) for name in names - KEEP_UNRENAMED}


def substitute(text: str, rename_map: dict) -> str:
    if not rename_map:
        return text
    pattern = re.compile(
        r"\b(" + "|".join(re.escape(k) for k in sorted(rename_map, key=len, reverse=True)) + r")\b"
    )
    return pattern.sub(lambda m: rename_map[m.group(1)], text)


# mpi.h defines MPI_Aint/MPI_Offset/MPI_Count through a scaffolding idiom:
# `#if !defined(MPI_ABI_X) / #define MPI_ABI_X <rhs> / #endif / typedef
# MPI_ABI_X MPI_X; / #undef MPI_ABI_X`. Both names rename to the *same*
# spelling here (MPI_ABI_Aint and MPI_Aint both collapse to MPIABI_Aint), so
# a line-by-line rename turns the typedef into `typedef MPIABI_Aint
# MPIABI_Aint;` -- the preprocessor macro-expands *both* occurrences (it does
# not know one was meant to survive as the new name), so the type is never
# actually introduced, and MPI_ABI_Count's build breaks one link down the
# chain. Resolved directly instead, using the header's own default (no
# `-D` override) branch -- mpiabi.h is generated fresh per build, so there is
# no separate configure step for it to preserve an override hook for.
_SCAFFOLD_RE = re.compile(
    r"#if !defined\(MPI_ABI_(\w+)\)\s*\n"
    r"#define MPI_ABI_\1\s+(.+?)\s*\n"
    r"#endif\s*\n"
    r"typedef MPI_ABI_\1 MPI_\1;\s*\n"
    r"#undef\s+MPI_ABI_\1"
)


def resolve_scaffolding_typedefs(text: str) -> str:
    def repl(m):
        name, rhs = m.group(1), m.group(2)
        rhs = re.sub(r"\bMPI_[A-Za-z0-9_]+\b", lambda r: rename(r.group(0)), rhs)
        return f"typedef {rhs} {rename('MPI_' + name)};"

    return _SCAFFOLD_RE.sub(repl, text)


def build_mpiabi_h(mpi_h_text: str) -> str:
    mpi_h_text = resolve_scaffolding_typedefs(mpi_h_text)
    lines = mpi_h_text.splitlines()

    # Drop the mpi.h include guard (own guard emitted separately) and the
    # top-level #include, which mpiabi.h re-states itself.
    body_lines = []
    for line in lines:
        stripped = line.strip()
        if stripped in ("#ifndef MPI_H_ABI", "#define MPI_H_ABI") or stripped.startswith(
            "#endif  /* MPI_H_ABI */"
        ) or stripped.startswith("#endif /* MPI_H_ABI */"):
            continue
        if stripped == "#include <stdint.h>":
            continue
        if stripped in _SKIP_BANNERS:
            continue
        if is_prototype_line(line):
            continue
        body_lines.append(line)

    rename_map = collect_rename_map(body_lines)
    # The scaffolding resolved above (MPI_Aint/MPI_Offset/MPI_Count) no longer
    # appears in `typedef MPI_ABI_X MPI_X;` form for collect_rename_map to
    # find, but the bare names are still referenced elsewhere in the header
    # (e.g. `MPI_Aint *extent`) and must still be renamed there.
    rename_map.update(
        {n: rename(n) for n in ("MPI_Aint", "MPI_Offset", "MPI_Count")}
    )
    body = "\n".join(body_lines)
    body = substitute(body, rename_map)

    # Collapse runs of 3+ blank lines left behind by dropped prototype blocks.
    body = re.sub(r"\n{3,}", "\n\n", body)
    body = body.strip("\n") + "\n"

    out = []
    out.append(GENERATED_NOTICE_MPIABI_H)
    out.append(f"#ifndef {INCLUDE_GUARD}")
    out.append(f"#define {INCLUDE_GUARD}")
    out.append("")
    out.append("#include <stdint.h>")
    out.append("")
    out.append(body)
    out.append(f"#endif /* {INCLUDE_GUARD} */")
    return "\n".join(out) + "\n"


def main():
    check = "--check" in sys.argv

    patched = apply_patch()
    mpi_h_out = GENERATED_NOTICE_MPI_H + "\n" + patched

    mpi_names, pmpi_names = extract_entrypoints(patched)
    check_symmetry(mpi_names, pmpi_names)

    mpiabi_h_out = build_mpiabi_h(patched)

    entrypoints_out = "\n".join(mpi_names) + "\n"

    targets = {
        OUT_MPI_H: mpi_h_out,
        OUT_MPIABI_H: mpiabi_h_out,
        OUT_ENTRYPOINTS: entrypoints_out,
    }

    if check:
        failed = []
        for path, content in targets.items():
            existing = path.read_text() if path.exists() else None
            if existing != content:
                failed.append(str(path))
        if failed:
            raise SystemExit(
                "regeneration differs from committed output: " + ", ".join(failed)
            )
        print(f"OK: {len(mpi_names)} entry points, MPI_/PMPI_ symmetric, "
              "regeneration matches committed output")
        return

    for path, content in targets.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    print(f"wrote {OUT_MPI_H}")
    print(f"wrote {OUT_MPIABI_H}")
    print(f"wrote {OUT_ENTRYPOINTS}")
    print(f"{len(mpi_names)} entry points, MPI_/PMPI_ symmetric")


if __name__ == "__main__":
    main()

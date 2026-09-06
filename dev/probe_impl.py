#!/usr/bin/env python3
"""What does *this* implementation actually have?

Writes `mpiwrapper_impl_config.h`, defining MPIWRAPPER_HAVE_<name> for every
entry point and every optional constant the implementation declares. The
generated sources guard on those and nothing else: an entry point that is
missing gets decision 6's run-time-reporting stub, and an optional constant
that is missing drops out of its conversion table.

**Nothing is guarded on the implementation's own spelling of a name.** That was
the obvious way to write it and it is quietly wrong: `#ifdef MPI_COMBINER_NAMED`
is *false* on MPICH, which spells that family as enumerators, as Open MPI does
for MPI_THREAD_*, MPI_COMM_TYPE_* and MPI_IDENT. A false `#ifdef` there does not
fail the build -- it drops the case, reaches the default arm, and passes an
unmapped value through. Measured, not hypothetical: MPICH 4.3.1 has
MPI_COMBINER_VALUE_INDEX as `= 20` in an enum, and an `#ifdef` on it answers no.

**Why not a version test.** `#if MPI_VERSION >= 4` was the obvious spelling and
is wrong in both directions. Open MPI 5.0.10 reports MPI-3.1 and has sessions
and partitioned communication, so a version test would stub what is there; and
the ABI is MPI-5.0 while the enforced floor is MPI-3.0 (decision 3), so the gap
is 200-odd entry points rather than a handful. Asking the implementation's own
header is exact, and it is the same header the wrapper bodies are compiled
against.

**Why not nm.** The library's symbol table misses what the header provides as a
macro -- Open MPI defines MPI_Aint_add that way -- and a macro is perfectly
callable. The compiler is the only thing that answers the question the
generated code actually asks.

**What gets probed** is read out of the sources that guard: every
`MPIWRAPPER_HAVE_<name>` they mention, and nothing else. So what needs asking
is decided by the code that asks it, and the two cannot drift apart.

That is `gen/mpiwrapper/` *and* `src/mpiwrapper/`. It used to be the generated
pair alone, on the reasoning that the generator decides -- which stopped being
true in S4a: a hand-written body is subject to decision 6 exactly like a
generated one, and its guard was silently false while the probe was not
reading the file it appeared in. A guard nobody probes is not a guard; it is a
permanently-taken `#else` branch that reports
MPI_ERR_UNSUPPORTED_OPERATION for an entry point the implementation has.

**How, and what it costs.** All of them go into *one* translation unit, one
probe per line, compiled without linking -- not one configure test per name.
`-fsyntax-only` where the compiler has it and `-c` where it does not, decided
by `compile_only_flags` below rather than assumed; nvc has no `-fsyntax-only`.
A name that is a macro answers `#ifdef`. Anything else has to be declared for
its probe to compile, and the probe differs by what the name is: `sizeof &name`
for an entry point, which is a function, and `sizeof(name)` for a constant,
which may be an enumerator whose address cannot be taken.

The constant form answers for a *type* as well, and that is used rather than
merely tolerated: `sizeof (MPI_T_event_registration)` is valid C exactly when
the typedef exists, so a handle class with no constant to test -- the ABI names
no MPI_T_EVENT_REGISTRATION_NULL -- is probed by its type name. Open MPI 5.0.6
is why it matters: it has cvars, pvars and enums and declares neither event
type, so the guard cannot be MPI_T-as-a-whole. If the compile fails,
the diagnostics' *line numbers* -- never their wording -- say which probes are
bad; those are dropped and the file is compiled again, so the answer is always
confirmed by a compile that succeeded. Measured at 316 ms for 732 guards
(1350 spellings) against native Open MPI 5.0.6, two or three rounds;
dev/gpu-query/timeprobe.py is the measurement.

One question is asked *before* those and separately: whether an optional
*header* compiles. OPTIONAL_HEADERS below has the reason -- Open MPI declares
two of decision 28's names in <mpi-ext.h> -- and probe() has why it cannot ride
along in the main translation unit.

Everything here is compile-only, so it works when cross-compiling (NOTES.md
#9).

Usage:
  dev/probe_impl.py --cc CC --out FILE [--flag F]... [--entrypoints F]
"""

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ENTRYPOINTS = ROOT / "dev" / "entrypoints.txt"
DEFAULT_SOURCES = ([ROOT / "gen" / "mpiwrapper" / "wrappers.c",
                    ROOT / "gen" / "mpiwrapper" / "constants.c"]
                   + sorted((ROOT / "src" / "mpiwrapper").glob("*.c")))

# Two diagnostic shapes, because this probe works by reading *which line* the
# compiler complained about, and compilers do not agree on how to say it:
#
#   gcc, clang   probe.c:10:5: error: ...
#   nvc          "probe.c", line 10: error: ...
#
# The second is the EDG frontend's, which nvc uses and Cray's compiler and the
# classic icc do too. Not recognising it does not make the probe wrong, it
# makes it *stop*: a diagnostic it cannot place reads as "no diagnostic pointed
# at a probe line", which is the hard error below, and nvc produced exactly
# that for seven names that were simply absent from MPICH -- MPI_ERR_ABI,
# MPIX_TYPECLASS_LOGICAL and the five sized MPI_LOGICAL* types.
#
# The two patterns cannot match each other's output: the first needs a colon
# immediately before the line number, and EDG puts a space there.
_LINE_RES = (
    re.compile(r"^[^\s].*?:(\d+):(?:\d+:)?\s*(?:fatal\s+)?error:",
               re.MULTILINE),
    re.compile(r'^"[^"]*",\s*line\s+(\d+):\s*(?:\w+\s+)?error:',
               re.MULTILINE),
)


# Headers an implementation may or may not have, asked separately from the
# names in them. Open MPI declares MPIX_Query_cuda_support and
# MPIX_Query_rocm_support in <mpi-ext.h> rather than in <mpi.h> (NOTES.md #7
# decision 28), so a probe TU that includes only <mpi.h> answers "absent" for
# an implementation that has them.
#
# header -> the MPIWRAPPER_HAVE_<suffix> the wrapper sources test.
OPTIONAL_HEADERS = {"mpi-ext.h": "MPI_EXT_H"}


def probe_source(names, headers=()):
    """The probe TU, and a line -> name map for reading the diagnostics.

    `names` is a list of (name, kind), where kind is "function" for an entry
    point and "value" for a constant. The distinction is not cosmetic:
    `&MPI_COMBINER_VALUE_INDEX` does not compile where that constant is an
    enumerator, and `sizeof(MPI_Send)` does not compile at all.

    `headers` are the optional headers that were found to compile, included
    after <mpi.h> so the names they declare are visible. The line -> name map
    is built from the running list, so the probes' line numbers follow.
    """
    lines = [
        "/* Generated by dev/probe_impl.py; compiled, never run. */",
        "#include <mpi.h>",
    ]
    lines += [f"#include <{h}>" for h in headers]
    lines += [
        "int mpiwrapper_probe(void);",
        "int mpiwrapper_probe(void)",
        "{",
        "  int n = 0;",
    ]
    where = {}
    for name, kind in names:
        lines.append(f"#ifdef {name}")
        lines.append("  n += 1;")
        lines.append("#else")
        where[len(lines) + 1] = name
        probe = f"&{name}" if kind == "function" else f"({name})"
        lines.append(f"  n += (int)sizeof {probe};")
        lines.append("#endif")
    lines.append("  return n;")
    lines.append("}")
    return "\n".join(lines) + "\n", where


_compile_only = None


def compile_only_flags(cc, workdir):
    """How to ask *this* compiler to compile without linking.

    `-fsyntax-only` is GCC's spelling and Clang's, and is the cheapest: it
    stops before code generation. It is not universal. NVIDIA's nvc has no
    such switch and stops with

        nvc-Error-Unknown switch: -fsyntax-only

    which was a portability defect in this probe rather than in the
    implementation it was probing -- NOTES.md #3 keeps the probe compile-only
    so that the build stays cross-compilable, and says nothing about which
    compiler runs it. `-c` is the spelling every C compiler has, so it is the
    fallback, and the object goes to the caller's temporary directory rather
    than to /dev/null, which not every toolchain accepts as an output path.

    Probed once and remembered: the answer cannot change within a run, and the
    probe below compiles one translation unit per bisection step.
    """
    global _compile_only
    if _compile_only is None:
        src = Path(workdir) / "flagprobe.c"
        src.write_text("int main(void) { return 0; }\n")
        proc = subprocess.run([cc, "-fsyntax-only", str(src)],
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if proc.returncode == 0:
            _compile_only = ["-fsyntax-only"]
        else:
            _compile_only = ["-c", "-o", str(Path(workdir) / "flagprobe.o")]
    return _compile_only


def compile_ok(cc, flags, text, workdir):
    src = Path(workdir) / "probe.c"
    src.write_text(text)
    proc = subprocess.run([cc, *compile_only_flags(cc, workdir), *flags,
                           str(src)],
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return proc.returncode == 0, proc.stdout.decode("utf-8", "replace")


def accepts(cc, flag, workdir):
    ok, _ = compile_ok(cc, [flag], "int main(void) { return 0; }", workdir)
    return ok


def probe(cc, flags, names):
    """(the subset of `names` this implementation declares, the optional
    headers it has)."""
    with tempfile.TemporaryDirectory() as workdir:
        # Report every error in one pass where the compiler can; without it
        # the loop below simply takes more rounds.
        for flag in ("-ferror-limit=0", "-fmax-errors=0"):
            if accepts(cc, flag, workdir):
                flags = [*flags, flag]
                break

        # A *separate* compile, and it has to be: the loop below drops a
        # failing probe by the line number the diagnostic points at, and
        # `fatal error: 'mpi-ext.h' file not found` is reported at the
        # #include's own line, which is not a probe line. The loop would read
        # that as "no diagnostic pointed at a probe line" and stop with the
        # hard error below.
        #
        # The compiler is the oracle here as everywhere (HISTORY.md #1.19),
        # which is also why this is a compile rather than __has_include: a
        # stray mpi-ext.h from a *different* MPI earlier on the include path
        # exists without compiling against this one, and existence is not the
        # question (HISTORY.md #1.26).
        #
        # Same tempdir as the loop, deliberately: compile_only_flags caches an
        # `-o <workdir>/flagprobe.o` path in a module global on first use, so a
        # compile in a directory that is later removed would leave the loop
        # writing its object into nothing.
        headers = [h for h in OPTIONAL_HEADERS
                   if compile_ok(cc, flags,
                                 f"#include <mpi.h>\n#include <{h}>\n",
                                 workdir)[0]]

        present = list(names)
        for round_ in range(1, 41):
            text, where = probe_source(present, headers)
            ok, output = compile_ok(cc, flags, text, workdir)
            if ok:
                return {name for name, _ in present}, headers
            bad = {where[int(m.group(1))]
                   for line_re in _LINE_RES
                   for m in line_re.finditer(output)
                   if int(m.group(1)) in where}
            if not bad:
                sys.stderr.write(output)
                raise SystemExit(
                    "the probe failed for a reason that is not a missing name "
                    "(no diagnostic pointed at a probe line). The compiler "
                    "output is above; this is a real build problem rather "
                    "than an implementation without some MPI function.")
            present = [(n, k) for n, k in present if n not in bad]
    raise SystemExit("the probe did not converge in 40 rounds")


HEADER = """\
/* GENERATED at configure time by dev/probe_impl.py -- do not edit and do not
 * commit. Says which MPI entry points and which optional constants the
 * implementation this build tree was configured against actually declares.
 *
 * gen/mpiwrapper/wrappers.c emits a real body for an entry point defined here
 * and decision 6's run-time-reporting stub for one that is not;
 * gen/mpiwrapper/constants.c emits a conversion case for a constant defined
 * here and leaves it out otherwise.
 *
 * MPIWRAPPER_HAVE_MPI_EXT_H is the one guard here that is not a name: it says
 * that `#include <mpi.h>` followed by `#include <mpi-ext.h>` compiles, which
 * is what src/mpiwrapper/hw_gpu.c needs before it may include that header
 * (NOTES.md #7 decision 28). The names inside it were probed with it
 * included.
 */

#ifndef MPIWRAPPER_IMPL_CONFIG_H
#define MPIWRAPPER_IMPL_CONFIG_H

/* wrappers.c #errors without this: a missing probe would otherwise turn the
 * whole library into stubs, which links and loads and answers
 * MPI_ERR_UNSUPPORTED_OPERATION to everything.
 */
#define MPIWRAPPER_IMPL_PROBED 1

"""


def wanted(sources, entrypoints):
    """guard name -> the (spelling, kind) pairs that must all be declared.

    A *standard* entry point needs both its MPI_ and PMPI_ names, because the
    body macro is instantiated once against each (decision 7). Everything else
    is a constant, and is probed as a value rather than as a function.

    **An extension has no guaranteed profiling twin, so each spelling is its
    own guard.** Decision 7's premise -- both names always exist -- is a
    property of the standard's entry points: Open MPI declares
    MPIX_Query_cuda_support and no PMPIX_ twin of it, and requiring both would
    report the name it does have as absent. So MPIX_X and PMPIX_X are asked
    separately, and src/mpiwrapper/hw_gpu.c chooses per spelling.

    dev/entrypoints.txt holds full names, so a guard is matched against the
    spelling it names rather than reassembled from a base.
    """
    names = {ln.strip() for ln in entrypoints.read_text().split() if ln.strip()}
    out = {}
    for src in sources:
        for name in re.findall(r"MPIWRAPPER_HAVE_([A-Za-z_]\w*)",
                               src.read_text()):
            if name in OPTIONAL_HEADERS.values():
                # Answered by the pre-check in probe(), not by a probe line.
                # Left in, it would become `sizeof (MPI_EXT_H)`, which cannot
                # compile anywhere and costs a round to drop.
                continue
            if name.startswith("MPI_") and name in names:
                out[name] = ((name, "function"), ("P" + name, "function"))
            elif name.startswith("MPIX_") and name in names:
                out[name] = ((name, "function"),)
            elif name.startswith("PMPIX_") and name[1:] in names:
                out[name] = ((name, "function"),)
            else:
                out[name] = ((name, "value"),)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--flag", action="append", default=[])
    ap.add_argument("--entrypoints", default=str(DEFAULT_ENTRYPOINTS))
    ap.add_argument("--source", action="append", default=[],
                    help="a generated source to read MPIWRAPPER_HAVE_* from")
    args = ap.parse_args()

    sources = [Path(s) for s in args.source] or DEFAULT_SOURCES
    guards = wanted(sources, Path(args.entrypoints))
    if not guards:
        raise SystemExit("no MPIWRAPPER_HAVE_* names in " +
                         ", ".join(str(s) for s in sources))

    names = sorted({pair for spellings in guards.values() for pair in spellings})
    # A flag may arrive as one shell-quoted string from CMake's list handling.
    flags = [f for raw in args.flag for f in raw.split() if f]
    present, headers = probe(args.cc, flags, names)

    have = sorted(g for g, spellings in guards.items()
                  if all(n in present for n, _ in spellings))

    out = [HEADER]
    for header in headers:
        out.append(f"#define MPIWRAPPER_HAVE_{OPTIONAL_HEADERS[header]} 1")
    if headers:
        out.append("")
    for name in have:
        out.append(f"#define MPIWRAPPER_HAVE_{name} 1")
    out.append("")
    out.append("#endif /* MPIWRAPPER_IMPL_CONFIG_H */")
    Path(args.out).write_text("\n".join(out) + "\n")

    extra = (", ".join(headers) + " present") if headers else "no optional header"
    print(f"probed {len(guards)} names: {len(have)} available, "
          f"{len(guards) - len(have)} absent from this implementation; "
          f"{extra}")


if __name__ == "__main__":
    main()

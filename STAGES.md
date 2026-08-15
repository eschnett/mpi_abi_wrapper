# Implementation stages

Ten stages, each sized to roughly one working session, each ending with the
repository committed and green. `NOTES.md` is the design; this is only the order of
work and the contract between stages.

The contract matters more than the sizing. A stage is finished when its **exit check
runs and passes**, because that is what lets the next session trust it without
re-deriving it. Where a stage's exit check is weak, that is called out — those are
the stages where a session can leave plausible-looking damage behind.

## Dependency structure

```
S0 headers+skeleton
 ├─→ S1 prototype ──→ S2 generator core ──→ S3 remaining classes ─┐
 │        │                   │                                    ├─→ S7 MPICH suite ──→ S8 consumers
 │        │                   └─→ S4 the hand-written ~50 ─────────┘
 │        └─→ S6 packaging                                         └─→ S9 sanitizers/threads/32-bit
 └─→ S5 Appendix A.2 cross-check
```

The critical path is **S0 → S1 → S2 → S3 → S7 → S8**. **S5** needs only S0 and
**S6** needs only S1, so both float: keep them in reserve for when the critical path
is blocked. **S4** can overlap S3 once the ledger exists.

## The stages

### S0 — Repository skeleton and header generation *(done)*

Vendor the mpi-abi-stubs `mpi.h` and `apis.json`; apply `doc/mpi.h.patch`; generate
`gen/include/mpi.h` and `gen/include/mpiabi.h` (the `MPIABI_` view, renaming typedef,
macro and enumerator names but **not** struct tags or members — `NOTES.md` §2).
CMake skeleton and directory layout per §9.

**Exit check.** Both headers compile standalone in a TU of their own; the extracted
entry-point list is exactly **688**, `MPI_*` and `PMPI_*` symmetric with zero
asymmetry, committed as the first frozen tally.

**Model: Sonnet.** Fiddly text transformation with an exact numeric oracle — the
counts either match or they don't.

### S1 — The prototype, end to end *(done: 29 entry points)*

The whole vertical slice by hand: `src/mpi_abi/` bootstrap (dlopen with per-platform
isolation, the version/subversion/layout handshake, the outward-resolution check),
`src/mpiwrapper/` conversion runtime (perfect-hash reverse map, status blob, rank/tag,
error codes, bitmask, staging, op trampoline pool), a hand-written vtable header for
those slots, and the functions of §11. **S1 delivered 29 rather than the 15 planned**:
the original list was not testable on its own, and the isolation probe added one more.

**Exit check.** All 29 pass against **both** MPICH and Open MPI;
`mpiwrapper_selftest` passes including the dynamic-handle collision probe;
`dev/dlopen-probe`'s conclusions hold in the real library.

**Model: Opus.** The highest-judgement stage in the project and the one every later
stage is measured against. `NOTES.md` §11 explains why it comes before the generator:
designing the generator before the shape of its output is known is the main way this
goes wrong.

### S2 — Generator core and the mechanical argument classes

`dev/` generator: parse both inputs, classify, emit all seven artifacts. Cover
passthrough scalars, scalar handles in/out/inout, error codes, ranks, tags and the
other mapped integer constants. The `HAND_WRITTEN` ledger that fails if any of the 688
is neither generated nor listed.

**Exit check — the important one.** The generator **reproduces the S1 prototype**,
byte-identically or with a diff explained line by line. `libmpi_abi` links and `nm`
matches the header's 1376 symbols in both directions.

**Model: Opus.** Cross-cutting structure, and every later generator session inherits
its shape.

### S3 — The remaining argument classes

Arrays and staging with the three lifetime rules, status arrays, bitmasks, keyvals,
output-string buffers, sentinels, callbacks and trampoline pools, `MPI_T`.

**Exit check.** Every one of the 688 is generated or in `HAND_WRITTEN`; frozen tallies
per class; the "no ABI-typed parameter reaches the implementation call" assertion
passes over the emitted text; the whole thing compiles against both MPIs.

**Model: Opus.** Likely **two sessions** — split at arrays/status versus
callbacks/`MPI_T` if so.

### S4 — The hand-written ~50

Lifecycle, the callback registration functions, spawn, buffer attach, `MPI_Pcontrol`,
dynamic error codes, the 26 Fortran converters, the ten output-string functions.

**Exit check.** `HAND_WRITTEN` fully implemented; `MPI_ERR_UNSUPPORTED_OPERATION`
returned only for genuine implementation gaps, and `gen/report.txt` lists exactly
those.

**Model: Opus.** Per-function judgement against the standard, which is the definition
of this set. May run to two sessions.

### S5 — Oracle 4: Appendix A.2 cross-check *(floats; needs only S0)*

`dev/check-c-bindings.py`: parse the C bindings out of `doc/mpi50-report.pdf` with
`pdftotext -layout` and compare against the header. Keep mpif's two properties — the
parse validates itself, and exemptions are named and fail when they stop firing.

**Exit check.** Runs in CI; every one of the 688 signatures either matches or is a
named exemption; breaking one signature on purpose makes it fail.

**Model: Sonnet.** Well-specified with a precedent to follow
(`mpif/dev/check-f08-bindings.jl`).

### S6 — Build, packaging, CI matrix *(floats; needs only S1)*

`mpicc`/`mpicxx`, CMake package files plus the `FindMPI` shim, pkg-config,
`ci-scripts/` for pinned MPI source builds, the variant matrix. Respect the
`ci-scripts/` versus `ci-scripts/suite/` cache-key split (§9).

**Exit check.** Each of the three consumption routes **builds and runs** a program,
with `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH` cleared; `nm` confirms `libmpi_abi` is the
executable's only MPI dependency, per §20.2.1.

**Model: Sonnet.** Much of it is transposable from mpif's `ci-scripts/`.

### S7 — MPICH C test suite

Runner, `mpiexec` filter, per-variant expected-failure list with a reason on every
line. Expect some expected failures to be *build* failures where a test reaches for
MPICH internals or `MPIX_*`.

**Exit check.** The suite runs against both implementations; the xfail list is
committed with reasons; a variant's result matching its list is the gate.

**Model: Sonnet** for the harness, **Opus** for triage once real failures appear —
the harness is mechanical, deciding whether a failure is our bug or the test's
assumption is not.

### S8 — Consumer integration

HDF5, PETSc, mpi4py, mpif — each covering something the others do not (§10).

**Exit check.** Each project builds against a wrapper and passes its own suite;
every failure is triaged into either a bug of ours or a documented gap.

**Model: Opus.** This is where omissions that all five oracles pass will surface, and
the work is diagnosis rather than construction.

### S9 — Sanitizers, threads, 32-bit

ASan/UBSan with a suppression file documenting the by-design leaks (op slots,
errhandler slots, keyval pairs, datarep state); an `MPI_THREAD_MULTIPLE` stress test
over the pools and maps; the 32-bit row. Settle whether `RTLD_DEEPBIND` survives ASan
or whether the sanitizer jobs must select `dlmopen` (§13).

**Exit check.** Sanitizer jobs green with every suppression entry explained; the
thread stress test passes repeatedly; the 32-bit variant passes the same gates.

**Model: Opus.** Sanitizer output over an MPI is mostly triage, and an unexplained
suppression is a design change nobody noticed.

## Choosing a model, and the actual principle

The recommendations above follow one rule: **how cheap a model can safely do a stage
is set by how strong that stage's exit check is.** S0 and S5 have exact oracles — a
count, a signature comparison — so a wrong answer cannot survive the check, and a
mid-tier model is fine. S4 and S8 have weak checks in the sense that matters: a
plausible-but-wrong conversion compiles, links, passes the in-house tests, and
produces a wrong answer at 4096 ranks. That asymmetry, not raw difficulty, is why
those want the strongest model available.

This is a judgement call rather than a measurement, and it is worth revisiting after
S2: if the generator's assertions turn out to be as tight as intended, S3 may drop to
Sonnet, which would be the single biggest cost saving available here.

## Session hygiene

- **Read only the `NOTES.md` sections a stage names.** The file is long; loading all
  of it wastes the context the work needs.
- **Do not re-litigate the numbered decisions.** They have reasons recorded, several
  of them measured. Reopen one only with a new argument or a new measurement, and if
  you do, update the decision rather than working around it.
- **New findings go in `NOTES.md`, new measurements in `dev/`.** Four separate
  results in this design came from probes that now live there; a claim in a commit
  message is a claim nobody will find again.
- **Prefer a benchmark or a probe to an argument** for anything performance- or
  loader-related, and **check any benchmark against its own disassembly** — two of
  the three benchmarks here reported confidently wrong numbers first, once from
  thermal drift and once because the compiler devirtualized what was being measured.
- **End committed and green.** A stage that leaves the tree red has not produced
  something the next session can build on.

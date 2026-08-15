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
 │        │                   └─→ S4 the hand-written ~90 ─────────┘
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

### S1 — The prototype, end to end *(done: 29 entry points, 58 slots)*

The whole vertical slice by hand: `src/mpi_abi/` bootstrap (dlopen with per-platform
isolation, the version/subversion/layout handshake, the outward-resolution check and
the behavioural probe), `src/mpiwrapper/` conversion runtime (perfect-hash reverse
map, status blob, rank/tag, error codes, bitmask, staging, op trampoline pool), a
hand-written vtable header for those slots, and the functions of §11. **S1 delivered
29 rather than the 16 planned**: the original list was not testable on its own, and
the isolation probe added `MPI_Get_version`. Nine are hand-written, twenty are in the
generator-shaped `wrappers.c`, and those twenty are what S2 must reproduce.

**Exit check.** All 29 pass against **both** MPICH and Open MPI — MPICH 4.3.1 at
two ranks, Open MPI 5.0.6 as a singleton, since no Open MPI 5.0.x launcher works
on macOS 26 (`NOTES.md` §11 and `test/README.md` say so rather than leaving it to
be inferred from a green run); `mpiwrapper_selftest` passes including the
dynamic-handle collision probe; `dev/dlopen-probe`'s conclusions hold in the real
library.

**Model: Opus.** The highest-judgement stage in the project and the one every later
stage is measured against. `NOTES.md` §11 explains why it comes before the generator:
designing the generator before the shape of its output is known is the main way this
goes wrong.

### S2 — Generator core and the mechanical argument classes *(done: 473 generated, 120 in the ledger, 95 deferred)*

`dev/generate.py`: parse both inputs, classify, emit all seven artifacts. The
`HAND_WRITTEN` ledger fails if any of the 688 is neither generated nor listed,
and `gen/report.txt` names all 688 with the reason for each.

The classes it covers went past the plan, because the exit check demanded it:
buffer sentinels, the mode bitmasks and the scalar out-status are on S3's list
below, and without them `MPI_Send`, `MPI_File_open` and `MPI_Recv` — three of
the twenty bodies S2 has to reproduce — could not be generated at all. It also
covers passthrough arrays and in-direction arrays needing element-wise
conversion, which cost nothing beyond `MPI_Type_create_struct`'s tested shape.
What it does **not** touch is anything with a lifetime question: out and inout
arrays, status arrays, and temporaries outliving their call.

Two things S2 had to build that this plan did not name, both in `NOTES.md` §3:
**`dev/probe_impl.py`**, because decision 6's `#ifdef` stubs need
something to test and nothing available at build time answers correctly — a
version test under-reports, `nm` over-reports, and `#ifdef` on a constant's own
name is *silently* false wherever an implementation spells it as an enumerator,
which both of them do; and
**`dev/check_prototype.py`** over a frozen `dev/s1-reference/`, so that the exit
check keeps running instead of having been asserted once.

**Exit check — the important one.** The generator **reproduces the S1
prototype**: 194 items, 190 exactly, 4 exempted with a reason in
`dev/check_prototype.py` (uniform out-rank mapping, a staging-buffer name, and
`MPI_Waitall`, whose class is S3's). `libmpi_abi` links and `nm` matches the
header's 1376 symbols in both directions, checked *both* ways.

**Model: Opus.** Cross-cutting structure, and every later generator session inherits
its shape.

### S3 — The remaining argument classes

Out and inout arrays with the three lifetime rules, status arrays, keyvals,
output-string buffers, callbacks and trampoline pools, `MPI_T`. Bitmasks and
sentinels moved forward into S2, above. **The list is no longer a matter of
memory: `gen/report.txt` names all 95 deferred entry points with the class that
blocks each**, and the generator fails when one stops being deferred without
being reclassified.

Staging includes the eight `*alltoallw*` forms whose temporaries outlive the
call, which §8 used to list as hand-written; `MPI_Ialltoallw` in
`handwritten.c` is their template, and S3 generates all eight and deletes it.
Their arrays are also the shape S2 could not size: the length is the
communicator's size rather than a parameter, so the body has to call
`PMPI_Comm_size` first. `MPI_Waitall` is in `handwritten.c` for the same kind
of reason and leaves the same way; `dev/check_prototype.py` fails when it does,
asking for its exemption to be deleted.

Also S3's, and a ledger item rather than a matter of memory: *every* completion
entry point must call `mpiwrapper_staged_release`, not just the ones S1 wrote
(`NOTES.md` §6.3).

#### S3a — arrays, statuses and lifetimes *(done: 518 generated, 118 in the ledger, 52 deferred)*

The first of the two sessions took the array half: out, inout and status
arrays, the extents `apis.json` records as `*`, the graph topologies, the
`ranges` triplet, the four `*_external` packing forms and the six pure
ABI-side status accessors. Both stand-ins came back out of the ledger, and the
release rule is emitted at all eleven entry points with an inout request.

Three things it settled that this plan did not name, all in `NOTES.md` §3's
"What S3's first half settled": the **lifetime rules need no flag** in the
request map, because "the implementation nulled the handle" already
distinguishes a completed nonblocking request from a merely-completed
persistent one — one emitted rule, both lifetimes; **`src/mpiwrapper/
extents.c`**, because an extent `apis.json` gives as `*` is a property of an
object and the only place to ask is the implementation; and
**`dev/get-contents-extent/`**, because Open MPI 5.0.6 dereferences the whole
of `MPI_Type_get_contents`' `max_datatypes` and segfaults on a legal program,
which is why the wrapper passes the envelope's count instead.

`test/abi_arrays_test.c` is the behavioural half, and it is what covers the
lifetime question that no assertion in the generator can see: a persistent
`MPI_Alltoallw` started three times, and 1200 create/free cycles against a
1024-entry table.

#### S3b — keyvals, output strings, callbacks, `MPI_T` *(done: 565 generated, 118 in the ledger, 5 on the ABI side, 0 deferred)*

The second session took the last 52: the 16 keyval entry points, the
output-string buffers with an explicit length, `MPI_Info_create_env`, and the
whole tool interface. **Every one of the 688 is now generated or in the
ledger.** The "deferred" tally stays at a frozen *zero*, so a future
`apis.json` or ABI header carrying a class the generator cannot place fails
there rather than quietly emitting one more stub.

Four things it settled that this plan did not name, all in `NOTES.md` §3's
"What S3's second half settled": **where the callback boundary falls**, as a
rule rather than a list — a callback-*typed parameter*, which puts
`MPI_T_event_handle_free` in the ledger beside the two registrars and leaves
the two `MPI_T_event_callback_*_info` calls generated, since a
`CALLBACK_SAFETY` is an enumerator and not a function; **MPI_T's null OUT
pointers**, which every one of its five query functions permits and which make
each converted write-back conditional — hoisted into a declared local rather
than written `abi_x ? &x : NULL` in the call, so that the "no ABI-typed
parameter reaches the implementation call" assertion stays a grep;
**`src/mpiwrapper/keyvals.c`**, because the ABI-side value of a dynamic keyval
is ours to choose and choosing it from a high base puts §5.6's collision beyond
reach by construction; and **`src/mpiwrapper/toolobj.c`**, because
`obj_handle`'s class is not in its own argument list — it is whatever a prior
`get_info` reported in `bind`, so the wrapper asks first.

**A fifth thing it settled, after the exit check had already passed:** the five
entry points MPI-3.0 *deleted* are answered by `libmpi_abi` itself, in terms of
the MPI-2 functions that replaced them, with no vtable slot and no wrapper body.
Open MPI main's `libmpi_abi` declares all 688 and defines 683, and those five
are the difference — so forwarding them through a slot is a *link* failure of
the whole wrapper rather than decision 6's run-time report, because the probe
asks the compiler and the compiler sees the declaration. They now work over any
implementation with the MPI-2 attribute interface. `MPI_Keyval_create` left the
ledger in the process, since it is `MPI_Comm_create_keyval` under an older name.
`NOTES.md` §3 has the reasoning and the checks.

`test/abi_tools_test.c` is the behavioural half, and the null-OUT path is what
it covers that no assertion in the generator can see: every MPI_T query is
called twice, once asking for everything and once for a single field, and a
body that copied back unconditionally writes through the nulls of the second
call. It also covers the `MPI_T_PVAR_ALL_HANDLES` sentinel, which is 1 in the
ABI, -1 in Open MPI and an `extern ... * const` in MPICH.

**Exit check.** Every one of the 688 is generated or in `HAND_WRITTEN`; frozen tallies
per class; the "no ABI-typed parameter reaches the implementation call" assertion
passes over the emitted text; the whole thing compiles against both MPIs.

**Model: Opus.** **Two sessions**, split at arrays/status versus
callbacks/`MPI_T`.

### S4 — The hand-written set (~90, not the "~50" earlier drafts claimed)

Lifecycle, the 15 callback registration functions, spawn, the 12 buffer
attach/detach forms, `MPI_Pcontrol`, dynamic error codes, the 26 Fortran
converters, the ten status-consuming functions and the ten output-string
functions. `NOTES.md` §8 adds these up; the total is about 90, which is why this
stage is sized for two sessions rather than one.

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

**Exit check.** Each of the three consumption routes **builds and runs** a program
from an installed prefix, with `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH` cleared; `nm`
confirms `libmpi_abi` is the executable's only MPI dependency, per §20.2.1. The
prefix is exclusive — installing into the wrapped MPI's prefix collides on `mpi.h`,
`mpicc` and `libmpi_abi` (§9), so the install test must use a prefix of its own and
should assert that the MPI's own `mpi.h` is not in it.

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
over the pools and maps; the 32-bit row.

Settle whether `RTLD_DEEPBIND` survives ASan (`NOTES.md` §12 lists it as an open
risk). **`dlmopen` is not the fallback it was planned to be** — it segfaults in
`MPI_Init` with any MPI that `dlopen`s components, which is every current one
(`NOTES.md` §2). The options that remain are: measure whether `RTLD_DEEPBIND`
really does disturb the sanitizers, build the MPI under test with its components
static, or accept that the sanitizer jobs cover `libmpi_abi` and the conversion
layer rather than the loaded configuration.

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

**After S2, that looks half-right.** The mechanical part of S3 is now strongly
fenced: the ledger accounts for all 688, `gen/report.txt` names each deferred
entry point *with the class that blocks it*, the frozen tallies fail on any
reclassification, the "no ABI-typed parameter reaches the call" assertion runs
over the emitted text, and `prototype-reproduced` catches a regression in any
shape S1 tested. What none of those check is the part S3 is actually about —
**when a staged temporary may be freed**. A body that releases at return
instead of at completion passes every check listed above and corrupts memory
under load. So: the array plumbing could be Sonnet's; the lifetime rules should
not be.

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

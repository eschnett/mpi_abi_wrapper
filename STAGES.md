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
 └─→ S5 Appendix A.3 cross-check
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

**The estimate is a tally now, and it is larger.** `HAND_WRITTEN` holds **118**,
of which S1 wrote 8 (`MPI_Init`, `MPI_Finalize`, `MPI_Get_count`,
`MPI_Op_create`, `MPI_Comm_create_errhandler`, `MPI_Error_string`,
`MPI_Comm_c2f`/`_f2c`), so S4's work is the remaining **110** — §8's list plus
MPI-5.0's `_toint`/`_fromint`, which make the converter group 44 rather than 22,
and the three `MPI_Remove_error_*` forms. `gen/report.txt` groups all 118 by
reason and marks each `[done]`, so neither session has to reconstruct the list.

**Two sessions, and the cut is not by count.** It falls on whether a body is a
per-call conversion the design already has a rule for, or a piece of
wrapper-owned state that does not exist yet. That puts 70 entry points in the
first session and 40 in the second, which is the right imbalance: 42 of the
first session's 70 are one table written eleven times.

#### S4a — the converter face (70) *(done: 78 of the ledger's 118 have bodies)*

The 42 remaining handle converters, the four Fortran status converters, the nine
remaining status-consuming functions of §5.2, the nine remaining output-string
functions of §5.8, and the six `MPI_Abi_*` introspection calls.

None of these add state to the wrapper. The handle converters are
`handwritten.c`'s `MPI_Comm_c2f`/`_f2c` pair against the other ten classes —
`handles.c`'s existing maps in one direction and the implementation's own
converter in the other, with `_toint`/`_fromint` the same conversion against
`int` instead of `MPI_Fint`. Statuses go through `status.c`, sized by §4.4;
the strings follow §5.8's truncation rule with `MPI_Error_string` as the
template; and the `MPI_Abi_*` calls answer about this library rather than the
wrapped one. Three of the five groups therefore have a tested S1 body to copy,
and the two that do not — the status converters and `MPI_Abi_*` — are the
session's real judgement: what the ABI reports for Fortran `LOGICAL` and how the
`MPI_Fint`/`MPI_F08_status` blobs are sized and copied.

**Exit check.** All 70 implemented, none of them still reporting
`MPI_ERR_UNSUPPORTED_OPERATION`; a behavioural test round-trips a predefined and
a live handle of each of the 11 classes through `c2f`→`f2c` and
`toint`→`fromint`, and a status through all four converters, against both MPIs.

**This is the weakest exit check in S4, and it is weak in a specific way.** A
round-trip that converts with our own code in both directions passes even if the
Fortran integer we hand out is not the one the implementation's own Fortran side
would recognise, which is the entire point of these 44 functions. The real
oracle is **mpif in S8**; until then, keep the round-trip honest by checking
against handles the *implementation* produced rather than only ones we made.

**What the session did, and what turned out to be stronger than this
predicted.** `test/abi_converters_test.c` is the behavioural half, and two of
its checks are not round trips at all: `_toint` of a predefined handle is
compared against the ABI header's own constant, which MPI-5.0 §20.4.5 requires
and no round trip can detect, and a status converted to the Fortran form and
back is asked `MPI_Get_count`, which only succeeds if the implementation's
private bytes travelled with it. The weakness above therefore applies to the 22
`_c2f`/`_f2c` forms alone; the 22 serialization forms turned out to have an
oracle in the standard. `mpiwrapper_selftest` covers what a black-box test
cannot reach — the intern table's capacity behaviour, which has no error
channel and so must answer 0.

Three things it settled that this plan did not name, all in `NOTES.md` §3's
"What S4a settled": **`_toint`/`_fromint` are ABI-side rather than converters**,
because §20.4 puts `c2f`/`f2c` outside the ABI while §20.4.5 pins serialization
to the ABI's own predefined values; **`src/mpiwrapper/serialize.c`**, because a
dynamic ABI handle is a 64-bit address on Open MPI and no cast to `int`
recovers it; and **`dev/probe_impl.py` now reads `src/mpiwrapper/` too**,
because a hand-written body's `MPIWRAPPER_HAVE_` guard was silently false while
the probe read only the generated sources.

#### S4b — the state the wrapper has to own (40) *(done: the ledger's 118 all have bodies)*

The six remaining lifecycle entry points, the 13 remaining callback registrars
of §6.1, the 12 buffer attach/detach forms, the six dynamic error-code forms,
the two spawn forms and `MPI_Pcontrol`.

Every group here needs something that does not exist yet: an initialization
state machine and a thread level the wrapper answers from (`MPI_Initialized`
and `MPI_Finalized` are true statements about *us*, not forwarded questions);
trampoline pools beside S1's two for the file/session/window errhandlers,
`MPI_Grequest_start`, the datarep pair, the three keyval create forms and the
two `MPI_T` event handlers; §5.6's dynamic error-code registry, which
`MPI_Add_error_string` shares with the `MPI_Error_string` body S1 already wrote;
the attached buffer's ownership record, including `MPI_BUFFER_AUTOMATIC` where
the implementation does not have it; and spawn's `argv`/`array_of_argv`/
`array_of_errcodes` together.

**Exit check.** All 40 implemented; a behavioural test per subsystem —
`MPI_Init_thread` and the state machine's answers before, between and after; a
registrar of each new pool called and its callback observed running; a buffer
attached, used, detached and its ownership verified; a dynamic error class
added, seen by `MPI_Error_class` and `MPI_Error_string`, and removed. **Spawn is
the exception and should be named as one**: it needs a launcher, so it is
MPICH-only here for the same reason `NOTES.md` §11 gives, and Open MPI's row is
a documented gap rather than a pass.

**What the session did, and where the exit check turned out weaker than this
predicted.** `test/abi_state_test.c` is the behavioural half and covers every
subsystem above, each written against the shape a plausible-but-wrong body
gets wrong rather than against the happy path: each of the four error-handler
classes checks that the trampoline handed the callback *the handle it set the
handler on*, the keyval case checks that `MPI_COMM_DUP_FN` reached the
implementation as the implementation's own function rather than as the ABI's
`(function *)0x1`, and the generalized request is asked `MPI_Get_count` on the
status its query callback filled — which only answers if the whole blob crossed
in both directions. `mpiwrapper_selftest` covers the error-code registry's
capacity behaviour, which has no error channel in the `toabi` direction.

**Three rows have no oracle here, and it is the implementations' doing rather
than the test's.** No implementation available supports a user datarep at all
(MPICH's ROMIO: "Read and Write datarep conversions are currently not supported
by MPI-IO"); MPICH declares every `MPI_T` event entry point and reports zero
event types, and Open MPI 5.0.6 has no event interface, so no registration
handle exists to reach `toolevents.c`'s map with; and `MPI_Comm_spawn` *hangs*
under MPICH's hydra on this host with no wrapper involved, so the spawn case is
behind `-DMPI_ABI_TEST_SPAWN=ON` and off by default. S7's suite and S8's
consumers are the next things that can strengthen those three.

Five things it settled that this plan did not name, all in `NOTES.md` §3's
"What S4b settled": **the thread level is not wrapper state** — this plan asked
for one and it would have had no reader, since every table here is lock-free
and `MPI_Query_thread` answers from the implementation; **the error-code
registry has to intern the *implementation's* codes as well as the
application's**, because MPICH answers essentially every error with an
instance-specific code and all of them were reaching applications as
`MPI_ERR_OTHER`; **generalized requests and datareps need no registry**,
contrary to §5.6's last line, because their `extra_state` argument is one;
**`MPI_BUFFER_AUTOMATIC` is emulated with a fixed buffer** where the
implementation lacks the mode, which is an approximation `gen/report.txt`
names; and **`MPI_Pcontrol`'s trailing arguments are dropped**, which §14.2.2
permits because they belong to a profiling library above this one.

**Exit check (the stage).** `HAND_WRITTEN` fully implemented — a frozen tally
of the generator's now, counted from `handwritten.h`, so a body going missing
fails generation rather than becoming a stub; `MPI_ERR_UNSUPPORTED_OPERATION`
returned only for genuine implementation gaps, which `dev/probe_impl.py`
decides per build, and `gen/report.txt` carries the one limitation that is this
library's rather than an implementation's.

**Model: Opus.** Per-function judgement against the standard, which is the definition
of this set. **Two sessions**, split at conversion versus state.

### S5 — Oracle 4: Appendix A.3 cross-check *(done: 688 match, 8 named exemptions)*

`dev/check-c-bindings.py`: parse the C bindings out of `doc/mpi50-report.pdf` with
`pdftotext -layout` and compare against the header. Keep mpif's two properties — the
parse validates itself, and exemptions are named and fail when they stop firing.

**The appendix numbers itself A.3, not A.2** — this report's own table of contents
gives A.2 to "Summary of the Semantics of all Op.-Related Routines" and A.3 to "C
Bindings"; STAGES.md and NOTES.md #10 are corrected to match rather than left
pointing at a section that doesn't hold what they say it holds.

Unlike mpif's Fortran declarations, a C signature has no `INTENT`/`::` list to
anchor a self-check on, so the parse validates itself differently: A.3 is a run of
signatures and nothing else once headings and margin numbers are stripped, so its
open- and close-paren counts must equal the signature count found, one pair each —
any leftover means the text was misread. Array parameters are folded to their
pointer-decay form before comparison (`T x[]` and `T *x` are the same C
declarator), which is what the language says rather than an approximation.

**Eight named exemptions, not one.** C has no per-argument prose to disagree
over, so where mpif's divergences were about `INTENT` and buffer types, all of
this stage's are about *names*: A.3 lists fourteen predefined callback constants
(`MPI_COMM_NULL_COPY_FN` and so on) under prototype syntax though none is an entry
point; `MPI_Wtime`, `MPI_Wtick`, `MPI_Aint_add` and `MPI_Aint_diff` have bindings
in the standard's body but never appear in A.3 itself; `MPI_Status_f082f`/
`_f2f08` are real A.3 bindings this ABI omits by design, composing from the four
converters it does have; seven functions' `index` is `indx` in the header, the
same libc-collision dodge every MPI implementation's own headers use; MPI_T's
`pe_session` is `session`; `MPI_Status_get/set_error`'s `err` is `error`;
`MPI_Precv_init` kept `MPI_Psend_init`'s `dest` in the vendored mpi-abi-stubs
header, a copy-paste slip upstream rather than in this project; and the two
Fortran-status converters take `MPI_F08_Status`, spelled with a capital S that
owes A.3's lowercase `mpi_f08_status` nothing, since MPI-5.0 §20.4 says outright
that `MPI_F08_Status` is not part of the C ABI at all — this project coined the
name itself.

**Exit check.** Runs in CI (`c-bindings-cross-check`); every one of the 688
signatures either matches or is a named exemption; breaking one signature on
purpose makes it fail — checked directly, not asserted.

**Model: Sonnet.** Well-specified with a precedent to follow
(`mpif/dev/check-f08-bindings.jl`), though the precedent's shape (Fortran
`INTENT` lists) didn't carry over as directly as expected — a flat C prototype
needed its own self-check and its own exemption shapes rather than a port.

### S6 — Build, packaging, CI matrix *(done: all three routes build and run, five legs in `check-install.sh`)*

`mpicc`/`mpicxx`, CMake package files plus the `FindMPI` shim, pkg-config,
`ci-scripts/` for pinned MPI source builds, the variant matrix. Respect the
`ci-scripts/` versus `ci-scripts/suite/` cache-key split (§9).

**Exit check.** Each of the three consumption routes **builds and runs** a program
from an installed prefix, with `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH` cleared; `nm`
confirms `libmpi_abi` is the executable's only MPI dependency, per §20.2.1. The
prefix is exclusive — installing into the wrapped MPI's prefix collides on `mpi.h`,
`mpicc` and `libmpi_abi` (§9), so the install test must use a prefix of its own and
should assert that the MPI's own `mpi.h` is not in it.

`ci-scripts/check-install.sh` is that install test, and it runs five legs
rather than three: `bin/mpicc`, `find_package(mpi_abi)`, `find_package(MPI)`
through the `FindMPI` shim, and `pkg-config`, plus the prefix-exclusivity
assertion itself. All five pass on macOS against a distro Open MPI, and
**on Linux against MPICH 4.3.1 and Open MPI 5.0.6 built from source by this
stage's own `install-mpich.sh`/`install-openmpi.sh`** — the whole chain,
download through a running program with the loader's search path cleared,
run end to end rather than inferred from the scripts' own logic reading
right, the same way S1's exit check was.

**What the session found that this plan did not name.** Decision 5's
"falling back to a build-time path" had never been implemented: before this
stage, `mpi_abi`'s only fallback was a bare filename
(`libmpiwrapper.so`/`.dylib`), which depends on the loader's default search
path finding a same-named library — true in the build tree by accident, false
in an installed, exclusive prefix. `mpi_abi` now bakes in the absolute
installed path (`CMAKE_INSTALL_FULL_LIBDIR`), so a program built through any
of the three routes runs with no environment variable at all; `MPI_ABI_WRAPPER_LIB`
still overrides it, which is what the cross test needs.

**A platform-specific finding, `NOTES.md` #9 now has a note on:** `bin/mpicc`
invokes `CMAKE_C_COMPILER` directly rather than through CMake, and on macOS
that path resolved to the Xcode toolchain's own compiler binary rather than
the `/usr/bin/cc` shim — which fails outright, `'stdio.h' file not found`,
without an explicit `-isysroot`. CMake passes that flag to every compile of
its own targets from `CMAKE_OSX_SYSROOT`; `bin/mpicc`/`bin/mpicxx` now do too.
Measured directly: identical invocation, only the flag differing.

**The `FindMPI` shim is two mechanisms, not one.** CMake's own bundled
`FindMPI` already has a path that finds this project unassisted — its
compiler-wrapper interrogation runs `-show`-style flags against whatever
`mpicc` it locates, which `bin/mpicc.in` answers the way a real one would —
and that needs nothing installed to work. What the shipped
`cmake/FindMPI.cmake` adds is the case interrogation cannot reach: a consumer
that names neither `mpi_abi` nor a specific compiler, only `find_package(MPI)`
with `CMAKE_MODULE_PATH` pointed at the installed `Modules/` directory. Both
were exercised (`check-install.sh`'s routes 2 and 2b).

**`ci-scripts/install-mpich.sh`/`install-openmpi.sh` are far shorter than
mpif's equivalents**, and deliberately: mpif needs an MPI that already
implements the standard ABI, hence its pinned commit from MPICH `main`, its
header substitution and its pruning of everything the ABI does not define.
This project wraps *any* MPI-3.0+ implementation through its own conversion
layer, so a stock `configure && make && make install` against a released
tarball is the whole of what CI needs to provision (`NOTES.md` #9). Verified
end to end on Linux: both scripts' primary rows — MPICH 4.3.1 and Open MPI
5.0.6 — built from a downloaded tarball and then taken straight through
`check-install.sh`'s five legs.

**The MPI-3.0 floor row cost a real attempt, not just a citation.** Building
MPICH 3.1.4 first surfaced a genuine coupling bug: `--disable-fortran` (added
to save build time) silently drops the *implementations* of
`MPI_Type_create_f90_{real,complex,integer}` — plain C entry points MPI-5.0
requires and this release's `mpi.h` declares unconditionally — so
`dev/probe_impl.py`'s compile-only probe reports them available and only the
wrapper's link step fails. Both installers now run a stock configure with no
Fortran-disabling flag. What remains after that fix is the release's own
limitation, already named in `NOTES.md` #9: 3.1.4's configure itself rejects
any Fortran compiler modern enough to warn rather than error on a
mismatched-argument call, gcc 11's and gcc 13's gfortran both included, so
this row was not re-verified end to end here and needs a pinned older
toolchain when someone next picks it up.

**The variant matrix itself is not this stage's job to populate**, only to
leave room for: `NOTES.md` #10's matrix (toolchains, platforms, 32-bit,
MVAPICH) is behavioural coverage that belongs to S7's suite and S9's
sanitizer/thread/32-bit rows, which do not exist yet. What S6 owes the matrix
is a `check-install.sh` that takes any `mpicc` — distro, pinned tarball, or
another implementation entirely — as its one argument, which it does.

**Model: Sonnet.** Much of it is transposable from mpif's `ci-scripts/`.

### S7 — MPICH C test suite *(done: 1212 tests over MPICH and 1231 over Open MPI, the harness in `ci-scripts/suite/`)*

Runner, `mpiexec` filter, per-variant expected-failure list with a reason on every
line. Expect some expected failures to be *build* failures where a test reaches for
MPICH internals or `MPIX_*`.

**Exit check.** The suite runs against both implementations; the xfail list is
committed with reasons; a variant's result matching its list is the gate.

`ci-scripts/suite/run-suite.sh` is that runner: it builds and installs this
project, configures MPICH 4.3.1's `test/mpi` **against the wrapper's prefix**
rather than an MPI's, builds the tests, runs them through
`ci-scripts/suite/mpiexec-filter`, and gates the TAP output against
`xfail-<variant>.txt` with `check-tap.py`. The gate runs in both directions,
the discipline S5 and S2's checks already use: an unlisted failure fails, a
listed failure that *passed* fails, a listed test that did not run fails, and
a line with no reason fails — so `--update-xfail`, which writes bare lines,
cannot be used to make a red run green.

**Four things the plan did not name, all in `NOTES.md` §3's "What S7
settled".** The suite's own configure answers **"Is the MPI derived from
MPICH... no"**, because the ABI header defines no `MPICH` macro, so wrapping
MPICH does not quietly turn its suite into a friendlier one. The `mpiexec`
filter is **four jobs rather than one** — route to the real launcher, drop
launcher-specific arguments the testlists carry literally, restate the
environment for a launcher that forwards none of it, and enforce the timeout
itself — and its watchdog's own file descriptors are load-bearing: holding the
pipe runtests reads made *every* test appear to take exactly the timeout, three
tests at 195 seconds each, all passing. The suite decides whether the `threads`
directory exists at all by **running** an MPI program, so a host where
`MPI_Finalize` fails for an unrelated reason silently drops 8 tests rather than
failing; `run-suite.sh` prints that decision. And the build failures STAGES
expected are here, but not for the reason it gave: they are the MPI-1 entry
points **MPI-3.0 deleted** (`MPI_LB`, `MPI_UB`, `MPI_Type_extent`), which the
ABI header does not declare, plus MPICH's QMPI. Passing runtests' own
`-strict` would skip them; the runner deliberately does not, because a line
naming the entry point the ABI omits is worth more than a skip.

**What the oracle found, which is the point of the stage.** Three genuine
conversion bugs, none of which any in-house check could have seen, and two of
them fixed here:

- **An attribute's *value* is a converted class, and no signature says so.**
  `MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_HOST, ...)` returned MPICH's
  `MPI_PROC_NULL`, which is the ABI's `MPI_ANY_SOURCE`; a window reported
  create-flavour 1, which is not one of the ABI's four. The class is decided by
  the *keyval*, so `apis.json` marks nothing and the generated body forwarded
  the caller's pointer. `MPI_Comm_get_attr` and `MPI_Win_get_attr` are now
  ledger entries (`src/mpiwrapper/hw_attr.c`, ledger 118 → 120) and
  `test/abi_tools_test.c` checks the five values. This also closed the
  `MPI_LASTUSEDCODE` gap §5.6 had named and could not close from the registry.
- **A single completion must not touch `status.MPI_ERROR`** (MPI-5.0 §3.2.5;
  only the multiple-completion calls of §3.7.5 set it). The wrapper hands the
  implementation a status temporary of its own, so the implementation cannot
  honour the rule for it — the field came back as whatever the temporary held.
  13 tests caught it, and *our own* `abi_prototype_test` had asserted the wrong
  thing since S1. Fixed with a second conversion function
  (`mpiwrapper_status_toabi_keep_error`) and one emitter site.
- **`MPI_DISPLACEMENT_CURRENT` is a sentinel and is not translated**, diagnosed
  and **not** fixed here: it is `(MPI_Offset)-1` in the ABI and `-54278278` in
  ROMIO, so `MPI_File_set_view` with it returns `MPI_ERR_ARG`. §5.3's sentinel
  rule covers pointers; this is the one that is an integer. Two xfail lines
  carry the diagnosis.

**And one finding that is bigger than a bug.** An *erroneous* argument that the
implementation would have diagnosed becomes a **crash**, because conversion
interposes a local: `MPI_Comm_create(comm, group, NULL)` is `MPI_ERR_ARG` on
MPICH and a segfault through the wrapper, since the body writes the converted
handle through the caller's null pointer. Twenty tests in `errors/` are exactly
this, and `MPI_IN_PLACE` passed where it is illegal is the same shape. Deciding
what the wrapper owes an erroneous program is a design question §5 has never
had to answer, and it is written up in §3 rather than patched around here.

**The two lists are in different states, deliberately.** `xfail-mpich.txt` is
finished: 43 failures out of 1229, every one with a cause, and the gate passes
against the run it was built from. `xfail-openmpi.txt` is 168 out of 1229 and
about half of it is attributed — the entry points Open
MPI 4.1.6 does not have, read out of the probe header rather than guessed, and
the causes shared with the MPICH row — while the rest are placeholders that say
what was observed and claim nothing more. That row is dominated by MPI-4.0:
Open MPI 4.1.6 provides 466 of the ABI's 688 entry points, so sessions,
partitioned communication, persistent collectives and the `_c` forms are stubs
answering `MPI_ERR_UNSUPPORTED_OPERATION`, which is decision 6 working. Finishing
its triage is the next session's first task, and the method is the one this
stage used throughout: build the same test with the implementation's own
`mpicc` and see whether it passes without the wrapper.

**A confirming run of the Open MPI row is what established which of its
failures are load-sensitive**, and it did so through the gate rather than by
inspection: three `rma/linked_list*` tests that failed under load came back as
"expected failure that passed", which is the only signal that distinguishes a
flaky line from a real one. One of the four still fails and is the only one
listed.

**Two rows this stage could not make honest, and said so rather than listing
them.** `spawn` is excluded by default (`--with-spawn` to include) because
`MPI_Comm_spawn` hangs under hydra on macOS with no wrapper involved, and
`errors/spawn` and `threads/spawn` are excluded with it — 31 tests reaching a
180-second timeout costs an hour to learn what `test/README.md` already
recorded. And the **Open MPI row runs on Linux**, in the container
`ci-scripts/suite/linux-suite.sh` starts, because no Open MPI 5.0.x launcher
works on macOS 26.

**Model: Sonnet** for the harness, **Opus** for triage once real failures appear —
the harness is mechanical, deciding whether a failure is our bug or the test's
assumption is not. **That split held exactly:** the harness took one pass, and
every hour after it went into deciding whether a failure was ours, the test's
assumption, or the implementation's.

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

# Approach for handling the MPI ABI's ~664 entry points in a dlopen-based shim

## Context

A new project: one shared library that exports the MPI-5.0 standard ABI and
implements it over an MPI that does not provide the ABI, reached by `dlopen`
and `dlsym` so no symbol collides. Decisions already taken:

- **built against the target MPI's own `mpi.h`**, once per MPI installation
  (the MPIwrapper model, not mukautuva's runtime sniffing);
- **scope is the whole surface**: 613 core functions, the 51 `MPI_T`
  functions, the 28 Fortran handle converters, and PMPI twins of all of it;
- **implementations are MPI-4.0 or later**, so every ABI function maps to one
  implementation call and no large-count narrowing fallback is needed.

The open question is what writes the entry points.

## Recommendation

**A code generator, in Python, over two machine-readable inputs — plus a named
set of ~50 functions written by hand.** Not one function at a time by a model,
and not a port of `mpif`'s Julia generator.

The split matters as much as the choice: roughly 600 entry points are pure
mechanical translation and belong to the generator; roughly 50 involve
per-function judgement (callbacks, spawn, status, buffer attach, error codes,
`MPI_Pcontrol`) and belong to a person or a model working with the standard
open. The generator owns the ledger — it holds an explicit `HAND_WRITTEN` set
and **fails if any ABI entry point is neither generated nor in that set**, which
is what makes "nothing was silently dropped" a checked property rather than a
hope (`mpif`'s `CODE.md:657` is the precedent).

## Why a generator, argued

**1. The unit of change is the translation rule, not the function.** New MPI
releases being a year apart is the wrong clock. The clock that matters ticks
daily for the first months: how a handle is represented, how a status is
carried across the boundary, whether error codes are mapped eagerly or lazily,
how a request array is staged. Each such decision must land identically at
600–900 sites. As a generator edit that is one line and a regeneration; as 664
explicit functions it is 664 edits, repeatedly, and the sites a sweep misses are
silently wrong. `mpif`'s `HISTORY.md:459` records exactly this at far smaller
scale — a prefix change threaded through the generator still missed
`MPI_Cart_sub`, because *"a mechanical sweep is not a proof"*.

**2. Uniformity is the correctness property, and only generated code makes it
checkable.** The claim to establish is not "`MPI_Send` is right" 664 times. It
is: *every ABI handle argument is converted exactly once on the way in, every
out-handle exactly once on the way out, every sentinel is translated, no
untranslated ABI value reaches the implementation.* Over generated text that is
an assertion the generator runs on its own output — `mpif` does precisely this
(`dev/mpiapi.jl:2295` greps its emitted call arguments and fails the run if a
parameter reaches the call untranslated, because *"a parameter that slips
through untranslated hands MPI the address of a COMMON block full of poison, and
no test of the routine itself would necessarily notice"*). Over 664
independently written functions there is no such assertion, and the symptom of
an omission is a wrong answer at 4096 ranks, not a crash.

**3. There is no per-function judgement to spend on the mechanical 600.** Both
inputs are machine-readable, and I verified this rather than assuming it:

- **The ABI `mpi.h`** (from `mpi-forum/mpi-abi-stubs`, MIT; a copy is at
  `build/mpi/mpich-gcc/include/mpi.h`) is parseable by construction — the
  Forum's own `update.py` parses it with one regex. It gives all 664 signatures
  one per line between marker comments; **104 handle constants in
  `((MPI_Datatype)0x00000219)` form, which encodes class *and* value**; the
  sentinels (`MPI_IN_PLACE` = `(void*)1`, `MPI_UNWEIGHTED` = `(int*)10`,
  `MPI_STATUS_IGNORE` and `MPI_STATUSES_IGNORE` both `(MPI_Status*)0`); error
  classes and the other integer constants as one-per-line anonymous enums; and
  the status layout, fixed at `int MPI_SOURCE, MPI_TAG, MPI_ERROR;
  int MPI_internal[5]` — 32 bytes, 20 of them scratch.
- **`apis.json`** (vendored, 2 MB, as `mpif` already does) gives the orthogonal
  half the header cannot: `param_direction`, which array is sized by which other
  argument (`length: "count"` vs `"*"`), `root_only`, `constant`, `func_type`
  for callback parameters, and the `POLY*` prefix that pairs each small form
  with its `_c` form.

Delegating a table lookup to a model is spending intelligence where a table
suffices — and doing it 664 times, with no cross-item consistency check.

**4. Two compilers do the whole signature half of the verification, for free,
because of the per-install build decision.** This is the strongest reason the
generator is cheap here and it follows from the build model already chosen:

- The **outer** translation unit includes the ABI `mpi.h` only and defines the
  exported `MPI_*`/`PMPI_*`. Any signature the generator gets wrong is a
  compile error, not a runtime surprise. `nm` on the result against the
  header's symbol list is a total completeness check in both directions.
- The **inner** translation unit includes the *implementation's* `mpi.h` and
  declares every dlsym'd pointer as `static __typeof__(MPI_Send) *p_MPI_Send;`.
  The implementation's own header supplies the type, so a wrong inner signature
  is also a compile error, and **the 2843-line hand-maintained function table
  that MPItrampoline carries never needs to exist.**
- Every constant map is a generated `switch` over the implementation's own
  macro names (`case ABI_MPI_INTEGER16: return MPI_INTEGER16;` under
  `#ifdef MPI_INTEGER16`), so the implementation-side value is never
  transcribed. This is what wi4mpi's vendored per-version headers and
  MPItrampoline's 260-constant table are for, and it deletes both.

The two headers never meet in one TU: generate an `ABI_`-prefixed view of the
ABI's types and constants for the inner side. That renaming is itself
generated from the ABI header.

Between those checks, *signatures and constants become mechanically correct*,
and testing is left to cover only semantics. Nothing equivalent is available to
664 hand-written functions, whose signatures would be checked but whose
translations would not.

## What the generator actually is

Small, because the hard part of `mpif`'s generator does not exist here. Its
1150-line dispatch is about Fortran descriptors, `INTENT`, assumed-size arrays
and blank stripping — none of which has an analogue in C→C. The 149
`apis.json` kinds collapse to **12 argument classes**:

| class | what it emits | approx. sites |
|---|---|---|
| passthrough scalar (int/Aint/Offset/Count/double/char\*) | nothing | ~1100 |
| handle, scalar, in / out / inout | one conversion call | ~700 |
| handle array, in / out | staged temporary + write-back | ~40 |
| status, scalar / array / IGNORE | see below | 56 |
| choice buffer with sentinel (`MPI_BOTTOM`, `MPI_IN_PLACE`, `MPI_BUFFER_AUTOMATIC`) | one test | 222 |
| error code (return value, `array_of_errcodes`) | mapping call | 574 |
| callback (`FUNCTION`/`POLYFUNCTION`, 7 typedefs) | trampoline install | 19 |
| string array (`MPI_ARGV_NULL`, `MPI_ARGVS_NULL`) | null test | 3 |
| weights (`MPI_UNWEIGHTED`, `MPI_WEIGHTS_EMPTY`) | one test | 5 |
| attribute value / extra state (`void*`) | passthrough | 37 |
| `MPI_T` handle classes | conversion call | ~120 |
| varargs (`MPI_Pcontrol` only) | hand-written | 1 |

Estimate: **1200–1800 lines of Python**, of which ~200 are named,
prose-commented exception tables keyed on `(routine, parameter)` — never
`if name == "MPI_Foo"` scattered through the body. `mpif` has only 8 such ad-hoc
tests in 3346 lines, and that is why its special-casing stays auditable; copy
the convention, not the code.

Four disciplines to lift wholesale from `mpif`, each of which it paid for:

- **Committed output, never hand-edited.** The bug goes back in the generator.
- **An on/off switch per axis**, so a refactor that should change nothing is
  *shown* to change nothing by regenerating to an empty diff (`CLAUDE.md:133`).
- **Frozen tallies** (`cfi_expected_class_counts` is the model), so a new
  `apis.json` or ABI header reclassifies loudly rather than silently.
- **Post-hoc assertions over the emitted text**, as in point 2.

Language: **Python**, not Julia. Not because Julia is worse but because none of
`mpiapi.jl`'s substance transfers, a C project's contributors and CI already
have Python, and the reusable part of `mpiapi.jl` — its kind-classification
tables and the prose explaining *why each is a list and not a rule* — is
readable in any language. Use **`pympistandard` as a dev-time cross-check
only, not a build dependency**: it is lightly maintained, has no tags, and its
`LICENSE` file is empty (MIT is declared in `pyproject.toml` only, upstream
issue #35). Vendor `apis.json`, which is what it would give you anyway.

## The ~50 hand-written functions

These are where a model working per-function, with the standard open, is the
right tool — and where the interesting design work is:

- bootstrap and lifecycle: `MPI_Init`, `MPI_Init_thread`, `MPI_Finalize`,
  `MPI_Abort`, `MPI_Initialized`, `MPI_Finalized`, session init/finalize;
- **status**, the one decision to settle before the generator, because it
  changes every one of the 56 status-bearing signatures. The 20 scratch bytes
  cannot hold every implementation's status (MPICH's is 20 bytes and fits; Open
  MPI's is 24 and does not). The portable route needs no layout knowledge at
  all: stash `{source, tag, error, byte count via MPI_Get_elements_x(…,
  MPI_BYTE, …), cancelled}` = 12 bytes, and reconstruct an implementation
  status on demand with `MPI_Status_set_elements_x` + `MPI_Status_set_cancelled`.
  Keep "embed the whole implementation status" as a `_Static_assert`-guarded
  fast path, so the build fails loudly where it does not fit;
- **callback trampolines** for ops, errhandlers, keyval copy/delete, generalized
  requests and datareps. mukautuva identifies a user reduction function by
  attaching it to a *duplicate datatype*, because the datatype is the only
  identifiable thing inside the trampoline. A pool of N generated static
  trampolines, each knowing its own index, is simpler and does not perturb
  datatypes — worth trying first;
- `MPI_Comm_spawn`/`_multiple`, `MPI_Buffer_attach`/`_detach`,
  `MPI_Pcontrol` (genuinely variadic), the dynamic error-code routines
  (`MPI_Add_error_class`/`_code`/`_string`), and the 28 Fortran converters,
  which are the reason `mpif` can then run over any MPI.

## Verification

Four oracles, three of them free:

1. **The ABI header, by compilation** — outer TU signatures, plus `nm` exports
   against the header's symbol set, both directions.
2. **The implementation's header, by compilation** — inner TU pointer types via
   `__typeof__`, and every constant `case` naming a real macro.
3. **The generator's assertions on its own output** — every handle/sentinel
   translated exactly once; frozen tallies; unknown kind is a hard stop.
4. **MPI-5.0 Appendix A.2 (C bindings) via `pdftotext -layout`**, as an
   independent route from the same LaTeX that produced `apis.json`.
   `dev/check-f08-bindings.jl` is directly transposable and A.2 is far more
   regular than the A.4 it currently parses — keep its two best properties: the
   **parse validates itself** (every argument declared exactly once, else the
   text was misread and no comparison is trustworthy), and **exemptions are
   named, explained, and fail the run when they stop firing**.

Then behavioural tests: the MPICH test suite, and `mpif`'s own `test/` run with
this shim as its `libmpi_abi` — which is the end-to-end proof that the two
projects compose.

## Sequencing

**Prototype before generator.** Hand-write ~12 representative entry points
end-to-end and get them passing against one MPI: `MPI_Init`, `MPI_Finalize`,
`MPI_Comm_size`, `MPI_Send`, `MPI_Recv`, `MPI_Get_count`, `MPI_Waitall`
(request array + `MPI_STATUSES_IGNORE`), `MPI_Allreduce` (`MPI_IN_PLACE` + a
user op), `MPI_Comm_split`, `MPI_Type_create_struct` (`_c` pairing),
`MPI_Error_string`, `MPI_Comm_c2f`. Only then write the generator, and require
it to reproduce those twelve. Designing the generator before the shape of its
output is known is the main way this goes wrong.

## Alternatives rejected

- **A model per function, all 664.** Rejected on points 1 and 2 above: the
  artifact is ~40k lines nobody will re-read, every cross-cutting change is a
  664-site edit, and the uniformity claim becomes unverifiable. The initial
  writing is not the cost; the second month is.
- **Porting `dev/mpiapi.jl`.** Its transferable content is ~200 lines of tables
  and its discipline; its 1150-line dispatch is Fortran-specific.
- **A hand-written per-function spec file** (wi4mpi's `functions.json`, 328 KB).
  Duplicates what `apis.json` already knows and must be re-maintained per MPI
  version.
- **X-macros / preprocessor wrappers.** MPItrampoline's README still advertises
  this; the repository has moved to generated Python tables. Macros cannot do
  type-directed per-argument dispatch over varying arity without an X-macro list
  that *is* the table a generator would emit, with worse diagnostics.

## Risks worth measuring early

- Whether the implementation's status fits the ABI's 20 scratch bytes (measure
  for MPICH and Open MPI before committing to the fast path).
- Whether a dynamically created implementation handle can simply be bit-cast
  into an ABI handle without colliding with the ABI's `0x20`–`0x2eb`
  predefined range. `mpif`'s `test/predefined_types_c.c` round-tripped all 103
  predefined handles with zero failures and is the model for the probe; MPICH's
  dynamic handles carry a kind field in the high bits and Open MPI's are real
  pointers, so the answer is probably yes — but probe it rather than assume it.
- `MPI_Op` needs a lookup regardless (the trampoline must find the user
  function), and non-blocking `MPI_Alltoallw` needs a request→array map, which
  puts a branch on every request-completion call. Both are noted in the
  EuroMPI'23 ABI paper as the only two places translation is not trivial.

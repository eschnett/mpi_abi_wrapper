# MPI ABI wrapper — design

The authoritative design document: what this project is, why it is shaped the
way it is, and what about it is missing, broken or undecided. One of four:

| | holds |
|---|---|
| `CLAUDE.md` | how to work here: the reading protocol, the host, the session rules |
| `CODE.md` | what the repository contains now, and the number behind every claim |
| **`NOTES.md`** | the design, its reasons, and what is missing, broken or undecided |
| `HISTORY.md` | roads not taken, beliefs that were measured false, and the stage record |

**Read this file by the section a task names, never whole.** `CLAUDE.md`
carries the protocol and the section map.

**The section numbers below are an interface.** Roughly two hundred places in
the source cite `NOTES.md #2`, `#5.7`, `#6.2` and the rest. §1–§13 keep their
numbers and their topics permanently; content may move within a section, and a
section may not be renumbered.

**What is *not* here.** Current tallies, file inventories and measured platform
status live in `CODE.md`, because a count written into prose rots and eleven of
them did (`HISTORY.md` §4). Stage narration and abandoned approaches live in
`HISTORY.md`. The rule for the boundary with that file: **if a wrong answer is
still one edit away from the current code, its reason is here**; if it is a road
the code no longer touches, it is there.

Every numeric claim about the ABI or about MPICH and Open MPI was read out of
the actual headers; see "Sources" at the end.

---

## 1. Goal, deliverables, scope

Implement the MPI-5.0 C ABI on top of an existing MPI implementation that does
not itself provide the ABI. Each ABI call is forwarded to that implementation
("the MPI library" below), converting arguments in both directions.

**Deliverables**

| | built | needs an MPI? |
|---|---|---|
| `libmpi_abi` + `mpi.h` | once, implementation-independent | no |
| `libmpiwrapper` | once per MPI installation | yes |

`mpi.h` comes from [mpi-forum/mpi-abi-stubs](https://github.com/mpi-forum/mpi-abi-stubs)
with `doc/mpi.h.patch` applied. That patch does three things: it adds the
Fortran-support declarations (`MPI_Fint`, `MPI_F08_Status`, the four
`MPI_F*_STATUS(ES)_IGNORE` macros with real storage behind them, and the 26
handle/status converters); it corrects an error in the stub header where
`MPI_Psend_init`/`MPI_Precv_init` were given `int count` with a separate `_c`
variant instead of `MPI_Count count`; and it gives `MPI_Status` a struct tag
(§2, "Naming"), which is worth proposing upstream since it costs nothing and
changes nothing about the ABI.

**Upstream has since made the `Psend`/`Precv` correction itself, and that is a
trap waiting for whoever re-vendors.** `dev/vendor/mpi-abi-stubs/` is pinned at
`a1183ce6` (2025-11-24), which still has the old signatures, so the hunk is
still needed *today*; upstream's tip carries the corrected ones. The moment the
vendored copy moves past that fix, `patch` finds the hunk already applied and
reports "Reversed (or previously applied)".

That is not a prediction: it is what happened to mpif, whose
`install-mpi-header.sh` clones the stubs *unpinned*, and which the mpif rows of
§10 caught failing on exactly this. There the consequence was severe, because
`patch` running with no terminal answers its own prompt and reverse-applies —
mpif's header came out with the Fortran declarations *removed*.

**Here it is only a build failure**, and the difference is worth knowing rather
than assuming: `dev/generate_headers.py` checks `patch`'s exit code and raises,
and this project's patch has four hunks where mpif's has three, so the reversal
does not reach the Fortran block. Measured against a simulated future stub, not
argued. `--forward` is passed anyway, so the refusal is stated rather than
inferred from an exit code that happened to be non-zero.

Vendoring is what makes this project's copy a dated artifact rather than a
moving one, and it is the whole reason this is ours to schedule rather than
ours to be surprised by.

**Which of the four hunks survive a re-vendor**, read off a diff of the
vendored copy against upstream `a8470014` rather than inferred from the patch
failure:

| hunk | what it does | at upstream tip |
|---|---|---|
| `@@ -37` | gives `MPI_Status` a struct tag | **still needed** — upstream still writes `typedef struct { … } MPI_Status;` |
| `@@ -941` | `MPI_Psend_init`/`MPI_Precv_init` | **adopted upstream**, drop it |
| `@@ -1609` | the `PMPI_` twins of the same | **adopted upstream**, drop it |
| `@@ -1896` | the Fortran block | **needed permanently** — see below |

So the patch does not become unnecessary; **half of it does**. And the Fortran
half is not waiting on upstream at all: MPI-5.0 §20.4 puts `MPI_Fint` and
everything depending on it *outside* the ABI on purpose, so the stub header
will never carry it. That block is this project's extension, shared with mpif
by agreement rather than by standard, and `HISTORY.md` §2.18 is what it cost to
learn that being outside the ABI does not mean being unconstrained.

**Upstream changed more than that hunk**, and the rest is re-vendoring work
rather than patch work: the `MPI_T` handle tags were renamed
(`struct MPI_T_enum_t` → `struct MPI_ABI_T_enum`, and five more like it), which
is the same convention §2's renaming rules assume and worth re-checking against
them; `MPI_Aint`/`MPI_Offset`/`MPI_Count` were restructured with an MSVC branch,
which is the first thing in the stub header to acknowledge Windows (§13.4); and
`MPI_ERR_LASTCODE`, `MPI_ORDER_C` and `MPI_ORDER_FORTRAN` were respelled from
hex to decimal at the same values, which changes nothing and will still show up
in a diff.

**Scope, counted from the patched header rather than estimated.** 688 entry
points, with `MPI_*` and `PMPI_*` exactly symmetric — every one has a twin, no
exceptions in either direction. `CODE.md` §2 carries the breakdown and the
authority for each number. Two of them are worth stating here because they are
*different numbers* and the difference is a design property:

- **1376 exported symbols** in `libmpi_abi`. All 688 entry points are always
  exported, under both names, on every build.
- **1366 vtable slots.** Five entry points are answered by `libmpi_abi` itself
  and never reach the wrapper (§3), so 683 × 2 slots carry the rest.

Deprecated still means provided: the 12 entry points the header marks
deprecated are implemented like any other.

**Consumers.** The point of the ABI is that large MPI-dependent projects can be
built once and run against any implementation, so the consumers that matter are
the widely-used libraries and applications:

- **HDF5**, whose parallel driver is the heaviest real user of `MPI_File_*` and
  therefore of the `MPI_File` handle class, the bitmask `amode`, and
  `MPI_File_get_view`'s `datarep`.
- **PETSc**, which exercises collectives, derived datatypes, user-defined
  operations and attributes about as broadly as anything does.
- **mpi4py**, which is the case that motivates the plugin scenarios: a host
  executable that knows nothing about MPI, loading extension modules that do.
- **[mpif](https://github.com/eschnett/mpif)**, MPI Fortran bindings over the
  ABI, and the only consumer that reaches the 26 Fortran converters and the
  status `f2c`/`c2f` paths. Requesting changes there is acceptable; it also
  shares problem shapes with this project, and several conventions here are
  lifted from it.

These are not interchangeable as tests. §10 treats them as distinct oracles
rather than as a list.

**Constraints**

- **Thread safety.** MPI applications may be multi-threaded. Prefer atomics over
  anything needing a support library.
- **Portability.** Linux and macOS are required; FreeBSD and Windows (mingw) if
  possible. Compiler-specific mechanisms are acceptable.
- **Cross-compiling is supported**, which has a sharp consequence: *nothing may
  be determined by running a program at configure time.* Every check is either a
  compile-time assertion or a runtime check inside the library.
- On macOS it is acceptable to assume the MPI library uses a two-level
  namespace. That assumption is load-bearing — no macOS build may acquire
  `-flat_namespace` by accident, ours *or the implementation's*, since a flat
  implementation's own internal calls resolve into our exports by load order
  (`HISTORY.md` §2.19) — and it is checked at load rather than at configure
  time (§2).
- **The ABI surface is complete and is MPI-5.0** (plus the Fortran extension of
  `doc/mpi.h.patch`). A function the implementation lacks is **reported at run
  time**, never omitted from the ABI: the slot returns
  `MPI_ERR_UNSUPPORTED_OPERATION` and the generator lists it in
  `gen/report.txt`. An application must be able to link and start against any
  wrapper and discover at run time what is missing.
- **The implementation is expected to provide the MPI-4.0 API.** That is what
  makes the common case a 1:1 mapping, since the `_c` variants exist there. It
  is an expectation, not a hard floor.

  **And it is not met by any released Open MPI.** Open MPI 5.0.10 defines
  `MPI_VERSION 3` / `MPI_SUBVERSION 1` and has **no `_c` entry point at all** —
  `MPI_Send_c`, `MPI_Type_create_struct_c` and the rest of the family are simply
  absent from its header. So the MPI-4.0 configure check is a warning rather
  than a hard error, and the enforced floor is MPI-3.0. It is worth knowing that
  the mechanism is **load-bearing on day one** and not a contingency for exotic
  implementations.

  What that used to cost was exactly the large-count half of the surface: all
  159 `_c` entry points became decision 6's stubs, which is 148 of the 182
  absent generated names on that implementation — 81% of the whole gap. **It no
  longer costs that.** Where the implementation lacks the `_c` form but has its
  small twin, the wrapper narrows the call rather than refusing it, and refuses
  only the values that will not fit (§5.10, decision 6). The expectation above
  is therefore about *cost*, not about capability: meeting it buys a 1:1 mapping
  with no narrowing check and no staged count array, and failing it buys a
  ceiling at `INT_MAX` rather than a missing entry point.

  **True of every release, and no longer true of `main`.** Open MPI's
  development branch — the commit mpif pins for its ABI rows, and the one the
  mpif legs of §10 wrap — declares `MPI_VERSION 5` / `MPI_SUBVERSION 0` and
  declares `MPI_Send_c` and the rest in its ordinary `mpi.h`. So the
  large-count half of the surface is exercised over Open MPI there, where over
  5.0.10 it is entirely decision 6's stubs.

  Either way **`MPI_Get_version` answers 5.0**, because that is the standard
  *this library* presents (decision 24); what the wrapped implementation
  supports is discovered per entry point, at run time, by
  `MPI_ERR_UNSUPPORTED_OPERATION`.

---

## 2. Architecture

```
application  ->  libmpi_abi.so  ->  libmpiwrapper.so  ->  libmpi.so
                 (ABI types)        (owns every           (linked normally,
                                     conversion)           not dlopened)
```

`CODE.md` §1 has the annotated version.

**Why two libraries.** `libmpi_abi` must export `MPI_Send`, and so does
`libmpi`. That collision is the only reason `dlopen` is involved: `libmpi_abi`
never links the MPI library, it loads `libmpiwrapper` at run time, and
`libmpiwrapper` pulls in `libmpi` through an ordinary `DT_NEEDED`.

**Why the boundary is ABI-typed.** Because the conversions live on the far side
of it, `libmpi_abi` contains no implementation types, needs no MPI to build, and
is built once for all implementations. `libmpiwrapper` links the MPI library
directly, so it calls `MPI_Send` and reads `MPI_COMM_WORLD` as ordinary code —
no `dlsym` of MPI functions, no preprocessor prelude renaming ~700 names out of
the way, and no need to export predefined handle values as symbols.
`HISTORY.md` §1.1 has the architecture this replaced and what it cost.

**The vtable.** `libmpiwrapper` exports exactly one symbol:

```c
const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t abi_subversion,
                      uint32_t layout_hash, size_t size,
                      const void *abi_probe, const char **diagnostic);
```

returning NULL with a diagnostic on mismatch. **All four version fields are
checked for exact equality.** The header carries both `MPI_ABI_VERSION`
(currently 1) and `MPI_ABI_SUBVERSION` (currently 0), and the hash does not
cover the subversion: one that added no slot would leave the hash unchanged
while still meaning the halves were generated from different specifications.
`layout_hash` is generated from the slot list, so a regeneration that reorders
or inserts a slot is caught rather than silently calling through a shifted one.

`size` is `sizeof(struct mpiwrapper_vtable)` as the *caller* understands it, and
it is kept even though the three checks in front of it look stronger, because it
is the one thing the hash cannot see: the hash is taken over the *text* of the
slot list, so two halves that agree on every declaration and disagree on what
those declarations weigh — a 32-bit `libmpi_abi` against a 64-bit
`libmpiwrapper`, or two compilers differing about a struct's layout — hash
identically and differ in `sizeof`. That is precisely the mismatch that produces
a call through a shifted slot, and §4.1's pointer-sized handles make it
plausible rather than exotic. (`HISTORY.md` §1.16 records the "serve the common
prefix" provision that was deleted, and how to get additive growth honestly if
it is ever wanted.)

A getter rather than an exported struct, because you would otherwise have to
trust the layout in order to read the version out of it — and because the getter
is a natural place to build the reverse handle map before returning.

**PMPI gets its own slots.** `MPI_Send` and `PMPI_Send` are two definitions in
`libmpi_abi` (not a weak alias — macOS aliases need `-Wl,-alias` or `__asm__`
labels, and a one-line body makes an alias pointless) reaching *two* slots,
whose wrapper bodies call the implementation's `MPI_Send` and `PMPI_Send`
respectively. `HISTORY.md` §1.2 has why folding them onto one slot is wrong.

**No configure probe is needed for the shifted names.** What holds everywhere is
that **both names exist and resolve to the same code when no tool is
interposed**; whether the alias is weak or strong varies by implementation and
platform and is an interposition detail that matters to profiling tools, not to
us. `HISTORY.md` §2.1 has the measurements, including the macOS shapes — which
do matter for one practical reason: the wrapper must link **what `mpicc`
links** rather than a library it names itself, since `-lmpi` alone would leave
every `PMPI_*` undefined against conda-forge MPICH, which ships a separate
`libpmpi.dylib`. An implementation that really lacked the shifted names now
fails to link with an undefined symbol naming one, which is the outcome §5.9
asks for.

**The wrapper's own internal MPI calls use `PMPI_*` unconditionally** — in the
hand-written set of §8, where `MPI_Init` needs a rank or the error-code registry
needs a class. An internal call is not application traffic and must not be
counted as such by an interposed tool. Same discipline implementations follow
inside themselves, and independent of the slot question.

**Locating the wrapper.** An environment variable (`MPI_ABI_WRAPPER_LIB`),
falling back to the absolute installed path baked in at build time
(`CMAKE_INSTALL_FULL_LIBDIR`). A bare filename is *not* an acceptable fallback:
it depends on the loader's default search path finding a same-named library,
which is true in the build tree by accident and false in an installed exclusive
prefix. One `libmpi_abi` binary can therefore be pointed at any wrapper, which
is the practical payoff of the split and what the cross test of §10 needs.

**Bootstrap.** A library constructor in `libmpi_abi` sets a plain pointer, read
with no atomic and no NULL check outside debug builds (decision 8). The load
cannot hang off `MPI_Init`, because `MPI_Initialized`, `MPI_Get_version` and the
`MPI_T_*` calls are legal before it — hence a constructor. It needs no lazy-init
guard beside it: anything that can call an entry point must link `libmpi_abi`,
hence depends on it, hence its own constructors run after ours, and a plugin
`dlopen`ed later is no exception because loading it loads `libmpi_abi` first.
`HISTORY.md` §1.6 measures what the guard would have cost — 23 instructions
instead of 4 and 95 KB of text instead of 22 KB, for a window that cannot occur.

### Symbol resolution when loading the wrapper

**This is the most delicate thing in the design.** The answer is `RTLD_LOCAL`
plus *active isolation*, matching what MPItrampoline does
(`MPItrampoline/src/mpi.c:477`).

**The problem is not what the wrapper exports; it is how the wrapper's own
references resolve.** On ELF a `dlopen`ed object resolves its *undefined*
symbols against the global scope **first** and its own dependency subtree second
— the asymmetry `RTLD_DEEPBIND` exists to invert. The application links
`libmpi_abi`, so `libmpi_abi` is in the global scope, so `libmpiwrapper`'s
reference to `MPI_Send` binds to *our* `MPI_Send`:

```
libmpi_abi::MPI_Send -> vtable -> w_MPI_Send -> libmpi_abi::MPI_Send -> ...
```

Infinite recursion, in the default configuration. `RTLD_LOCAL` does not fix it:
`LOCAL`/`GLOBAL` controls what the loaded object *exports*, not how its
references resolve. **Isolation is mandatory, not an optimization.**

**Why `RTLD_GLOBAL` is actively harmful**, three independent reasons, kept here
because the flag is one edit away in `bootstrap.c`:

- It puts `libmpi`'s `MPI_Send` into the global scope, so a plugin `dlopen`ed
  *later* binds to the native MPI — global is searched before the plugin's own
  local scope, where `libmpi_abi` lives — and is then handed ABI-typed handles
  and a 32-byte status. Silent corruption, and only in the second plugin. This
  is a normal configuration: mpi4py plus a second MPI-using extension module in
  one Python process.
- The implementation's own internals are written against MPI in places (Open
  MPI's ROMIO and io components). Capturing those is not merely wrong but
  **memory-unsafe**: a component calling `MPI_Recv` passes a 24-byte
  `ompi_status_public_t`, and our ABI `MPI_Recv` writes 32 bytes into it.
- Handles would survive such a capture *by accident* — dynamic ones bit-cast to
  themselves, and predefined implementation values sit outside the ABI's
  `0x20`..`0x2eb` range so they bit-cast through too — which makes the failure
  intermittent and data-dependent rather than immediate.

Calling `PMPI_*` internally does not save the implementation: we export those
too, so both names are captured.

**Per platform:**

| | how | why |
|---|---|---|
| macOS | `RTLD_LOCAL` | the two-level namespace binds `libmpiwrapper`'s `MPI_Send` to `libmpi` at link time, so there is nothing to capture |
| Linux | `RTLD_LOCAL \| RTLD_DEEPBIND` by default, `dlmopen(LM_ID_NEWLM)` selectable | `DEEPBIND` is simpler, has no namespace limit, and is the only one that works with a real MPI |
| FreeBSD | `RTLD_LOCAL \| RTLD_DEEPBIND` | `dlmopen` does not exist |

`dev/dlopen-probe/` settled two things reasoning had left open, both about the
Linux default: **`RTLD_DEEPBIND` applies transitively**, redirecting the
*implementation's own* internal `MPI_*` references and not merely the wrapper's,
even though `libmpi` is a dependency loaded by the same call rather than the
object named in it; and **`RTLD_GLOBAL` really does promote dependencies** into
the global scope, with the later-plugin bypass demonstrated rather than
predicted. Its results are statements about a mock with no MPI in it, which is
exactly why its `dlmopen` row does not generalize.

**`dlmopen` does not survive contact with a real MPI.** It segfaults in
`MPI_Init` with any implementation that `dlopen`s its components, which is every
current one; the cause is glibc's own loader and is structural rather than one
implementation's quirk. `HISTORY.md` §1.5 has the diagnosis and the confirming
case. The mode stays selectable — one environment variable, and a component-free
MPI is a real configuration — but **nothing may be planned on it**, and §11's S9
is where that matters.

**Binding mode defaults to `RTLD_LAZY`, not `RTLD_NOW`.** `RTLD_NOW` forces
every undefined symbol in `libmpi` and its dependency closure to resolve, and
real MPI installations have symbols that are never called. Overridable.

### Proving the isolation worked

**Check the outcome, not the mechanism.** `dlinfo(handle, RTLD_DI_LMID)`
confirms which namespace you got but not that every reference resolved the way
the namespace was meant to make it resolve. Two checks run at load, and they are
independent:

1. **By address.** `libmpi_abi` passes the address of one of its own functions
   to `mpiwrapper_get_vtable`, and the wrapper `dladdr`s that together with the
   `MPI_Send` it actually resolved, refusing if the two share a base object.
   This does not depend on knowing whether `RTLD_DEEPBIND` propagates to
   dependencies. `src/mpiwrapper/getvtable.c`.
2. **Behaviourally.** `dladdr` answers a subtly different question than the one
   that matters — "where does this *name* resolve", not "where does this *call*
   bind" — and `HISTORY.md` §2.3 has the measured case where the two answers
   differ: a flat-namespace implementation build whose own internal
   `MPI_X → PMPI_X` forwards resolve by load order into our strong `PMPI_X`
   *while taking the symbol's address still resolves correctly*, and the
   symptom is silent double execution rather than recursion (§2.19 for the
   mechanism, `dev/weakdef-probe/` for the matrix). So the ABI side makes one
   call through the vtable and sees whether the call comes back. `MPI_Get_version` is the probe:
   legal before `MPI_Init` in every version of the standard, no side effects.
   (`MPI_Wtime` reads better and is wrong — `HISTORY.md` §2.5.)

   **`MPI_Get_version` is hand-written now (decision 24) and still makes this
   call on purpose.** Its *answer* is the ABI's own 5.0 and the
   implementation's is discarded — but the call itself is what this probe
   observes, so a body that computed two constants and returned would leave
   the probe with nothing to detect and retire the check silently. The comment
   in `src/mpiwrapper/hw_lifecycle.c` says so at that end too; a call whose
   result is thrown away is exactly what a later reader deletes.

**The probe's mechanism keeps the generated code out of it entirely.** A
captured call re-enters `libmpi_abi`'s own exported entry point, which does
nothing but call through `mpi_abi_vt` — so pointing `mpi_abi_vt` at a **decoy
table** during the probe both detects the re-entry and stops the recursion, with
no forwarder needing to know a probe exists. *Every* slot of the decoy points at
the same recorder, not just the one being called: the captured call can land on
any entry point, since the implementation's `MPI_Get_version` may reach for
another MPI function internally, and a decoy with one slot filled turns the
capture into a jump through a null pointer instead of a diagnostic.

**Both checks are sound and incomplete**, and `HISTORY.md` §2.4 has the measured
partial capture that gets past them. What they leave is a stack overflow, not a
wrong answer, which is why `test/check_isolation.cmake` treats a crash as an
acceptable outcome and fails only on a *successful* run of an unisolated
wrapper.

**The reliability property this adds up to** is weaker than "it always works"
and stronger than "it usually works": **either the wrapper loads and its calls
provably reach the implementation, or it refuses to start and says why.** There
is no configuration known to us in which it loads and silently does the wrong
thing.

One consequence for the design: **a flat-namespace implementation build cannot
be wrapped on macOS, by us or by anything that exports its names.** A flat
image resolves even calls to symbols it defines itself by global load order,
so its internal `MPI_X → PMPI_X` forwards land in our strong `PMPI_X` — the
captured edges are all the *implementation's*, which is why no choice of call
target on our side avoids them (we must export both names), and why refusing
at load is the correct outcome. A **two-level** implementation is never
captured, weak symbols or strong: coalescing chooses among images that have
weak definitions, and our all-strong `libmpi_abi` never participates. This
replaces an earlier belief that weak coalescing made any ABI-implementing MPI
unwrappable here — `HISTORY.md` §2.19 and `dev/weakdef-probe/` have the
measurements, and oracle 5 in §10 has the two-level standard-ABI build that
wraps.

### Naming, and the renaming rules for `mpiabi.h`

`MPIABI_` uniformly (`MPIABI_Comm`, `MPIABI_COMM_WORLD`). Not `MPI_ABI_`,
because the stub header already uses `MPI_ABI_Comm` as a *struct tag* and
reusing it as a typedef name is legal but confusing. Three rules:

1. **Typedef names, macro names and enumerator names are prefixed.** That is
   what lets both views coexist in one translation unit, so a wrapper body can
   say `MPI_INT` for the implementation's datatype and `MPIABI_INT` for the
   ABI's on the same line.
2. **Struct tags are left alone.** `MPIABI_Comm` stays `struct MPI_ABI_Comm *`,
   which is *the same type* as the ABI header's own `MPI_Comm`. Renaming the tag
   would make them incompatible and force a cast in all 1376 forwarders on the
   ABI side — casts that would then silently absorb a genuine type error.
3. **Struct member names are left alone.** Members live in a per-struct
   namespace, so `MPIABI_Status.MPI_SOURCE` and the implementation's
   `MPI_Status.MPI_SOURCE` cannot collide, and keeping them identical means the
   status conversion reads the way the standard describes it.

**Six exceptions, each of which the rules do not reach.** All are implemented in
`dev/generate_headers.py`; each was found by a build failing or, worse, not
failing.

- **`MPI_ABI_VERSION`/`MPI_ABI_SUBVERSION`** (the ABI *protocol* handshake
  version, 1/0) rename to `MPIABI_ABI_VERSION`/`MPIABI_ABI_SUBVERSION` under the
  plain rule. The double "ABI" looks odd next to `MPI_VERSION`/`MPI_SUBVERSION`
  (the MPI *standard* level, 5/0), which rename to the unadorned
  `MPIABI_VERSION`/`MPIABI_SUBVERSION` — but the two source names differ, so the
  renamed spellings differ too and nothing collides. The tempting special case
  collides; `HISTORY.md` §1.21.
- **`MPIX_TYPECLASS_LOGICAL`** is the one enumerator in the stub not spelled
  `MPI_*` — a legacy alias in the same anonymous enum as the `MPI_TYPECLASS_*`
  family. Left unrenamed it collides with `mpi.h`'s own definition the moment
  both headers are included together. Renamed to `MPIABIX_TYPECLASS_LOGICAL`.
- **`MPI_T_cb_safety`/`MPI_T_source_order`** are declared `typedef enum
  MPI_T_cb_safety { ... } MPI_T_cb_safety;` — tag and typedef spelled
  identically, unlike every handle type. Rule 2 protects tags because
  handle-pointer tags are meant to be *shared* with `mpi.h` for type identity;
  here the opposite is true, since a real implementation's `<mpi.h>` also
  declares a tag of that name with different enumerator values. Both occurrences
  are renamed, deliberately breaking the tag identity — and the consequence is
  that those two ABI types and their implementation counterparts are genuinely
  distinct, which makes the four `MPI_T` forwarders that pass them the only
  place in `entrypoints.c` where a cast is correct.
- **`MPI_Aint`/`MPI_Offset`/`MPI_Count`** are defined through a `#if
  !defined(MPI_ABI_X)` / `#define MPI_ABI_X <rhs>` / `typedef MPI_ABI_X MPI_X;`
  / `#undef` idiom, and for these three both spellings rename to the *same*
  name. A line-by-line rename turns the typedef into `typedef MPIABI_Aint
  MPIABI_Aint;`, and the preprocessor expands *both* occurrences — it has no
  notion that one was meant to survive as the newly-introduced name — so the
  type is never introduced and `MPI_Count`'s definition fails one link down the
  chain. The generator resolves these three directly (`typedef intptr_t
  MPIABI_Aint;`), using the header's own default branch.
- **`MPI_Status` needs a struct tag, and `doc/mpi.h.patch` adds one.** The stub
  gives every handle a tag but declared the status as a typedef of an
  *anonymous* struct, and two anonymous structs are two incompatible types
  however identical their layout — so the ABI side could not pass an
  `MPI_Status *` into a slot typed `MPIABI_Status *` without a cast in 90-odd
  forwarders. Unlike the handle tags this struct is *defined* in both views, so
  the definition is guarded and the typedef is not:

  ```c
  #if !defined(MPI_ABI_STATUS_DEFINED)
  #define MPI_ABI_STATUS_DEFINED
  struct MPI_ABI_Status { ... };
  #endif
  typedef struct MPI_ABI_Status MPI_Status;   /* MPIABI_Status in the other view */
  ```

  which works in either include order.
- **`MPI_ABI_STATUS_DEFINED` is deliberately left unrenamed** (the generator's
  `KEEP_UNRENAMED` set): it coordinates the two views rather than naming
  anything in the API, and renaming it would give the two headers different
  guards, at which point both define the struct and neither compiles beside the
  other.

**`gen/include` holds `mpi.h` beside `mpiabi.h`, and that is a trap for the
wrapper's build.** `libmpiwrapper` includes `<mpi.h>` meaning the
*implementation's*. Put our directory on its include path and `<mpi.h>` finds
the ABI header instead — which **compiles and links**, because the ABI header is
a complete valid `mpi.h` and the implementation exports the names, and then
fails at run time with the implementation rejecting `comm=0x101`. It cannot be
fixed by ordering the flags: CMake passes an imported target's includes as
`-isystem`, and every `-I` is searched before every `-isystem`. So `mpiabi.h` is
staged into a directory containing nothing else, and `src/mpiwrapper/internal.h`
carries an `#error` on the same condition for anyone building outside CMake,
with `MPIWRAPPER_WRAP_ABI_IMPL` as the escape hatch that oracle 5 needs.

---

## 3. The generator

**All 688 entry points are accounted for by a generator, in Python, with a named
set written by hand.** The generator fails if an entry point is in neither set,
which is what makes "nothing was silently dropped" a checked property rather
than a hope. `gen/report.txt` names all 688 with the reason for each, and
`CODE.md` §2 carries the split.

### Why a generator

**The unit of change is the translation rule, not the function.** New MPI
releases being a year apart is the wrong clock. The clock that matters ticks
daily for the first months: how a handle is represented, how a status crosses
the boundary, whether error codes are mapped eagerly or lazily, how a request
array is staged. Each such decision must land identically at 600–1400 sites. As
a generator edit that is one line and a regeneration; as 688 explicit functions
it is 688 edits, repeatedly, and the sites a sweep misses are silently wrong.
mpif records exactly this at far smaller scale: a prefix change threaded through
its generator still missed `MPI_Cart_sub`, because a mechanical sweep is not a
proof.

**Uniformity is the correctness property, and only generated code makes it
checkable.** The claim to establish is not "`MPI_Send` is right" 688 times. It
is: *every ABI handle argument is converted exactly once on the way in, every
out-handle exactly once on the way out, every sentinel is translated, and no
untranslated ABI value reaches the implementation.* Over generated text that is
an assertion the generator runs on its own output. Over 688 independently
written functions there is no such assertion, and the symptom of an omission is
a wrong answer at 4096 ranks, not a crash. `HISTORY.md` §1.20 has the
alternatives and why each fails this test.

**Both inputs are machine-readable.** The ABI `mpi.h` is parseable by
construction — the Forum's own `update.py` parses it with one regex — and gives
all 688 signatures one per line, the predefined handle constants in
`((MPI_Datatype)0x00000219)` form (which encodes class *and* value), the
sentinels, the integer constants as one-per-line anonymous enums, and the status
layout. **`apis.json`** (vendored, ~2 MB, as mpif already does) gives the
orthogonal half the header cannot: `param_direction`, which array is sized by
which other argument, `root_only`, `constant`, `func_type` for callback
parameters, and the `POLY*` prefix pairing each small form with its `_c` form.
Critically, it also gives each parameter's *kind* — which is what makes a rank
distinguishable from a tag (§5.4).

**Two compilers do the whole signature half of the verification, for free.** The
`libmpi_abi` side includes the ABI `mpi.h` only, so any signature the generator
gets wrong is a compile error. The `libmpiwrapper` side calls the implementation
directly, so the implementation's own declarations check every call site — a
stronger check than a typed function pointer, and it means no hand-maintained
function-signature table needs to exist. Every constant map is a generated
`switch` over the implementation's own macro names, so implementation-side
values are never transcribed by hand.

Language is Python, not Julia: none of mpif's Fortran-descriptor machinery
transfers, and a C project's contributors and CI already have Python. Use
`pympistandard` as a dev-time cross-check only, never a build dependency — it is
lightly maintained, has no tags, and its `LICENSE` file is empty. Vendor
`apis.json`, which is what it would provide anyway.

### Argument classes

Every parameter falls into one of these. Site counts are deliberately omitted:
the generator's frozen tallies are the authority, and a number written in prose
rots.

| class | what it emits |
|---|---|
| passthrough scalar (`int`, `MPI_Aint`, `MPI_Offset`, `MPI_Count`, `double`, `const char*`) | nothing |
| handle scalar, in | one conversion |
| handle scalar, out / inout | local of the implementation's type, then convert back |
| handle array, in | staged temporary |
| handle array, out / inout | staged temporary + write-back |
| status, scalar | §5.2, and the arity decides whether `MPI_ERROR` is preserved |
| status, array | §5.2, staged |
| choice buffer with sentinel (`MPI_BOTTOM`, `MPI_IN_PLACE`, `MPI_BUFFER_AUTOMATIC`) | one test |
| error code, return value | mapping call |
| error code, array (out) | in-place element mapping |
| rank | role-specific mapping (§5.4) |
| tag | role-specific mapping (§5.4) |
| **integer sentinel** (`MPI_DISPLACEMENT_CURRENT`) | one compare, in-direction only (§5.3) |
| other mapped integer constant (`MPI_ORDER_*`, `MPI_DISTRIBUTE_*`, `MPI_COMBINER_*`, `MPI_THREAD_*`, `MPI_TYPECLASS_*`, `MPI_LOCK_*`, `MPI_SEEK_*`, `MPI_Comm_split`'s `color`) | `switch` |
| bitmask constant (`MPI_MODE_*`) | OR-decomposition, by role (§5.5) |
| attribute keyval | mapping with a bias (§5.6) |
| callback (7 typedef families) | trampoline install (§6) |
| output string buffer with implementation-defined maximum | staged temporary, truncate or error (§5.8) |
| output string buffer *with* an explicit length | passthrough, both buffer and length |
| string array with sentinel (`MPI_ARGV_NULL`, `MPI_ARGVS_NULL`) | null test |
| weights array with sentinel (`MPI_UNWEIGHTED`, `MPI_WEIGHTS_EMPTY`) | one test |
| attribute value / extra state (`void*`) | passthrough — **except where the keyval decides the class** (§5.6), which is why the two attribute getters are hand-written |
| `MPI_T` handle classes | conversion (sentinel-shaped, not the eleven's machinery) |
| `MPI_T`'s `obj_handle` | class comes from a prior `get_info`, so the body queries first |
| varargs (`MPI_Pcontrol` only) | hand-written |

Per-function and per-parameter exceptions live in **named, prose-commented
tables keyed on `(routine, parameter)`** — never as `if name == "MPI_Foo"`
scattered through the generator body. mpif has only 8 such ad-hoc tests in 3346
lines, and that is why its special-casing stays auditable. Several of these
tables exist because `apis.json` cannot answer the question: where
`MPI_IN_PLACE` is legal, which parameter bounds an output string, which extents
must be asked of the implementation, and where a displacement may be the
sentinel.

### Naming convention in generated code

ABI-side names carry an `abi_` prefix; implementation-side names are bare. This
is a wrapper body in `libmpiwrapper`, so it *calls* `MPI_Send` and does not
define it:

```c
static int w_MPI_Send(const void *abi_buf, int abi_count,
                      MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                      MPIABI_Comm abi_comm)
{
  const void *const  buf      = abi_buf;
  const int          count    = abi_count;
  const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);
  const int          dest     = mpiwrapper_rank_fromabi(abi_dest);
  const int          tag      = mpiwrapper_tag_fromabi(abi_tag);
  const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);

  const int ierror = MPI_Send(buf, count, datatype, dest, tag, comm);
  return mpiwrapper_errorcode_toabi(ierror);
}
```

This is not cosmetic. It makes the load-bearing generator assertion a grep:
**no parameter of an ABI-typed signature may appear in the argument list of the
implementation call** — only locally declared converted values may. If the
generator emits `MPI_Send(abi_buf, ...)`, that is a hard stop.

The assertion is worth more than any single convenience, which is why
conditional write-backs are hoisted into a declared `T *const x_p = abi_x ? &x :
NULL;` rather than written `TARGET(..., abi_x ? &x : NULL, ...)` inline. **A
property worth a named local is worth not carving an exception into.**

**The tested code is the reference, not `examples/`.** The generator is required
to reproduce `dev/s1-reference/`, frozen, checked by `dev/check_prototype.py`.
`examples/` holds narrated excerpts, and `HISTORY.md` §5 lists the six ways they
have drifted — five of which compiled cleanly.

### One portability trap in generated switches

Case labels over *handles* must be **numeric, with the symbolic name in a
comment** — the integer families are ordinary enumerators and are switched by
name:

```c
case 0x00000209: return MPI_INT;  /* MPIABI_INT */
```

not `case (uintptr_t)MPIABI_INT:`. `MPIABI_INT` expands to
`((MPIABI_Datatype)0x00000209)`, and casting an integer constant to a pointer
type and back is not an integer constant expression in standard C — gcc and
clang accept it, but a case label is exactly where that extension is not worth
relying on. The generator parsed the header, so it has the numeric value and
transcribes nothing either way. The same fact is why the attribute-callback
sentinels cannot be `_Static_assert`ed and are checked at run time instead.

### Four disciplines lifted from mpif

- **Committed output, never hand-edited.** A bug goes back into the generator.
- **An on/off switch per axis**, so a refactor that should change nothing is
  *shown* to change nothing by regenerating to an empty diff.
- **Frozen tallies**, so a new `apis.json` or a new ABI header reclassifies
  loudly rather than silently. This includes the tallies that are **zero**:
  "deferred" stays frozen at 0 so that a future input carrying a class the
  generator cannot place fails there rather than quietly emitting one more stub.
- **Post-hoc assertions over the emitted text**, as above.

### Entry points the ABI declares and no implementation need define

Five entry points get **no vtable slot and no wrapper body**: `libmpi_abi`
answers them itself, in terms of the MPI-2 functions that replaced them.
`CODE.md` §5 lists them.

The reason is a limit of the availability probe, and it is worth stating as a
rule rather than as a fix. `dev/probe_impl.py` asks the *compiler* whether an
entry point is available, on purpose: §9 keeps it compile-only so the build
stays cross-compilable, and `nm` was rejected because a header may provide an
entry point as a macro. Against a conventional MPI, whose header and library
agree, that is exact. Against an MPI built from the ABI header it is not — Open
MPI main's `libmpi_abi` declares all 688 and defines 683 — and the failure mode
is the worst available: decision 6 promises that a missing entry point keeps its
slot and reports at run time, and instead the whole wrapper fails to **link**.

For these five the right answer was to stop forwarding rather than to forward
more carefully. They are not entry points an implementation happens to lack;
they are entry points the standard **removed**, and each is exactly the MPI-2
function that replaced it under an older name. What that buys is larger than the
link: they now work over *any* implementation with the MPI-2 attribute
interface. The general case — an entry point an implementation declares and does
not define — is still unhandled; §13.3 carries it.

Details that make the substitution exact rather than approximate: the set is
closed by the header (`deprecated: MPI-2.0`), not by memory, and a sixth fails
generation; the equivalence is *checked* — return type, arity and every
parameter type, with `MPI_Copy_function` and `MPI_Comm_copy_attr_function`
compared by what they expand to rather than by spelling; the sentinel values
agree (`MPI_NULL_COPY_FN` and `MPI_COMM_NULL_COPY_FN` are both `0x0`,
`MPI_DUP_FN` and `MPI_COMM_DUP_FN` both `0x1`) and that is tested at run time
rather than statically, for the case-label reason above; the shifted-name rule
survives, since `PMPI_Attr_get` reaches `PMPI_Comm_get_attr`; and the slots
really are *gone* rather than kept and left unfilled, so an old `libmpi_abi`
paired with a new `libmpiwrapper` fails the handshake instead of reaching a slot
that no longer means what it did.

---

## 4. Verified facts about the ABI and the two implementations

### 4.1 Handles are pointer-typed

```c
typedef struct MPI_ABI_Comm* MPI_Comm;
#define MPI_COMM_WORLD ((MPI_Comm)0x00000101)
```

An ABI handle is **pointer-sized**: 64 bits on LP64/LLP64, **32 bits on ILP32**.
Not a 64-bit integer. Consequence: any scheme relying on spare high bits is
unavailable on 32-bit targets, which is why the design below does not use one.

Predefined handle constants occupy `0x00000020`..`0x000002eb` — 103 values, all
< 748.

### 4.2 Status layouts

| | layout | sizeof (64-bit) | sizeof (32-bit) | private bytes |
|---|---|---|---|---|
| ABI | `int MPI_SOURCE, MPI_TAG, MPI_ERROR; int MPI_internal[5]` | 32 | 32 | **20** |
| MPICH | `int count_lo, count_hi_and_cancelled, MPI_SOURCE, MPI_TAG, MPI_ERROR` | 20 | 20 | 8, at the front |
| Open MPI | `int MPI_SOURCE, MPI_TAG, MPI_ERROR, _cancelled; size_t _ucount` | 24 | **20** | 12 / 8, at the back |

Open MPI's shrinks on 32-bit because `_ucount` is a `size_t` (measured on
`arm32v7`, not inferred). Nothing depends on the exact number; what matters is
the invariant **`sizeof(impl status) <= 32`**, and more precisely that the
private part fits the ABI's 20 scratch bytes. Both do, with room to spare — Open
MPI's 24 is its *total* size and only 12 of it is private, so a design that read
24 as "does not fit" would reject the simple scheme for no reason.

### 4.3 Integer constants differ, and differ inconsistently

| constant | ABI | MPICH | Open MPI |
|---|---|---|---|
| `MPI_ANY_SOURCE` | -1 | -2 | -1 |
| `MPI_ANY_TAG` | -2 | -1 | -1 |
| `MPI_PROC_NULL` | -3 | -1 | -2 |
| `MPI_ROOT` | -4 | -3 | -4 |
| `MPI_UNDEFINED` | -32766 | -32766 | -32766 |
| `MPI_ORDER_C` | 12 | 56 | 0 |
| `MPI_THREAD_MULTIPLE` | 4096 | 3 | 3 |
| `MPI_KEYVAL_INVALID` | 0 | 0x24000000 | -1 |
| `MPI_TAG_UB` | 501 | 0x64400001 | small int |
| `MPI_MODE_RDONLY` | 16 | 2 | 2 |
| `MPI_MODE_NOCHECK` | 1024 | 1024 | 1 |
| `MPI_ERR_LASTCODE` | 16383 | 0x3fffffff | 92 |
| `MPI_DISPLACEMENT_CURRENT` | `(MPI_Offset)-1` | -54278278 | -54278278 |
| `MPI_IN_PLACE` | `(void*)1` | `(void*)-1` | `(void*)1` |
| `MPI_BOTTOM` | `(void*)0` | `(void*)0` | `(void*)0` |
| `MPI_BUFFER_AUTOMATIC` | `(void*)2` | MPI-4.1 | MPI-4.1 |
| `MPI_STATUS_IGNORE` | `(MPI_Status*)0` | `(MPI_Status*)1` | `(MPI_Status*)0` |
| `MPI_STATUSES_IGNORE` | `(MPI_Status*)0` | `(MPI_Status*)1` | `(MPI_Status*)0` |
| `MPI_UNWEIGHTED` | `(int*)10` | — | `(int*)2` |
| `MPI_WEIGHTS_EMPTY` | `(int*)11` | — | `(int*)3` |
| `MPI_ARGV_NULL`, `MPI_ARGVS_NULL`, `MPI_ERRCODES_IGNORE` | NULL | NULL | NULL |
| `MPI_BSEND_OVERHEAD` | 512 | 96 | 128 |
| `MPI_T_PVAR_ALL_HANDLES` | 1 | `extern ... * const` | -1 |

`MPI_STATUS_IGNORE` and `MPI_STATUSES_IGNORE` are *the same value* in the ABI,
so they cannot be told apart — which is fine, since arity is known per call
site. But NULL must be mapped to MPICH's `(MPI_Status*)1`.

`MPI_DISPLACEMENT_CURRENT` is the same value in both implementations because
both use ROMIO for MPI-IO; it is not a core constant, which is why its guard is
probed separately.

`MPI_T_PVAR_ALL_HANDLES` on MPICH is a value that is not a constant expression
at all, so it can be no case label anywhere and both directions must be run-time
compares.

### 4.4 `MPI_MAX_*` and the Fortran status size

The ABI took the maximum over MPICH and Open MPI for every `MPI_MAX_*` (the stub
header documents both values in a comment beside each), so neither
implementation can overflow a buffer the application sized with an ABI constant.
That is a property of those two, not of the ABI.

| | `MPI_F_STATUS_SIZE` | index of SOURCE, TAG, ERROR |
|---|---|---|
| ABI | 8 | 0, 1, 2 |
| MPICH | 5 | 2, 3, 4 |
| Open MPI | `OMPI_FORTRAN_STATUS_SIZE` (6) | 0, 1, 2 |

8 ints = 32 bytes = exactly the ABI C status, with the named fields at the same
indices. **The ABI Fortran status is the ABI C status**, which is what
`doc/mpi.h.patch`'s `typedef MPI_Status MPI_F08_Status` assumes. Against MPICH
it is not: the named fields move. 8 ≥ 5 and 8 ≥ 6, so an mpif status buffer
sized by the ABI constant is never too small.

That sentence admits two readings and only one is right: the four converters do
*not* forward to the implementation's `MPI_Status_c2f`, they are a 32-byte copy
between two spellings of the same thing. `HISTORY.md` §1.14 has why forwarding
would be actively wrong.

**And neither do the other 22** (decision 23). All 26 converters — the four
status forms, the 11 classes' `_c2f`/`_f2c`, and the `_toint`/`_fromint` pairs
beside them — are answered from **the ABI's own handle values, with the
implementation never asked**. `MPI_Comm_c2f` *is* `MPI_Comm_toint` and
`MPI_Comm_f2c` *is* `MPI_Comm_fromint`; the same for every class.

The reason is that `MPI_Fint` is not in the ABI at all (§20.4), so the only
caller these functions can ever have is a Fortran binding layered *over* the
ABI — and such a layer holds ABI values in its INTEGER handles, because
§20.4.5 requires `_toint` to produce exactly those. So the class the standard
declines to define is fixed by the one consumer it can have. mpif implements
`MPI_Comm_c2f` as `return MPI_Comm_toint(comm)` and its `c2f` test fails
against anything else.

Two consequences worth stating separately, because both were wrong before:

- **No `MPIWRAPPER_HAVE_` guard, and no decision-6 stub.** These bodies touch
  no implementation entry point, so there is nothing for the availability probe
  to find missing. `MPI_Session_c2f` used to return 0 over an implementation
  without sessions; it now answers correctly over every implementation, because
  the question was never the implementation's to answer.
- **A round trip does not test this.** `f2c(c2f(h)) == h` holds under the
  forwarding semantics too. `HISTORY.md` §2.18 is the measurement, and
  `test/abi_converters_test.c` now compares against `_toint` instead.

---

## 5. Conversion rules

### 5.1 Handles

ABI → implementation is a dense `switch` over `0x20`..`0x2eb` (which the
compiler turns into a jump table), else a bit-cast.

Implementation → ABI needs the reverse: predefined implementation handle values
are *not* compile-time constants in general (Open MPI's are addresses), so the
map is built at initialization inside `mpiwrapper_get_vtable`. It is a **perfect
hash** — the whole key set is known by then, so a multiplier is searched for
until no two keys collide, making a lookup one multiply, one shift, one load and
one compare with no probe loop. `HISTORY.md` §1.18 has the measurements against
the alternatives, including why interpolation search is a trap.

Construction must be bounded and must fail **loudly at initialization** rather
than degrading to probing at run time, which would put the data-dependent branch
back: widen and retry, then refuse in `mpiwrapper_get_vtable` with a diagnostic.

**Collision.** A bit-cast dynamic implementation handle is wrong if it lands in
`0x20`..`0x2eb`. It never does today: MPICH's handles carry a kind field in the
high bits so all real handles are ≥ 0x04000000, and Open MPI's are object
addresses. But cross-compiling forbids probing this at configure time, and
32-bit targets have no spare high bits for a tagging scheme. So: **check in the
`toabi` direction only** — that is object creation, not every `MPI_Send` — and
fail with `MPI_ERR_INTERN`. A test also probes it at run time (§10).

**Two properties of both directions that are easy to get wrong:**

- **The `fromabi` default arm needs the range test too.** A value inside
  `0x20`..`0x2eb` that reached the default arm is an ABI predefined handle *this
  implementation does not provide* — the sized Fortran types are the realistic
  case — and bit-casting it hands the implementation a fabricated handle, which
  on MPICH is an `int` whose kind bits it will read. Returning the class's null
  handle instead makes the implementation reject the call with its own error
  code. The test is free: a dense switch has to bounds-check anyway.
- **Aliasing in the reverse map is normal and is not a collision.** An
  implementation may give two ABI-distinct predefined handles the same value —
  MPICH answers `MPI_DATATYPE_NULL` for five of the optional sized types, so 103
  ABI handles map onto 98 distinct MPICH values. The construction therefore
  distinguishes *same key inserted twice* (an alias: keep the first, which makes
  the ABI's own order canonical) from *different keys in one slot* (a real
  collision: retry with another multiplier). Conflating them would make map
  construction fail on MPICH and take the wrapper down at load.

### 5.2 Status

Copy the three named fields; `memcpy` the implementation's private bytes into
all 20 bytes of `MPI_internal`.

The private bytes are the complement of the named-field block: at the *front* in
MPICH, at the *back* in Open MPI. A head range and a tail range of
`offsetof`/`sizeof`-derived constant length cover both, and one of the two is
empty in each, so it compiles to a single 8-byte copy for MPICH and a single
12-byte copy for Open MPI. `_Static_assert` the contiguity of the three named
fields and `sizeof(impl status) - 12 <= 20`; an implementation that interleaves
private bytes between the named fields fails the build rather than being
silently mishandled.

`_ucount` is `size_t`-aligned while `MPI_internal` is `int`-aligned, so convert
through an aligned local and `memcpy`. Zero the unused private bytes on the way
out, and zero the whole implementation status before filling it on the way back
— not for correctness, but so ABI statuses are bit-reproducible and
implementation stack garbage never reaches the user, which matters under
valgrind or MSan.

**No validity marker and no synthesis fallback.** All 20 bytes hold the blob,
there is no second code path, and `MPI_Status_set_elements_x` is never called
except when forwarding it. `HISTORY.md` §1.13 has the design this replaced. What
makes the scheme sound in general — not just for these two implementations — is
that MPI requires statuses to be freely copyable by the user, which forces those
private bytes to be position-independent and self-contained.

**A single completion must not write `status.MPI_ERROR`.** MPI-5.0 §3.2.5
reserves that field: only the multiple-completion calls of §3.7.5 — the ones
that can return `MPI_ERR_IN_STATUS` — set it, and everything else must leave the
caller's field as it found it. That is free for a native implementation, which
writes the caller's status directly. It is not free here: the wrapper hands the
implementation a status temporary of its own, so the implementation *cannot*
honour the rule for it, and a straight copy-back overwrites the field with
whatever the temporary held.

So there are two conversion functions, and **the choice is by arity, not by
name**: a scalar OUT status preserves the caller's error field
(`mpiwrapper_status_toabi_keep_error`), an array of statuses does not
(`mpiwrapper_status_toabi`). That makes it one emitter decision rather than a
list, and it puts `MPI_Waitany`/`MPI_Testany` correctly on the preserving side —
they return a single status and are not §3.7.5 calls, however much they look
like their `*all`/`*some` siblings. The property is checkable from the emitted
text: every body with a scalar OUT status preserves, every body with a status
array does not.

**Only ten functions need an implementation status built from an ABI status:**
`MPI_Get_count`, `_count_c`, `MPI_Get_elements`, `_c`, `_x`,
`MPI_Test_cancelled`, `MPI_Status_set_cancelled`, and the three
`MPI_Status_set_elements*`. The rest are pure ABI-side:
`MPI_Status_get_source`/`_tag`/`_error` and
`MPI_Status_set_source`/`_tag`/`_error` touch only named fields, and all four
Fortran converters are memcpy-shaped.

Those six accessors are the only generated bodies emitted **without** a
`MPIWRAPPER_HAVE_` guard: the field they read or write is in the caller's own
ABI status and already carries the ABI's encoding, so nothing about them depends
on the implementation having the entry point — decision 6's stub would turn a
working answer into `MPI_ERR_UNSUPPORTED_OPERATION` over an MPI-3.1
implementation for no reason.

### 5.3 Sentinels

A sentinel is **a distinguished value inside a domain that is otherwise not
converted at all**, translated by one compare — as against a mapped family's
switch over a closed set. The word does not mean "not a constant"; every one of
them is a constant. It names the *role* a value plays in a parameter's domain,
and the role is what picks the mechanism.

**Pointer sentinels** (`MPI_BOTTOM`, `MPI_IN_PLACE`, `MPI_BUFFER_AUTOMATIC`,
`MPI_UNWEIGHTED`, `MPI_WEIGHTS_EMPTY`, `MPI_STATUS(ES)_IGNORE`,
`MPI_ARGV(S)_NULL`, `MPI_ERRCODES_IGNORE`) are fixed in the ABI and may be
non-constant in the implementation — possibly `extern void *`, i.e. constant at
link time but not at build time. One test per site.

Which sentinels are legal is a property of the *parameter*, not of the type, so
choice buffers get four mappers rather than one: send and receive (the C type's
constness decides which), each with and without `MPI_IN_PLACE`. `apis.json` does
not record where `MPI_IN_PLACE` is allowed — every choice buffer is just
`BUFFER` — so that is a named table, and it needs the receive form because
`MPI_Scatter` and `MPI_Scatterv` take `MPI_IN_PLACE` at the *receive* buffer.

**One sentinel is an integer**, and it needed a class of its own rather than a
kind. `MPI_DISPLACEMENT_CURRENT` is `(MPI_Offset)-1` in the ABI and `-54278278`
in ROMIO, so an untranslated `MPI_File_set_view` answers `MPI_ERR_ARG`. Three
reasons it is `DISPLACEMENT_SENTINEL`, a `(routine, parameter)` table with one
entry, rather than a mapped family:

- **per parameter, not per kind.** `apis.json` gives `disp` kind `OFFSET`, which
  is also every ordinary file offset in the library.
- **in-direction only.** `MPI_File_get_view`'s outgoing `disp` has the same kind
  and is a real displacement, so the symmetric `fromabi`/`toabi` pair a family
  generates would emit a reverse mapping that means nothing.
- **a compare, not a table**, since the rest of the domain is open.

It was missed for four stages, and the reason generalizes: the pointer sentinels
all sit on parameters that *already have a conversion point* in the emitted body
that a case can be added to. An `OFFSET` scalar is emitted as a bare passthrough
local — there was nothing to add a case to. **When looking for a missed
sentinel, look at the passthrough classes**, not at the ones that already
convert.

### 5.4 Ranks and tags are different classes

In the ABI, `MPI_ANY_TAG` is -2 and `MPI_PROC_NULL` is -3; in MPICH both
`MPI_ANY_TAG` and `MPI_PROC_NULL` are -1. So **an `int` cannot be translated
without knowing whether it is a rank or a tag**, and `rank_fromabi` and
`tag_fromabi` must be separate functions driven by the parameter's kind in
`apis.json`. This is load-bearing, not decorative — it is why `apis.json` is
required and the header alone is insufficient.

The same applies in the out direction: `status.MPI_SOURCE` can be
`MPI_PROC_NULL` or `MPI_ANY_SOURCE`, and `MPI_Group_translate_ranks` can return
`MPI_UNDEFINED`. Out-ranks are mapped **uniformly**, including where a per-site
argument says the sentinel cannot occur: `MPI_Comm_rank`'s answer is never a
sentinel and `MPI_Group_rank`'s is `MPI_UNDEFINED`, one function away.
Uniformity is the correctness property; per-site reasoning is what a generator
must not encode.

### 5.5 Bitmasks

`MPI_MODE_*` are OR-combined and the bit assignments differ completely (ABI
`RDONLY` = 16 against 2 in both implementations; `NOCHECK` 1024 against Open
MPI's 1). These need OR-decomposition, not a `switch`. Sites: `MPI_File_open`'s
`amode`, and the `assert` argument of `MPI_Win_post`/`_start`/`_fence`/`_lock`.

**The role belongs in the function name**, exactly as for ranks and tags:
`filemode_fromabi`/`_toabi` and `winassert_fromabi`/`_toabi`, chosen per
parameter from `apis.json`. The ABI puts file modes and window asserts in one
enum with disjoint bits, which makes a single mapper look sufficient; Open MPI
gives the two families *the same bits*, so an implementation-side `1` is
`CREATE` or `NOCHECK` depending only on which parameter it came from
(`HISTORY.md` §2.10). The out direction is not hypothetical:
`MPI_File_get_amode` returns one.

### 5.6 Values whose class is not in their own type

Three families where the signature does not say what a value means.

- **Keyvals** are plain `int`s handed out by the implementation at run time, and
  can collide with the ABI's predefined values at 501–507 and 601–605. The
  thirteen predefined attribute keys convert through a generated switch like
  every other family; a dynamic keyval cannot, because it is an `int` with no
  structure and no pointer slack to tag around. So the switch's default arm asks
  `src/mpiwrapper/keyvals.c` instead of passing the value through, and **the
  ABI-side value is ours to choose** — drawn from a base far above any
  predefined key, which makes "predefined or dynamic" a property of the value
  and puts the collision beyond reach by construction rather than by luck. The
  reverse direction scans newest-first, because an implementation may reuse the
  number of a freed keyval and the entry that matters is then the most recent
  registration. Nothing is ever removed, per §6.2.
- **Dynamic error codes.** `MPI_ERR_LASTCODE` is 16383 in the ABI against
  MPICH's 0x3fffffff, so codes from `MPI_Add_error_class`/`_code` must be
  *renumbered* into the ABI's range, not passed through. The ABI-side value is
  drawn from just *above* `MPI_ERR_LASTCODE` rather than capped at it: MPI-5.0
  §9.5 puts every dynamic class above the implementation's own last code, and
  above 16383 is where an application comparing against `MPI_ERR_LASTCODE`
  expects to find one.

  **The registry has to intern the implementation's codes as well as the
  application's**, which is not obvious from the standard and is forced by
  MPICH: it answers essentially every error with an instance-specific code
  (`HISTORY.md` §2.9), so a default arm answering `MPI_ERR_OTHER` means no
  application can ever be told `MPI_ERR_NO_SUCH_FILE`. With the interning, the
  ABI-side value converts back down and `MPI_Error_class` and `MPI_Error_string`
  reach the implementation's own class and its own message. A code the table
  never issued still answers `MPI_ERR_OTHER`, which is also what a full table
  falls back to.
- **An attribute's *value* is a converted class, and no signature says so.**
  `MPI_Comm_get_attr` and `MPI_Win_get_attr` hand back a `void *` whose meaning
  is whatever the *keyval* says it is, and for five of the thirteen predefined
  keys that meaning is a family this library maps: `MPI_HOST` and `MPI_IO` are
  ranks, `MPI_LASTUSEDCODE` is an error code, `MPI_WIN_CREATE_FLAVOR` and
  `MPI_WIN_MODEL` are enumerations. This is the one conversion class that **no
  rule keyed on a parameter's type can find**, which is why the two getters are
  in the ledger and not generated, and why it took an outside oracle to notice
  (`HISTORY.md` §3, S7).

  `MPI_LASTUSEDCODE` also closes a gap the error-code registry could not: what
  it answers is not a conversion of the implementation's number but
  `mpiwrapper_errorcode_lastused()`, the largest code *this* library can hand
  out — every code an application can be handed here has been through the
  mapping above, so interning the implementation's maximum would produce a value
  in our range that bounds nothing. An empty registry answers `MPI_ERR_LASTCODE`
  exactly, which is the floor MPI-5.0 §9.5 requires.

Generalized requests and datarep names were once listed here as needing the same
treatment. **They do not** — both registrars take an `extra_state` argument that
MPI hands back to every callback, so the `{user_fn, user_extra}` pair *is* the
registry (`src/mpiwrapper/extrastate.c`). `MPI_T`'s events are the one family
that genuinely cannot, and §6.1 says why.

### 5.7 Arrays: always temporaries, never in place

An earlier design contemplated rewriting array arguments in place. It does not
survive contact with the standard, and the argument is kept in full because the
narrower version of the idea is still tempting.

- §6.12 (nonblocking collectives): "Once initiated, all associated send buffers
  and buffers associated with input arguments (such as arrays of counts,
  displacements, or datatypes in the vector versions of the collectives) should
  not be modified, and all associated receive buffers should not be accessed,
  until the collective operation completes."
- §6.13 (persistent collectives): "After initialization, all arrays associated
  with input arguments ... must not be modified until the corresponding
  persistent request is freed with MPI_REQUEST_FREE."

Both forbid the *user* from modifying. **Neither forbids the user from
reading.** So an application may legally read its own datatype array while a
nonblocking collective is in flight, and in-place conversion would show it
implementation handles.

**And note where each sentence puts the obligation, because the natural reading
is the wrong way round.** Both put it on the *user*, not on the implementation.
There is no rule that an implementation must copy the counts, displacements and
datatype arrays out at initiation — it is explicitly permitted to keep reading
them until the operation completes, and §6.12's own advice to implementors
describes exactly the kind of design that would. A search of MPI-5.0 for any
statement that the arrays are copied finds none.

That is what forces the request-keyed table, and it is the whole reason it
exists: the arrays the implementation reads are *ours*, not the caller's, so
they must survive as long as the implementation may read them. If the standard
did require the implementation to copy, every one of these temporaries could be
scoped to the call and §6.3's table, its tombstones and its lock word would all
be deletable.

A side effect worth knowing, since it is a behavioural difference from a native
MPI rather than a conformance question: because *we* copy at initiation, an
application that violates the rule and modifies its ABI-side arrays after
posting still gets the right answer through the wrapper, where it might not
natively. We are more permissive than the standard requires, in the direction
that hides a user bug.

Four independent reasons, each sufficient:

1. **`const` may mean physically read-only.** An application's `static const
   MPI_Datatype types[3]` lives in `.rodata`; writing to it crashes a legal
   program.
2. **The same array may be in two MPI calls at once.** Two threads calling
   `MPI_Alltoallw` on different communicators with one shared `types[]` array is
   legal. In-place conversion races, and even convert-then-restore is visible to
   the other call. This kills in-place even where `const` could be cast away.
3. **For nonblocking there is no restore point.** The array would have to stay
   converted for the whole operation, exactly while the application may read it.
4. **Error paths.** Every early return would have to un-convert.

**Rules:**

- **Allocation:** a fixed-size local buffer with a heap fallback above a *byte*
  threshold. Not a VLA (optional in C11, absent in MSVC, and unbounded means a
  stack overflow at 100k ranks) and not `alloca`. Threshold in bytes rather than
  elements, so status arrays do not blow the budget. Allocation failure returns
  `MPI_ERR_INTERN`.
- **Blocking:** scoped to the call.
- **Nonblocking:** heap, owned by the request, freed at completion.
- **Persistent:** freed at `MPI_Request_free`, **not** at completion — the
  request is re-armed by `MPI_Start` repeatedly, so freeing at completion is a
  use-after-free on the second `MPI_Start`. `MPI_Waitall` can mix both kinds.

  **This needs no flag in the request map.** The two lifetimes have one
  discriminator and the implementation maintains it: a request it has set to
  `MPI_REQUEST_NULL` is finished with, and it nulls a nonblocking request at
  completion and a persistent one at `MPI_Request_free` — and *not* at a
  persistent request's completion, which is precisely the case an early free
  would corrupt. So "release the pre-call handle wherever the post-call handle
  is null" is the whole rule, emitted identically at every completion site.
- Whether a temporary outlives its call is a property of the *signature*, not of
  the routine's name: an in-direction array that has to be converted, in a
  routine that hands back a request. That predicate produces exactly eight
  entry points, and the count is a frozen tally, so a ninth must be admitted
  deliberately.

  **The narrowing fallback (§5.10) makes the predicate arm-dependent.** A `_c`
  vector collective's count and displacement arrays cross as pointer casts in
  the primary body and as staged `int` arrays in the narrowing body, so the
  nonblocking and persistent vector forms stage past return **only where the
  implementation lacks the `_c` entry point**. Rather than mutate the eight into
  one number covering both arms and lose the distinction, the generator freezes
  the two separately: `staged past return` stays **8** and describes the primary
  bodies, and `narrowed staged past return` is **18**. A nineteenth still has to
  be admitted deliberately, which is the whole point of freezing either number.

  The two sets overlap rather than nest: four of the eighteen — the `_c`
  spellings of `Alltoallw_init`, `Ialltoallw`, `Neighbor_alltoallw_init` and
  `Ineighbor_alltoallw` — are already in the eight, because their *datatype*
  arrays are staged in both arms. What the fallback adds to those four is a
  second element type in the same block, which is why `mpiwrapper_staged_next`
  exists.
- **`MPI_STATUSES_IGNORE`** (NULL in the ABI) must short-circuit before any
  temporary is allocated.
- **Stage for value mapping or for representation, never for spelling.** What an
  ABI fixes is representation, and `dev/type-identity/` measured size and
  signedness identical for `MPI_Aint`, `MPI_Count`, `MPI_Offset` and `MPI_Fint`
  everywhere tried; where the spellings differ the cost is a cast, licensed by
  the `_Static_assert` battery in `internal.h`. So displacement arrays and the
  `_c` forms' count arrays pass straight through, and only the datatype arrays
  beside them are staged. `HISTORY.md` §1.9 has the rule this replaced.
- **An OUT array the implementation fills only partly needs two extents.**
  `MPI_Graph_get`, `MPI_Graph_neighbors`, `MPI_Dist_graph_neighbors` and
  `MPI_Type_get_contents` take a caller's maximum and write however much the
  object actually has; converting the tail would convert uninitialized elements
  — a wrong answer wherever the garbage collides with an implementation
  sentinel, and a sanitizer report always. So those carry an allocation extent
  and a smaller conversion extent.
- **An extent `apis.json` records as `*` is not one class.** The communicator's
  size sizes `MPI_Alltoallw`'s datatype arrays (the *remote* size on an
  intercommunicator); two different degrees size the neighbourhood forms' send
  and receive arrays; the last entry of the index array sizes
  `MPI_Graph_create`'s edges; the degrees *sum* to `MPI_Dist_graph_create`'s
  destinations. Each is a named `(routine, parameter)` entry, and the ones that
  must ask the implementation go through `src/mpiwrapper/extents.c` **before**
  the call they serve, so a failure returns the implementation's own error and
  nothing has been allocated. The errors coincide: asking a communicator with no
  topology for its neighbour counts fails with the `MPI_ERR_TOPOLOGY` the
  neighbourhood collective would itself have returned.
- **Two arrays must not be read at all.** With `MPI_IN_PLACE` at `sendbuf`,
  MPI-5.0 §6.11 makes `sendcounts`, `sdispls` and `sendtypes` *ignored*, and a
  legal program may pass a null pointer for them. Harmless for the two the
  wrapper forwards and fatal for the datatype array it reads element by element,
  so the six non-neighbourhood `alltoallw` forms fill their staged send array
  with `MPI_DATATYPE_NULL` in that case. Measured: both implementations accept
  such a call with all three arguments null.
- **`MPI_Type_get_contents`' `max_datatypes` is not forwarded.** The standard
  makes it an upper bound, so the wrapper passes the *envelope's* count instead
  and stages an array that size — which satisfies "at least as large as"
  exactly, is what the implementation was going to write either way, and keeps
  our staged array's uninitialized tail out of reach of an implementation that
  walks the whole maximum (`HISTORY.md` §2.7). A caller's too-*small* maximum
  still reaches the implementation and is still rejected, because the
  substitution is a minimum.
- **One array crosses unconverted although its kind says otherwise.**
  `MPI_Group_range_incl`'s `ranges` is `int[][3]` and `apis.json` calls the whole
  triplet a `RANK`. Two thirds of it is one; the third column is a *stride*, and
  mapping it would be a wrong answer rather than a redundant one — a stride of
  -1 is `MPI_ANY_SOURCE`'s ABI value and would reach MPICH as -2. Neither
  genuine rank can be a sentinel, since both name a member of the group, so the
  whole array passes through.

**The one legitimate in-place case:** OUT arrays whose element type is the same
size and which need only value mapping — `array_of_errcodes` in
`MPI_Comm_spawn`, `ranks2` in `MPI_Group_translate_ranks`. Let the
implementation write into the user's array, then map each element in place. No
`const`, no concurrent-read expectation, no packing, no restore path.
`array_of_indices` in `MPI_Waitsome` needs no mapping at all.

**The same bug class on scalars:** any out or inout handle needs a local of the
implementation's type; never reinterpret the ABI storage.
`MPI_Comm_free(MPI_Comm *comm)` forwarding `(MPI_Comm *)abi_comm` writes MPICH's
4-byte handle into an 8-byte ABI slot and leaves the upper half garbage — while
on Open MPI it works by accident, so it would pass tests on one implementation
and corrupt on the other. No `_Static_assert` catches this; the "no ABI-typed
parameter reaches the implementation call" assertion does.

### 5.8 Output string buffers

Ten functions have an output string buffer and **no explicit length argument**,
so an implementation whose `MPI_MAX_*` exceeds the ABI's would write past the
caller's array: `MPI_Error_string`, `MPI_Get_library_version`,
`MPI_Get_processor_name`, `MPI_Comm_get_name`, `MPI_Type_get_name`,
`MPI_Win_get_name`, `MPI_Info_get_nthkey`, `MPI_Open_port`, `MPI_Lookup_name`,
and `MPI_File_get_view`'s `datarep`. Those ten are hand-written.

Everything else is safe because the caller passes the size, and **that set is a
named table too** (`STRING_OUT_LENGTH`), giving the parameter that bounds each
buffer and checked against the signature — because that naming is the *only*
thing separating the safe class from the dangerous one. It is `MPI_Info_get`'s
`valuelen`, `MPI_Info_get_string`'s `buflen`, `MPI_Session_get_nth_pset`'s
`pset_len`, and MPI_T's twelve. `MPI_MAX_STRINGTAG_LEN` is an input limit only.

**`MPI_Get_library_version` is the one that does not merely copy** (decision
26). It puts a two-line banner in front of the wrapped library's own string:

```
mpi_abi_wrapper 1.0.0 (MPI 5.0 standard ABI, MPI_ABI_VERSION 1.0)
wrapping:
MPICH Version:      4.3.1
...
```

The reason is that **after decision 24 nothing else tells an application there
is a shim here at all**. `MPI_Get_version` reports the standard this library
presents rather than the wrapped one's, and `MPI_Abi_get_version` reports the
ABI; both are answers about the ABI rather than about the implementation of it.
The library version string is where a person looks and what a bug report
pastes, so it is where the wrapper's own identity belongs.

It is *additive*, which is the property that keeps it safe: the implementation's
text follows unchanged, every implementation names itself in that text, and
consumers grep it — mpif's `MPIF_TEST_MPI_LIBRARY` does, which is why the
banner names no implementation of its own. The four version numbers in it are
stringified from the header rather than typed, per `CLAUDE.md`'s rule about
writing down how to re-derive a number.

Two consequences worth stating: the staged buffer grows by the banner and the
implementation writes into the middle of it, so the composition costs no second
copy and carries the implementation's own `resultlen` convention through
unchanged; and **truncation becomes reachable**, where §4.4's "the ABI took the
maximum over both" had made it impossible. A banner in front of a maximal
8192-byte MPICH string exceeds the ABI's 8192. It truncates, per the table
below, and the tail of the wrapped banner is the right end to lose — real ones
run to one or two kilobytes, and the wrapper's line is the part a reader cannot
reconstruct.

**Truncate or error is a per-parameter judgement**, and belongs in the named
`(routine, parameter)` table:

- **Prose → truncate silently**, which is what an implementation does with its
  own too-short buffer anyway: `MPI_Error_string`, `MPI_Get_library_version`,
  and the three `*_get_name`.
- **Identifiers fed back into MPI → return an error**, because a truncated one
  fails mysteriously much later: `MPI_Open_port` and `MPI_Lookup_name` (a
  truncated port name fails at connect), `MPI_Info_get_nthkey` (used to look up
  a value), `MPI_File_get_view`'s `datarep`.
- `MPI_Get_processor_name` is the awkward one: it reads as prose, but
  applications use it for rank-to-node mapping, where truncation can make two
  nodes indistinguishable — a silently wrong answer. Truncate anyway, since MPI
  already permits implementations to truncate to their own maximum, but the
  table entry needs a comment explaining the call.

**Always stage; never `#if` the staging.** Conditional on `impl_max > ABI_max`,
the temporary never compiles on MPICH or Open MPI, so the first implementation
that needs it runs code nobody has executed. All ten are cold paths, so staging
unconditionally costs nothing and keeps one code shape exercised on every run.
On the error paths return `MPI_ERR_INTERN` (the limitation is ours, not the
caller's) *and* write the truncated NUL-terminated string anyway, so a caller
that ignores the return code does not read uninitialized memory.

**What `*resultlen` means.** Set it to what was actually copied, so
`string[*resultlen] == '\0'` holds — and *not* to `strlen(string)`, which is the
tempting stronger reading and is not one implementations honour: Open MPI
5.0.6's `MPI_Get_library_version` reports 119 for a 118-character string. So the
implementation's own count passes through unchanged, and the property to test is
the NUL, not the equality.

The opposite direction is not fixable and is correct to pass through: where the
implementation's limit is *smaller* than the ABI's (Open MPI's
`MPI_MAX_INFO_KEY` is 36 against the ABI's 256), a long key is rejected by the
implementation with `MPI_ERR_INFO_KEY`, which we map and return.

### 5.9 When to assert at compile time

**Static-assert where a runtime check would cost something on a hot path; handle
at run time where the check is free.**

But "handle it rather than fail the build" is not universal, and the
discriminator is **whether the degraded behaviour announces itself.** A
truncated error string is visibly truncated, so handling `MPI_MAX_*` at run time
is right. A silent fallback from `PMPI_X` to `MPI_X` where the shifted name was
missing would be invisible, and would quietly reintroduce the defect the
separate slot exists to prevent — there a link error naming the missing symbol
is the better outcome, and it costs nothing since both names always exist. So:
handle it at run time when the fallback is observable, fail the build when it is
not.

| | why |
|---|---|
| `MPI_MAX_*` | cold paths only, and truncation is visible → run time |
| `sizeof(MPI_Count)`/`MPI_Aint`/`MPI_Offset` | a narrowing check would land on `MPI_Send_c` and every large-count call → `_Static_assert` |
| a large-count value against the *small* twin's `int` | a different question from the row above, and it goes the other way: there is no build-time answer, because whether the small twin is called at all depends on what the implementation has → run time (§5.10) |
| status layout | **no runtime recourse exists** — nowhere to put a private part exceeding 20 bytes, and side storage keyed on a status address is unsound because statuses are freely copied → build failure |
| dynamic handle collision | one compare, and only on object creation → run time |

---

### 5.10 Narrowing a large-count call onto a small-count implementation

The ABI's surface is MPI-5.0 and includes **159 `_c` entry points**. An
implementation below MPI-4.0 has none of them — every released Open MPI, and
MPICH before 4.0 — and decision 6 used to answer all 159 with
`MPI_ERR_UNSUPPORTED_OPERATION`. It no longer does. Where the `_c` form is
missing but its small twin is present, the wrapper calls the small twin.

**Why this is worth the machinery, stated once because the obvious reading is
wrong:** `_c` is not the big-message API, it is *the* API. A program compiled
against MPI-4 C bindings calls `MPI_Send_c` with `count = 1`. The narrowing
fallback is therefore not a large-message feature — it is what makes the
large-count half of the ABI usable at all on an implementation that predates it,
and the ceiling it introduces sits above every count real programs pass.

**Four fates, and which applies is a property of the signature.** The generator
decides per entry point by diffing the `_c` prototype against its small twin;
nothing here is keyed on a name.

1. **Exact, by an `_x` twin.** MPI-3.0 already answers five of these questions
   in `MPI_Count`: `MPI_Type_size_x`, `MPI_Type_get_extent_x`,
   `MPI_Type_get_true_extent_x`, `MPI_Get_elements_x`,
   `MPI_Status_set_elements_x`. Prefer them — they are exact, need no guard, and
   are present on everything at the enforced MPI-3.0 floor. Deprecated in
   MPI-4.1 means "do not write new code against it", not "absent" (§8).
2. **Exact, by widening.** An out parameter that the small twin reports as `int`
   or `MPI_Aint` always fits in an `MPI_Count`. Six entry points, no guard.
3. **Narrowing, guarded.** Every in-direction value must be checked against the
   small twin's type before the call.
4. **No small twin either.** Decision 6's stub, unchanged.

**The error class is `MPI_ERR_VALUE_TOO_LARGE`, not
`MPI_ERR_UNSUPPORTED_OPERATION`.** MPI-4.0 has a class meaning exactly "a value
is too large to be stored in the given parameter" and this is that case. Keeping
the two apart is what lets a caller tell "this wrapper has no such entry point"
from "the entry point is here and your value will not fit" — and the two need
different responses, since the first is permanent and the second depends on the
argument. It also keeps `test/`'s own `unsupported()` helper honest: that helper
treats `MPI_ERR_UNSUPPORTED_OPERATION` as *skip this check*, so returning it for
an oversized count would turn a real failure into a silent skip at 46 call
sites.

**Arrays must be staged, and this is where the fallback stops being free.** A
count or displacement array crosses today as a pointer cast, because the ABI and
the implementation agree on the representation of `MPI_Count` and `MPI_Aint`
(`HISTORY.md` §1.9). Narrowed to `int` they do not agree, so §5.7's rule applies
in full: allocate, convert element by element, and — for a routine that hands
back a request — keep the block alive past return. That is a per-call allocation
and an O(*n*) loop where there was a cast, on the v- and w-collectives, whose
*n* is the communicator's size.

**`root_only` is the hazard, and it is the one that can crash a legal program.**
`MPI_Gatherv`'s `recvcounts` and `displs`, and `MPI_Scatterv`'s `sendcounts` and
`displs`, are significant **only at the root**. Nothing reads them today, so a
non-root rank may legally pass a null pointer or an uninitialized array.
Narrowing has to read them, and reading them at a non-root rank faults on a
program that was doing nothing wrong. So a narrowed array whose parameter is
marked `root_only` in `dev/apis.json` is staged **only where it is
significant** — which on an intercommunicator means testing `root` against
`MPI_ROOT` rather than comparing it to the local rank. `apis.json` carries the
flag on 59 parameters and the generator ignored it entirely before this rule
existed.

**What a caller can observe that a native MPI-4 implementation would not.** The
envelope of a datatype is a property of the *constructor*, not of the values:
`MPI_Type_contiguous_c(5, MPI_INT)` reports one large count and zero integers on
MPICH, and the small `MPI_Type_get_envelope` **refuses** such a type with
`MPI_ERR_TYPE` and the message "use MPI_Type_get_envelope_c to query large count
datatype". Measured in `dev/large-count-envelope/`. Since the fallback builds
every datatype through a small-count constructor, its envelope and contents
answers are internally consistent and are exactly right for the types that can
exist in such a build — but they differ from a native MPI-4 implementation's,
and the small envelope succeeds where a native one refuses. §13.2 carries it as
a limitation; the alternative is a side table of which types the caller built
through the large-count entry points, keyed on a handle the implementation owns
and recycles, which is the unsound shape §5.2 rejects for statuses.

---

## 6. Callbacks

### 6.1 Which need trampoline pools

Seven typedef families, **16** registration functions. The ones with an
extra-state argument can carry a heap-allocated `{user_fn, user_extra}` pair;
the ones without need a pool of generated static trampolines, each knowing its
own index.

| registrar | mechanism |
|---|---|
| `MPI_Op_create`, `_c` | **pool** (2, one per `MPI_User_function` variant) |
| `MPI_Comm`/`File`/`Win`/`Session_create_errhandler` | **pool** (4) |
| `MPI_Comm`/`Type`/`Win_create_keyval`, `MPI_Keyval_create` | extra state |
| `MPI_Grequest_start` | extra state |
| `MPI_Register_datarep`, `_c` | extra state |
| `MPI_T_event_register_callback` | registration-keyed map |
| `MPI_T_event_set_dropped_handler` | registration-keyed map |
| `MPI_T_event_handle_free` | its own `user_data` carries the pair |

Fifteen of the sixteen are in the ledger; `MPI_Keyval_create` is not, because it
is `MPI_Comm_create_keyval` under a name MPI-3.0 deleted (§3), so the trampoline
judgement is made once rather than twice.

Error handlers are the non-obvious pool case: `MPI_Comm_errhandler_function` and
its three siblings have no extra-state argument, so `MPI_Op_create` is *not* the
only exception.

**The discriminator is a callback-*typed parameter*, not the word "callback" in
an argument class.** `MPI_T_event_callback_get_info` and `_set_info` carry a
`CALLBACK_SAFETY`, which is an enumerator naming a safety level and converts
like any other enum, so they are generated. `MPI_T_event_handle_free` carries an
`MPI_T_event_free_cb_function`, which runs on the way back *into* user code and
must convert an implementation registration handle and `cb_safety` to the ABI's
— so it is a registrar, and the rule found it where a list had not.

`MPI_T` events need a map rather than a pool: `MPI_T_event_set_dropped_handler`
has no `user_data` parameter even though `MPI_T_event_dropped_cb_function` takes
one, so state cannot be smuggled through the registration — but both callbacks
receive `event_registration`, a handle we convert anyway, so one map keyed on
the implementation's registration handle serves both.

A user reduction trampoline receives an implementation datatype and must convert
it back to an ABI datatype, so it needs the reverse predefined-handle map
(§5.1).

**The registrars live apart from their mechanism, deliberately.** Each exists
twice — once calling the implementation's `MPI_` name and once its `PMPI_` one
(decision 7) — and the files that own the trampolines must name neither.

**The predefined attribute functions are sentinels, and a body must not get this
wrong.** The ABI spells `MPI_COMM_NULL_COPY_FN` as `(function *)0x0` and
`MPI_COMM_DUP_FN` as `(function *)0x1`; both implementations spell them as real
functions with real addresses (Open MPI's is `OMPI_C_MPI_COMM_DUP_FN`). So they
are recognized and replaced with the implementation's own, exactly like
`MPI_BOTTOM`, and a trampoline must *not* be installed for them — wrapping
`MPI_COMM_DUP_FN` would turn a copy the implementation performs internally into
a call into user code that is not there. `MPI_CONVERSION_FN_NULL` is the same
shape for datareps.

Errhandler trampolines must be **declared variadic**, matching
`MPI_Comm_errhandler_function`'s `...`. Nothing needs forwarding — the extra
arguments are implementation-specific and the user's ABI-side function is
variadic too — but variadic and non-variadic calling conventions differ on arm64
macOS, so a non-variadic declaration would be a silent ABI bug. Taking the type
from the implementation's own typedef gets this right for free. A useful related
property: an errhandler trampoline runs when the process is already in trouble,
and a pool lookup allocates nothing.

### 6.2 Lifetimes: almost nothing can be reclaimed

MPI-5.0 §2.5.2 governs: "A call to a deallocate routine invalidates the handle
and marks the object for deallocation... MPI need not deallocate the object
immediately. Any operation pending (at the time of the deallocate) and decoupled
MPI activity (see Section 2.9) that involves this object will complete normally;
the object will be deallocated afterwards."

**So a free call is not a reclamation point.** Checked against the standard for
all sixteen:

| registrar | safe reclamation point |
|---|---|
| `MPI_Op_create`, `_c` | **none observable** (§2.5.2) |
| `*_create_errhandler` | **none**: §9.4 "deallocated after all the objects associated with it (communicator, window, or file) have been deallocated" |
| `*_create_keyval` | **none before finalize**: §7.7 "not erroneous to free an attribute key that is in use, because the actual free does not transpire until after all references ... have been freed" |
| `MPI_Grequest_start` | **our `free_fn` trampoline** — invoked exactly once, object deallocated after it returns |
| `MPI_Register_datarep`, `_c` | **never** — MPI has no deregistration call |
| `MPI_T_event_register_callback` | **the free callback**: §15.3 invoked "when it is able to guarantee that no further event instances ... will be raised" |
| `MPI_T_event_set_dropped_handler` | same |
| `MPI_T_event_handle_free` | its own `free_cb_function`, invoked once when the registration is released |

Consequences:

- **Op slots are not reclaimed at `MPI_Op_free`.** Precise reclamation would
  need a per-slot refcount incremented by every operation that can invoke the op
  and decremented on completion: the six reduction families in blocking,
  nonblocking and persistent form (×2 for `_c`), ~36 functions, plus
  `MPI_Start`/`MPI_Startall` re-arming a persistent reduction. RMA is *excluded*
  — §12.3.4 says of `MPI_Accumulate`'s `op` that "user-defined functions cannot
  be used". **v1 does not reclaim.** Not reclaiming is trivially safe, whereas a
  refcount bug is a use-after-free surfacing as a wrong reduction result at
  scale.
- **Errhandler slots are permanent.** The association is with communicators,
  windows, files and sessions, inherited through `MPI_Comm_dup` and surviving to
  `MPI_Finalize`; tracking it means reimplementing the implementation's
  refcounting.
- **Keyval pairs are never freed.** Not even at finalize: the implementation
  invokes attribute delete callbacks on `MPI_COMM_SELF` from inside
  `MPI_Finalize` (§11.4.1), so the earliest safe point is after the
  implementation's `MPI_Finalize` returns, which is worthless when the process
  is exiting.
- **Generalized requests are the one clean case:** free the pair inside our
  `free_fn` trampoline after calling the user's, unconditionally — MPI
  deallocates even if the user's `free_fn` returns an error.
- **`MPI_T`'s event registrations are the second.** Ours is installed even when
  the application passes no free callback of its own, since otherwise a tool
  that allocates and frees registrations in a loop would fill a map it never
  asked for.

**"Not reclaimed" is reached by ordinary programs, and the numbers are lower
than they read.** MPICH's own C suite exhausts two of these tables:
`attr/fkeyvaltype` creates 32,768 keyvals in one process against 1024 slots, and
`coll/nonblocking3` creates several thousand user ops against 1024 trampolines
while freeing each one with a nonblocking reduction still in flight — the case
the table calls "none observable", demonstrating why. **Neither is fixable by
raising the number:** the op pool is *code*, and the keyval rule that forbids
reclamation is the standard's. The honest statement is the one in the table — a
process that creates unboundedly many ops or keyvals over its lifetime is
outside what this design serves. §13.2 keeps it as a stated limitation rather
than a bug.

### 6.3 Concurrency

All shared tables are fixed-capacity and lock-free: CAS slot allocation for the
trampoline pools, atomic append for the keyval / error-code / datarep maps,
open-addressing CAS insert for the staged-request hash, release/acquire on the
vtable pointer. No mutex anywhere. Overflow is a documented limit returning
`MPI_ERR_INTERN`.

Staged temporaries that must outlive their call live in the request-keyed hash,
guarded by a global atomic count so that completion calls pay one relaxed load
and a compare against zero when the application never uses those routines.

**That hash is keyed on the implementation's request handle, and such a handle
does not uniquely identify an operation.** That is the assumption the whole table
rests on, and `dev/request-identity/` measures it false on both implementations
(`HISTORY.md` §2.6, §2.6a): an operation already complete on return gets a
*shared built-in* request, and a completed handle's value is reused immediately.

**Both doors into the table were open, and the attach door was thought shut.**
Only eight entry points ever attach, and it was recorded here that the
shared-request rows are therefore not attach collisions. That was false. Open
MPI answers *any* libnbc collective whose schedule turns out empty with
`ompi_request_empty` — `NBC_Schedule_request`, one rule over all of them — and
`a2aw_sched_linear` skips zero-span sends and receives, so an `MPI_Ialltoallw`
in which this rank exchanges nothing with anybody has an empty schedule on a
communicator of any size. Two of those posted before either is waited on hand
the table one key twice. Measured in the default configuration, and the refusal
reproduced end to end (`dev/request-identity/`). **§13.2's (b) shut that door**:
a shared built-in is only ever handed out for an operation already complete on
return, and such an operation no longer attaches at all.

The release door is open as well and always was: **every completion call
releases by handle value**, so a wait on a `MPI_PROC_NULL` `MPI_Isend` does look
its key up, and where an implementation shares one object across kinds it can
free a block belonging to a different operation. That door is wider than
"`MPI_PROC_NULL`" suggests — both implementations answer an *eagerly completed
ordinary short send* with the same built-in, so everyday traffic releases this
key.

That is believed safe for a reason worth stating precisely, because it is what
bounds the whole scheme — and because it is **an inference, not a citation**:
MPI requires every request to be independently testable and completable, so two
*live* operations should not be able to share a handle. An implementation that
shared one between two incomplete operations could not honour `MPI_Wait` on
either, since the handle alone would not say which operation to wait for.
Sharing therefore implies the handle carries no per-operation state, which
implies both operations are already complete — and a block freed then is a block
nobody is reading.

**The standard does not say this.** A search of MPI-5.0 for any statement about
handle uniqueness across operations finds none; what it constrains is the
*behaviour* of `MPI_Wait`/`MPI_Test` on a request, and the inference is drawn
from that. Two things follow, and they should be kept apart:

- **This is load-bearing for memory safety, not only for conformance.** The
  release path frees by handle value, so if the inference were false a wait on
  one operation could free another's in-flight block. The attach-side refusal
  below protects nothing here; it is the inference that does.
- **The persistent half does not need the inference.** A persistent request must
  carry its own arguments to be startable, so it cannot be a shared singleton,
  and release fires for it only when the implementation nulls the handle — which
  is at `MPI_Request_free`, by construction the point its block may go. Measured
  rather than only argued: both `_init` forms hand back distinct requests on
  both implementations even in the zero-work shapes that share everywhere else,
  and both implementations allocate for the persistent case by name.

Reasoning about what an implementation *cannot* do is the class of claim this
project has been wrong about repeatedly (`HISTORY.md` §2), so §13.3 keeps this
open rather than closed and names the two redesigns that would retire it. Note
which half each of §13.2's and §13.3's ideas addresses, because they differ:
merely *permitting* a duplicate key does nothing here, since the release that
frees the wrong block comes from an operation that never attached — but
**declining to attach an operation that is already complete narrows this a
long way**, because a shared built-in is only ever handed out for such an
operation, so under that rule a shared key is never in the table for an
unrelated completion to find. §13.2 has both.

Hence the two rules the table follows:

- It **declines a second block for a key it already holds** rather than
  overwriting — overwriting loses the first block, and freeing it would be
  indefensible even though the argument above says nothing is reading it. The
  table only declines, and says which of its two fates it is: a duplicate key
  is the caller's bad luck and must not fail the call, while a full table is
  this design's usual capacity limit and does. §13.2's (a), (b) and (c) are the
  policy above it, and (a) and (b) are why the duplicate is not reached at all
  on anything measured.
- **Every completion entry point releases**, not just the ones an author happens
  to think of. That is by construction rather than by list: the generator emits
  the release wherever an entry point has an *inout request* parameter, which is
  eleven of them and nothing else — `MPI_Start` and `MPI_Startall` are in the
  set too and never fire it, because they null nothing.

Two details are easy to get subtly wrong: released entries leave a **tombstone**
rather than an empty slot, or they would truncate the probe chain of some other
key; and a release **claims the entry before clearing it** (key → LOCKED, clear
block, then publish TOMBSTONE), or a concurrent attach could take the entry
between the releaser reading the key and clearing the block, and the releaser
would free the new owner's block.

None of this touches the callback pools, which are keyed on a slot index we hand
out ourselves, or the predefined-handle maps, which are built once from values
that do not change.

---

## 7. Decisions

Reopen one only with a new argument or a new measurement, and if you do, update
the decision rather than working around it.

1. **Conversions live in `mpiwrapper`, behind an ABI-typed vtable.** §2.
2. **Status: blob only** — no validity marker, no synthesis fallback. §5.2.
3. **The ABI surface is complete MPI-5.0**; functions the implementation lacks
   are reported at run time, never omitted. The implementation is *expected* to
   provide the MPI-4.0 API, which is what makes the mapping 1:1 — an
   expectation, warned about at configure time and not enforced, since no
   released Open MPI meets it. The enforced floor is **MPI-3.0**, verified with
   MPICH 3.1.4. §1, §9.

   **Failing the MPI-4.0 expectation costs a ceiling, not a surface.** It used
   to cost the whole large-count half: 159 entry points answering
   `MPI_ERR_UNSUPPORTED_OPERATION`. The wrapper now narrows a `_c` call onto its
   small twin and refuses only the values that will not fit (§5.10). What the
   expectation buys is the absence of a narrowing check and of a staged count
   array — a cost question — rather than whether the entry point works at all.
4. **`mpiwrapper` exports exactly one symbol**, a getter carrying
   `MPI_ABI_VERSION`, `MPI_ABI_SUBVERSION`, a generated layout hash and the
   vtable's `sizeof`. **All four are checked for exact equality**; there is no
   prefix serving and no forward compatibility, and the size is what catches a
   layout mismatch the text-derived hash cannot. §2.
5. **`mpi_abi` finds the wrapper from an environment variable**, falling back to
   the absolute build-time path — never a bare filename. §2. **Both libraries
   are built together into one prefix per MPI installation**, and the split is
   not user-visible; wrapper libraries are *not* name-tagged by MPI. **That
   prefix is exclusive** — no second wrapper, no other MPI, and never the
   wrapped MPI's own prefix, since we install `mpi.h`, `mpicc` and `libmpi_abi`
   under names it already uses. §9.
6. **Functions the implementation lacks return `MPI_ERR_UNSUPPORTED_OPERATION`**
   from generated `#ifdef` stubs, and the generator reports them. What the
   `#ifdef` tests is `MPIWRAPPER_HAVE_<name>`, written at configure time by
   `dev/probe_impl.py` from the implementation's own header — not a version
   test, not `nm`, and not `#ifdef` on the implementation's own name for a
   constant. §3, `HISTORY.md` §1.19.

   **The stub is the last arm, not the only alternative.** A large-count entry
   point whose `_c` name is absent but whose small twin is present gets a
   narrowing body instead, and reaches the stub only when neither name exists
   (§5.10). So the guard chain is `#ifdef HAVE_<name>_c` → `#elif defined
   HAVE_<name>` → `#else`, and the stub's meaning tightens from "the
   implementation lacks this" to "the implementation lacks this *and* cannot be
   asked for it another way". This is a refinement of the decision and not a
   reversal: nothing is omitted from the ABI, everything is still discovered at
   run time, and a value the small twin cannot carry is still refused — with
   `MPI_ERR_VALUE_TOO_LARGE`, which §5.10 explains is deliberately a different
   class from the stub's.

   **A stub must leave every out parameter defined, not just the handles.** This
   was half-done for a long time: the stub nulled out-handles — `null_out_handles`,
   whose docstring already gave the reason as "so that nobody is handed an
   uninitialized one" — and left plain integers untouched. A caller that ignores
   the return code, which the standard lets it do and real code does, then reads
   its own uninitialized stack. `mpi_t/mpit_vars` is the case that found it:

   ```c
   int num_sources;
   MPI_T_source_get_num(&num_sources);      /* no released Open MPI has this */
   for (int i = 0; i < num_sources; i++)    /* loop bound is stack garbage */
   ```

   The consequence is not a wrong answer but undefined behaviour whose *symptom*
   depends on what the stack held, which is why that test's failure is intermittent on
   both architectures and was mis-diagnosed three times from too few runs. **A report the caller can ignore into undefined behaviour is not a
   report**, so `stub_out_zeros` now writes a defined zero to every `out` scalar,
   under the same nullable rule the handles use. `inout` integers are deliberately
   left alone: `MPI_T_category_get_info`'s `name_len` is the caller's buffer size
   on the way in, and zeroing it would destroy an input rather than define an
   output.

   **The same promise binds every early return in a generated body**, and that
   half was missing too — the amendment above reached `emit_stub` and stopped
   there. `dev/check_out_params.py` measured **71 such returns across 26 entry
   points**, of the 415 generated arms (out of 705) that own an out handle or
   out scalar at all. Three shapes were open:

   - the array-length checks — `if (count < 0) return MPIABI_ERR_COUNT;` and
     the `SIZE_MAX` overflow guard — emitted neither half of the pair;
   - `emit_extent_queries` called `null_out_handles` without `stub_out_zeros`,
     so a failed extent probe and a rejected array extent defined the out
     *handles* and left the out *scalars*;
   - the staging-allocation failure `goto done` emitted neither.

   `MPI_Graph_map` is the plainest case, having the one bare `int *newrank`
   among the graph routines — both `if (nedges < 0) return MPIABI_ERR_ARG;` and
   `if (!edges) goto done;` returned without writing `*abi_newrank`. It was far
   from the only one: `MPI_Testany`'s `*abi_indx` and `*abi_flag`,
   `MPI_Waitsome`'s `*abi_outcount` and `MPI_Type_create_darray`'s
   `*abi_newtype` are all in the set.

   **The narrowing fallback reached the same conclusion independently, at three
   of its own four new sites** — its `narrow_in` and `narrow_inout` checks and
   its staged narrowing loop each spell the pair out, with a comment giving the
   same reason. That is the argument for one owner rather than a habit: the
   rule was rediscovered rather than reused, in code written after the
   amendment that states it. `out_defined` is now that owner — it *is* the
   `null_out_handles` + `stub_out_zeros` pair — and `guarded_lines` is the
   single emitter for a guard, which is what lets a length check and a
   narrowing check share a shape they were building by hand in three places.
   Refactoring those three changed no generated byte, which is the check that
   the shared emitter reproduces what it replaced.

   **The `goto done` family is the one that cannot define its outs at the
   return**, because `done:` is also where the success path leaves and
   `abi_ierror` does not discriminate: `mpiwrapper_errorcode_toabi` and the
   `mpiwrapper_take_handle_error` check can each produce `MPIABI_ERR_INTERN` on
   a path that did write the outs. So the pair is emitted once at body level,
   before the first jump to the label, and the success path overwrites it.

   `assemble_outliving`'s four exits — two malloc failures, its narrowing loop
   and its post-call error — are **latent, not live**: the generator already
   refuses an outliving routine that produces any handle but the request, and
   all eight produce no out scalar either, so `null_out_handles` and
   `out_defined` emit the same text there today. They were unified for
   uniformity, and one of them was the fallback's one miss — a miss that costs
   nothing until some future outliving routine has an out scalar, which is
   exactly the kind of debt a shared helper retires.

   `out-params-defined` is the test. Its first two revisions were **worse than
   no test**, and in a way worth recording: the audit keyed on
   `#define BODY_(\w+)\(TARGET\)` and on "the second definition of a name is
   the stub". The fallback made the macro `BODY_X(TARGET, FALLBACK)` and gave
   148 entry points a *third* arm, so the regex stopped matching those macros
   and the arm rule discarded the new body — and the audit reported clean over
   22 open returns it had never read. It now splits arms on the preprocessor
   directive that introduces each one, which is what the generator itself keys
   the stub on, and reports which arm a finding is in.

   Out *arrays* remain outside the promise, for the reason
   `STUB_ZEROABLE_POINTEES` already excludes them: their extent is the caller's
   contract, not ours. `MPI_Group_translate_ranks`'s `ranks2[]` and
   `MPI_Waitall`'s `array_of_statuses` are still undefined after a rejected
   length. That is a §13.2 limitation, not a decision 6 defect — the caller who
   ignores the return code learns nothing about how much of an array was
   written either way.
7. **PMPI gets its own vtable slots** — two per entry point, 1366 in all,
   calling the implementation's shifted names directly. No probe and no
   fallback, since both names always exist and reach the same code when nothing
   is interposed; which of the two is the strong definition varies by
   implementation and platform and does not matter to us. The wrapper's internal
   MPI calls use `PMPI_*` unconditionally. §2.
8. **Bootstrap by constructor into a plain pointer** — no atomic, no lazy-init
   branch, no NULL check outside debug builds. The wrapper is loaded
   `RTLD_LOCAL` and *isolated* — `RTLD_DEEPBIND` on Linux (`dlmopen` selectable
   but unusable with any MPI that `dlopen`s components), the two-level namespace
   on macOS — never `RTLD_GLOBAL`, and `RTLD_LAZY` by default. The wrapper then
   proves at load that its `MPI_*` calls resolved outward, by address *and*
   behaviourally. §2.
9. **Naming: `MPIABI_` uniformly** for the renamed view. §2.
10. **Staged temporaries outliving a call** go in a request-keyed hash behind a
    global atomic count. §6.3.
11. **All shared tables fixed-capacity and lock-free.** Op and errhandler slots
    are process-lifetime; only generalized-request and `MPI_T`-event state is
    reclaimed. §6.2, §6.3.
12. **One generated file per artifact**, seven in all. §3.
13. **No in-place argument conversion**, except same-size OUT arrays needing
    only value mapping. §5.7.
14. **`MPI_MAX_*` mismatches are handled at run time, not asserted.** §5.8.
15. **Static-assert only where a runtime check would cost something hot.** §5.9.
16. **Shared libraries only in v1.** §9.
17. **Prototype before writing the generator**, and the generator must reproduce
    it. §11.
18. **The predefined-handle reverse map is a perfect hash**, built at
    initialization, failing loudly there rather than degrading to a probe loop.
    §5.1.
19. **Ship `mpicc`, CMake package files (including a `FindMPI` shim) and
    pkg-config**, each exercised by CI rather than merely parsed. §9.
20. **The five entry points MPI-3.0 deleted are answered by `libmpi_abi`
    itself**, in terms of their replacements, with no slot and no wrapper body.
    §3.
21. **`libmpi_abi`'s soname is `MPI_ABI_VERSION`, not the project's version**,
    and `libmpiwrapper` has none. The soname is the name a client binary
    records, so it is the thing that decides whether a binary built here starts
    against *another* implementation's `libmpi_abi` — which is the entire
    promise of a standard ABI, and is therefore not ours to pick freely. Open
    MPI's ABI branch installs `libmpi_abi.so.1`/`libmpi_abi.1.dylib`, so this
    does too. It is read out of `gen/include/mpi.h` at configure time rather
    than written down again, and **must never be made to follow
    `PROJECT_VERSION`**: a 1.1.0 or a 2.0.0 of this project still implements
    ABI major 1 and must still answer to `libmpi_abi.so.1`. `libmpiwrapper` is
    reached by `dlopen` at an absolute path and MPI-5.0 §20.2.1 forbids the
    application naming it at all, so a soname on it would name nothing. §9.
22. **Neither ELF version script names its node**, so no symbol this project
    defines carries a version. A named node stamps every definition — `MPI_Send`
    becomes `MPI_Send@MPIABI_1` in a client's relocations — and another
    implementation's `libmpi_abi`, which has never heard of that node, then
    fails to satisfy it. An anonymous node filters the export set identically,
    which is the only thing §9 wanted a version script for. §9.
23. **All 26 C–Fortran converters are answered from the ABI's own values**, and
    the implementation is never asked: `MPI_X_c2f` is `MPI_X_toint`, `MPI_X_f2c`
    is `MPI_X_fromint`, and the four status forms are a 32-byte copy. §20.4
    leaving `MPI_Fint` out of the ABI is what *settles* this rather than what
    frees it — the only possible caller is a Fortran layer over the ABI, whose
    INTEGER handles hold ABI values. No availability guard and no decision-6
    stub, since no implementation entry point is involved. §4.4, `HISTORY.md`
    §2.18.
26. **`MPI_Get_library_version` prepends a banner** naming this library and its
    version, ahead of the wrapped implementation's own string, which follows
    unchanged. It is the only entry point whose answer this library *adds to*
    rather than converts or forwards, and it exists because decision 24 leaves
    nothing else that reports the shim: `MPI_Get_version` answers for the ABI,
    `MPI_Abi_get_version` for the ABI's version. §5.8.
24. **`MPI_Get_version` reports the ABI's own version**, 5.0, not the wrapped
    implementation's. MPI-5.0 §2.7 requires the `MPI_VERSION` macro and this
    call to agree, and `gen/include/mpi.h` says 5; forwarding put a second and
    contradictory answer in front of an application, which then believes its
    library is older than the header it compiled against. What actually varies
    is *which entry points answer* `MPI_ERR_UNSUPPORTED_OPERATION`, and
    decision 3 already makes that a run-time discovery rather than a version
    comparison. `MPI_Get_library_version` still forwards, so the wrapped
    implementation's own version is still available where implementations put
    it. **The body still calls the implementation**, because that call is §2's
    isolation probe. §1, §2.
25. **The Fortran getters answer from this build's Fortran compiler**, when
    there is one and when the wrapped MPI has Fortran datatypes; otherwise
    "not set", which is what §20.4.1 defines that to mean. The values cannot
    come from anywhere else — the standard passes `.TRUE.` and `.FALSE.` *by
    address* precisely because no C code and no MPI call can produce them — so
    one Fortran translation unit is compiled into `libmpiwrapper` and read at
    run time. It brings **no Fortran runtime**: `libmpiwrapper` is `dlopen`ed
    beside the application's own, and a second `libgfortran` in one process is
    its own class of failure. An explicit `MPI_Abi_set_fortran_booleans` still
    wins, because the standard makes the application's answer authoritative.
    §8, §9.

---

## 8. The hand-written set

Functions where per-function judgement is needed. The generator's
`HAND_WRITTEN` ledger names them and fails if the two sets do not together cover
all 688; `CODE.md` §6 has the current grouping and `gen/report.txt` the reason
on every line. **This section is about what the set is *for*, not how big it
is** — the count has been wrong in prose three times (`HISTORY.md` §4) and the
ledger is the authority.

A body is hand-written when it needs one of three things the generator cannot
supply:

**1. A judgement per function against the standard.** Truncate or error for each
output string (§5.8); what the ABI reports for Fortran `LOGICAL`; whether a
sentinel is legal at a given parameter; which of `_toint`/`_fromint` and
`_c2f`/`_f2c` may forward and which may not (§4.4). These have signatures a
generator could match and answers it could not choose.

**2. State the wrapper owns.** An initialization state machine, because
`MPI_Initialized` and `MPI_Finalized` are true statements about *us* rather than
forwarded questions; the trampoline pools and maps of §6; the dynamic error-code
and keyval registries of §5.6; the attached buffer's ownership record; the
intern table behind handle serialization.

**2a. State that is neither ours nor the implementation's.** The Fortran
getters of decision 25 are the only instance, and they are why this is a
separate line rather than a case of 2: what they report belongs to *the
compiler this library was built with*, which neither the wrapper nor the
wrapped MPI can be asked for at run time. The value arrives through a build
input — one Fortran translation unit, `src/mpiwrapper/fortran_probe.f90` — and
the entry point exists to hand it over. §9 has what that costs the build.

**3. A class no signature carries.** The attribute getters of §5.6, whose
returned `void *` means whatever the keyval says. This is the category worth
watching, because it is the one that has no in-house detector: the emitted text
passes every generator assertion, the call returns `MPI_SUCCESS`, and the frozen
tallies are unmoved. An outside oracle found the only two instances known.

Groups that are *not* hand-written, though an earlier draft claimed them, and
the reason each moved:

- **The eight staged-past-return `*alltoallw*` forms are generated.** Staging is
  an argument class (§5.7), and one hand-written example existing before the
  generator did is not an argument for hand-writing the other seven.
- **`MPI_Wtime` and `MPI_Wtick` are generated.** A `double` return with no error
  code is mechanical.
- **The six flush forms** of the buffer family are mechanical; only
  attach/detach need judgement.
- **`MPI_Keyval_create`** is `MPI_Comm_create_keyval` under an older name, so
  §3's substitution makes the trampoline judgement once instead of twice.

---

## 9. Building

`CODE.md` §3 and §8 have the layout, the options and the invocations. This
section is the constraints they implement.

**`cmake && make install` builds both libraries against the MPI it finds, into
one prefix.** The two-library split is an *internal* matter and must not be
user-visible: a user configures against an MPI and gets a working `mpi.h`,
`libmpi_abi`, `libmpiwrapper`, compiler wrapper and package files. Nobody should
have to know that `libmpi_abi` does not itself need MPI. That property is still
real and still valuable internally — it is what makes the cross test possible
and what lets one `libmpi_abi` be pointed at another wrapper — but it is exposed
only as a developer option.

**One prefix per MPI installation, and the prefix is exclusive.** Everything a
build produces goes into one prefix, and that prefix holds nothing else: not a
second wrapper, not another MPI, and in particular **not the MPI this build
wraps**. The reason is file names and is not a matter of taste: `mpi.h` against
`mpi.h`, `mpicc` against `mpicc`, and — because MPI-5.0 §20.2.1 requires a
library implementing the standard ABI to be named `mpi_abi` — `libmpi_abi`
against `libmpi_abi`. Installing beside the MPI does not merely risk a clash, it
*is* one, on the two files that decide which MPI a consumer compiles against. It
also breaks the build that produced it, since the no-self-wrapping check then
refuses to configure against that MPI — and the failure arrives at the *next*
configure, on someone else's machine.

**Four configure-time checks, all compile-only** so cross-compiling works:

1. `MPI_VERSION >= 3`, **hard**, and `>= 4` as a **warning**. The warning names
   the consequence — the `_c` slots become `MPI_ERR_UNSUPPORTED_OPERATION` stubs
   — and the floor sits at MPI-3.0, below which the ABI's own surface stops
   being expressible. The floor is tested rather than declared: MPICH 3.1.4
   passes all six tests at two ranks, and `ci-scripts/run-linux-docker.sh floor`
   is that row wired up rather than a recipe to rediscover. It needs an older
   toolchain to build at all (gcc 9; gcc 11 and 13 both reject its own headers),
   a newer CMake than Ubuntu 20.04 ships, an in-tree build, and a generator that
   stays inside Python 3.8 — four constraints that are about the *toolchain*
   floor rather than the standard one, and worth knowing before promising a
   distro matrix. `HISTORY.md` §3's S6 entry has what each cost.
2. **No self-wrapping.** Hard error if the found MPI prefix is *our own*
   installation, detected by the presence of `mpiwrapper_marker.h` — a
   hand-written file in `cmake/` that we install and that exists for no other
   purpose. Neither `MPI_ABI_VERSION` nor the library name discriminates, since
   a genuine ABI-implementing MPICH installs `libmpi_abi` too. The accident is
   easy to hit, because `find_package(MPI)` is looking for exactly the `mpi.h`
   and `libmpi_abi` we install, and its symptom is a startup loop rather than a
   diagnostic. `mpi.h` itself stays pure — the stub header plus
   `doc/mpi.h.patch`, with no marker macro. Wrapping a *genuine* ABI MPI is
   permitted behind an explicit flag, with a warning; see oracle 5.

   **The marker is a file of its own rather than a generated header** because
   the installed include directory holds `mpi.h` and the marker and nothing
   else. `mpiwrapper_vtable.h` was the marker until it was noticed that
   installing it drags `mpiabi.h` in with it — the vtable header includes it —
   and neither belongs in a prefix that no consumption route reads them from.
   The two are the halves' private contract; every build that needs them,
   including the per-MPI wrapper builds the README describes, is a build of
   this project from source with `gen/include` on its path.
3. The `_Static_assert` battery of §5.9.
4. Every generated constant `case` naming a real implementation macro — free,
   since it is a compile error.

**Generated code stays committed**, with a `regenerate` target outside `all` and
a CI job asserting an empty diff. Python is a dev dependency, never a build
dependency.

**Ship all three consumption routes**, since a library nobody can consume
portably has not been delivered, and CI must *use* each of them to build and run
a program rather than merely check that they parse — `--libs` is worthless if
the executable it produces cannot start. `mpicc` must **not** name
`libmpiwrapper`: MPI-5.0 §20.2.1 requires `mpi_abi` to be the sole direct MPI
dependency of the application binary, and the wrapper is reached by `dlopen`.

**Symbol visibility** is `-fvisibility=hidden` plus an explicit export macro,
*and* a version script on ELF / `-exported_symbols_list` on macOS. Both halves
are needed: the visibility preset reaches every symbol this project writes and
not the handful the linker inserts into every shared object. `libmpi_abi`
exports only `MPI_*`/`PMPI_*`; `libmpiwrapper` only `mpiwrapper_get_vtable`.

**The version scripts filter; they do not version** (decision 22). Both nodes
are anonymous. The distinction is invisible in the export set — which is all
either script was added for, and why the named nodes survived unexamined — and
decisive one level up: a named node puts `MPI_Send@MPIABI_1` in every client
binary's relocations, and a client that carries a version can only be satisfied
by a library that defines it. Since no other implementation of the standard ABI
defines `MPIABI_1`, a named node would have made every binary built here
unrunnable against exactly the libraries the ABI exists to let it run against.
An anonymous node also emits no symbol for itself, which is what let
`test/check_exports.cmake` drop its exemption list.

**The soname is the ABI's, and it is a cross-implementation agreement**
(decision 21). This is the sharpest instance of a rule the rest of this section
only implies: **a name a client binary records is not this project's to
choose.** `libmpi_abi.so`'s consumers are compiled elsewhere, possibly against
someone else's implementation, and MPI-5.0 §20.2.1 both requires the name
`mpi_abi` and permits an application to depend on "its versioned variant" —
which makes the version part of the contract rather than packaging trivia. So
`SOVERSION` is `MPI_ABI_VERSION`, matching what Open MPI's ABI branch installs,
and it is `file(STRINGS)`-ed out of the generated header so that no edit can
put the two out of step. `VERSION` is `PROJECT_VERSION` and moves freely; the
two are deliberately different numbers that happen to agree at 1.0.0 today.

**Mach-O gates on a second number, and it is not the one in the file name.**
dyld records the compatibility version a client was linked against and refuses
at load any library offering a lower one — so the leaf name being right and
this being wrong produces exactly the failure the soname exists to prevent,
on macOS only, and only when a swap is actually attempted. The ecosystem's
number comes from libtool: ABI version 1 is `-version-info 1:0:0`, which gives
the name `libmpi_abi.1.dylib` and compatibility version *2.0.0*. CMake derives
`-compatibility_version` from `SOVERSION`, which must stay 1 for the file name,
so the two are decoupled with `MACHO_COMPATIBILITY_VERSION`.

This was wrong for a day, between decision 21 and the mpif rows of §10 —
`libmpi_abi.1.dylib` with compatibility version 1.0.0, which is a library
correctly named and unloadable by anything linked against a real one. Nothing
here found it; mpif's `ci-scripts/check-mpi-install.sh` asserts both fields and
did. The lesson is §10's, not §9's: **the export-side conventions of an ABI are
only checkable against another implementation of it.**

`libmpiwrapper` gets neither, and the asymmetry is the two-library split
showing through: nothing links it, so nothing records a name for it, so there
is no name to keep stable. It is found by the absolute path decision 5 bakes
in, or by `MPI_ABI_WRAPPER_LIB`.

What none of this yet establishes is the swap itself — a binary built here
starting against a vendor `libmpi_abi`, and the reverse. That is a test this
project does not have; §10's oracle 5 wraps an ABI-implementing MPI, which is a
different question. It is the next thing to build, and it is what would have
caught both of these choices while they were still implicit.

**A Fortran compiler is an optional build input** (decision 25), and the
degradation without one is to the previous behaviour rather than to a broken
one: the two Fortran getters answer "not set", which is legal and was the only
answer before. A build meant to serve a Fortran consumer wants one present, and
§10's mpif rows are what notice.

Three separate mechanisms keep its runtime out of `libmpiwrapper`, and all
three are needed — each was added after the previous one proved insufficient,
measured with `otool -L`:

1. an **object library**, so the `.f90` is compiled apart from the wrapper;
2. `LINKER_LANGUAGE C` on the wrapper, because CMake otherwise picks `gfortran`
   to drive the link on account of that object's language;
3. `CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES` emptied, because CMake adds
   `-lgfortran -lquadmath` to any target holding a Fortran object regardless.

Emptying (3) is safe only because the probe is written to need no runtime — no
I/O, no allocatables, no derived types, and `nm -u` showing `malloc`, `free`
and `memcpy` as its only undefined symbols. **Check that with `nm -u` if
anything is ever added to it**; if it does need the runtime, the link fails
naming the symbols, which is the constraint stating itself rather than a
silent second `libgfortran`.

The probe also forced `cmake/mpiwrapper.exported_symbols` into existence: a
`bind(C)` subroutine is exported whatever `-fvisibility=hidden` says, ELF's
version script caught it for free, and macOS had no counterpart until now.

**The no-Fortran configuration broke twice before it ever ran**, which is worth
recording as a coverage fact rather than as two mistakes. Nothing gates it: the
laptop has a Fortran compiler, and so does every Docker row, because the commit
that introduced the probe added `gfortran` to `ci-scripts/linux-test.sh` in the
same breath. **CI's only coverage is the `sanitize` job, and that is an
accident** — it happens not to install one. Both failures were of the shape
where the argument is obviously right and the build was never run: first the
derived state declared inside the `#if` while the bodies reach it behind a
run-time test, then `enable_language(... OPTIONAL)` reporting a language it had
not found. A deliberate second configure in one existing row would make this a
tested configuration rather than a claimed one; until there is, the claim that
a C-only build works rests on the `sanitize` row noticing.

**Shared only in v1.** Static linking would require splitting `entrypoints.c`
into 688 translation units, because MPI-5.0 §15.2.1(2) requires that "those MPI
functions that are not replaced may still be linked into an executable image
without causing name clashes" — for an archive that means one entry point per
member (mpif's `split-wrappers.sh` is the precedent; for a shared library
ordinary interposition satisfies it). `dlopen` is central to this design anyway,
so a static `libmpi_abi` is an odd configuration. The split would be a build
step rather than a generator output, so decision 12 survives if this is
revisited.

**Provisioning MPI in CI** is pinned released tarballs, built from source and
cached: stock configure, no pruning, no header substitution. Do not add
`--disable-fortran` to save build time — it silently drops the *implementations*
of `MPI_Type_create_f90_{real,complex,integer}`, plain C entry points MPI-5.0
requires, so the compile-only probe reports them available and only the
wrapper's link step fails.

Version choice is about coverage, not admissibility:

| | why this version |
|---|---|
| MPICH >= 4.0 | provides the `_c` surface, so the large-count half of the ABI is exercised at all. Was "the only implementation that does", and is not any more — see the row below |
| Open MPI >= 5.0 | sessions, and the current component architecture; 4.1 is *wrappable* and is a legitimate extra row rather than an excluded one |
| MPICH 3.1.4 | the MPI-3.0 floor itself, verified rather than declared |
| MVAPICH 4.1, Intel MPI 2021.15 | **the `_c` surface is not MPICH's alone, and `MPI_VERSION` does not predict it.** Measured, not read off a page: MVAPICH 4.1 declares MPI 4.1 and carries 387 `_c` prototypes in `src/include/mpi_proto.h`; Intel MPI 2021.15 declares MPI **3.1** and still declares 125 of them, `MPI_Type_size_c` among them. **That partial surface is the row's second reason to exist:** with 125 of the ABI's `_c` forms present and the rest absent, it is the only row where §5.10's narrowing fallback is compiled for some entry points and not others — MPICH >= 4.0 and MVAPICH 4.1 have the whole surface and Open MPI 5.0.10 none of it, so both take one arm throughout. Open MPI 5.0.10 stays the row that makes the fallback, and §13.2's `INT_MAX` cap with it, load-bearing rather than exotic (`dev/third-implementations/`) |

---

## 10. Testing

### Five oracles

1. **The ABI header, by compilation — no MPI, no launcher, seconds.** Wrong
   signatures are build errors; `nm` on `libmpi_abi` against the 1376-symbol
   list extracted from the header, **in both directions**; and nothing else
   exported. A total completeness check as the cheapest job in CI.
2. **The implementation's header, by compilation**, plus `nm` asserting
   `libmpiwrapper` exports exactly one symbol.
3. **The generator's assertions on its own output** — every handle and sentinel
   translated exactly once, no ABI-typed parameter in an implementation call
   argument list, frozen tallies, unknown kind a hard stop — plus the empty-diff
   regeneration and the reproduction of `dev/s1-reference/`.
4. **MPI-5.0 Appendix A.3 ("C Bindings") via `pdftotext -layout`** on
   `doc/mpi50-report.pdf`, an independent route from the same LaTeX that
   produced `apis.json`. (A.2 in this report is the op.-related semantics
   summary, not the bindings; earlier drafts cited the wrong one.) A C signature
   carries no `INTENT`/`::` list to check completeness against, so the parse
   validates itself differently: A.3's open- and close-paren counts must equal
   the number of signatures found, one pair each, with nothing left over. Array
   parameters are folded to their pointer-decay form before comparison.
   Exemptions are named, explained, and fail the run when they stop firing.

   **Eight named exemptions**, all about *names* rather than semantics, since C
   bindings have no `INTENT` to disagree over: fourteen predefined callback
   constants (`MPI_COMM_NULL_COPY_FN` and the rest) that A.3 documents with
   prototype syntax but that are values, not entry points;
   `MPI_Wtime`/`MPI_Wtick`/`MPI_Aint_add`/`MPI_Aint_diff`, which the standard's
   body binds but A.3 itself never lists; `MPI_Status_f082f`/`_f2f08`, real A.3
   bindings this ABI omits by design (they compose from the four converters it
   does carry); `index`/`indx`, the libc-collision dodge every implementation's
   headers use, in seven functions; MPI_T's `pe_session`/`session`;
   `MPI_Status_get/set_error`'s `err`/`error`; `MPI_Precv_init`'s `dest`, held
   over from `MPI_Psend_init`'s template in the vendored stub header rather than
   corrected to `source`; and `MPI_F08_Status`'s capital S, which owes A.3's
   lowercase `mpi_f08_status` nothing since §20.4 states that `MPI_F08_Status`
   is not part of the C ABI at all — the name is this project's own coinage.
5. **The identity configuration: wrap an MPI that already implements the ABI.**
   Every conversion becomes an identity — predefined handle values, error codes,
   sentinels, `MPI_MAX_*`, ranks and tags all match — so the conversion tables
   are neutralized and any difference from native pass-through is a bug in the
   *plumbing*: vtable handshake, bootstrap, staged temporaries, trampoline
   pools, lifetime rules. This isolates the half of the system that is otherwise
   hardest to attribute, since a wrong reduction result does not say whether the
   datatype map or the temporary was at fault. It also lands the status
   assertion exactly on its boundary: an ABI implementation's status is 32 bytes
   with the named fields at 0/4/8, so its private part is 20 bytes and fits with
   nothing to spare. It does *not* test the conversion tables, where most of the
   risk lives.

   **It runs on macOS too, provided the implementation is a two-level build.**
   An earlier version of this section called it a Linux-only oracle, blaming
   dyld's weak-definition coalescing; `HISTORY.md` §2.19 overturned that — the
   one refused configuration was a `-flat_namespace` implementation build
   (mpif's gcc/libtool one), a two-level implementation is never captured
   however weak its symbols, and a flat one still gets the refusal, which is
   correct and stays worth keeping as a test. Measured against Open MPI main
   `--enable-standard-abi` (`build/mpi/ompi-main-prefix`): the wrapper loads,
   passes both isolation checks, and the arrays, large-count and selftest
   suites run clean at one rank. The failures that remain are the
   implementation's, and knowing that is itself oracle-5 information: every
   one in the prototype and converter suites (2 and 14 respectively) is a
   c2f/f2c round trip — decision 6 stubs, because the build ships no Fortran
   interface — and `abi_tools_test` segfaults inside the implementation's own
   `PMPI_T_pvar_reset`.

   **Building against Open MPI's own ABI mode** needs one accommodation for
   that missing Fortran interface: `mpicc_abi`'s `mpi.h` does not declare
   `MPI_Fint`, so `internal.h`'s `sizeof(MPI_Fint) == sizeof(MPIABI_Fint)`
   assertion is guarded on `MPIWRAPPER_HAVE_MPI_Comm_c2f` — an implementation
   that declares any converter must declare `MPI_Fint` to type it, so the
   guard skips the assertion exactly where there is nothing to name. Making
   the oracle a CI row is what remains.

   **mpif is now a CI row rather than an aspiration**, four of them:
   `{mpich, openmpi} × {native, wrapper}`, where the native legs run mpif over
   an implementation's own standard ABI and the wrapper legs run the same mpif
   over this project wrapping *the same commit* of that implementation, built
   stock. The comparison is the product: a wrapper-leg failure its native
   counterpart lacks is a defect here, one they share is not. Neither
   statement can be had from a single leg.

   It earned its cost immediately. mpif's first run answered 69 of 81 where
   the native ABI answered 81, and the twelve were three separate defects that
   all thirteen of this project's own tests had passed over — `HISTORY.md`
   §2.18's converter bug, and decisions 24 and 25. Two more came from mpif's
   `check-mpi-install.sh` alone, before any test ran: the Mach-O
   compatibility version (§9) and the export style (§13.2).

   `ci-scripts/test-mpif.sh` is the runner and takes **two** prefixes, because
   this project's has no `mpiexec` — the launcher belongs to the wrapped MPI.
   Only mpif's `ctest` runs; its MPICH Fortran suite is a separate thing and is
   out of scope.

### Behavioural tests, in increasing cost

`CODE.md` §10 lists what each one covers. The design points:

- **`mpiwrapper_selftest`** is white box: it compiles the conversion runtime
  into itself so it can walk the maps in both directions rather than inferring
  them from MPI results. It is the only place the capacity behaviour of tables
  with **no error channel** can be checked — `MPI_Comm_toint` returns an `int`,
  so a full intern table must answer 0 and no `_fromint` may accept it — and it
  carries the **dynamic-handle collision probe**, which is specifically the
  runtime replacement for the configure-time test cross-compiling forbids.
- **Each behavioural test is written against the shape a plausible-but-wrong
  body gets wrong**, not against the happy path. That is the whole discipline:
  a persistent `MPI_Alltoallw` started three times rather than once; every
  `MPI_T` query called twice, the second time with null OUT pointers; `_toint`
  of a predefined handle compared against the header's own constant rather than
  round-tripped; a status taken through Fortran and back and then asked
  `MPI_Get_count`; an errhandler trampoline asked whether it received *the
  handle the handler was set on*.
- **The large-count half needs an oracle that does not know which arm it is
  testing**, and `abi_large_count_test` is it: the `_c` form and its small twin
  must agree, which is checkable over MPICH (where the implementation answers)
  and over Open MPI (where the fallback does) with the same assertions. Without
  that property the fallback would be exercised only where nothing can verify
  it. §5.10 has the rule; its sharpest cases are the ones a wrong body gets
  wrong rather than the ones it gets right -- a vector collective at a non-root
  rank passing a genuine `NULL`, a nonblocking one whose caller overwrites its
  own count arrays the instant it is posted, and a refused call's out handle.
- **MPICH's C test suite** is the first oracle nothing in this repository wrote,
  and it earned that position: three conversion bugs no in-house check could
  have seen (`HISTORY.md` §3, S7). Its gate reads the expected-failure list in
  both directions and rejects a line with no reason.
  **How a test fails decides which list it belongs in, and there are four.** An
  expected failure runs and fails the same way; a flaky one runs and does either;
  a **hang** runs, fails, and costs `runtests`' 180-second default every time it
  does; and only a test that takes the runner down before reporting is excluded,
  because that is the one case with no TAP line to gate on. The third category is
  the one the taxonomy was missing: run 32586710591 spent 39.1 of the Open MPI
  `p2p` shard's 46.3 minutes and 12.0 of the `rest` shard's 13.0 on seventeen
  hangs that were *already* expected failures — 83% of the workflow's critical
  path re-establishing what the lists said. `timelimit-ci-openmpi.txt` caps those
  lines at 30 s with the suite's own `timeLimit=` key, so they keep running,
  keep reporting and keep gating — run 32604435562 is the same workflow at ~20
  minutes instead of 48m47s, reporting the identical 61 and 84 failures. **Cap a hang, do not exclude it:** an excluded
  test cannot tell you the day the implementation fixes it, which is the only
  reason to keep an expectation rather than a note.
- **The cross test, the headline property:** one `libmpi_abi`, one test binary,
  run against an MPICH wrapper and an Open MPI wrapper by changing only the
  environment variable. mpif's `cross` stage rebuilds against each
  implementation; ours rebuilds nothing, which is a strictly stronger claim.
- **Sanitizers and valgrind.** The suppression file is part of the design
  record: op slots, errhandler slots, keyval pairs and datarep state are leaks
  *by design*, so an unexplained entry means the design changed.

### Matrix

MPICH >= 4.0 and Open MPI >= 5.0 as the primary rows, plus MPICH 3.1.4 as the
MPI-3.0 floor and Open MPI 4.1 where a distro provides it; gcc and clang; Linux
and macOS required, FreeBSD via a VM on a Linux runner (mpif's precedent),
Windows/mingw later.

**32-bit is load-bearing, not routine coverage.** ABI handles are pointer-sized,
so i386/arm32v7 is the only place the "no spare high bits for tagging"
constraint of §4.1 is visible.

**MVAPICH is worth a row, and it now has two.** It is MPICH-derived, so its
handle values, error classes and status layout are MPICH's and the conversion
tables are already exercised; what it adds is a third *installation* shape — its
own library naming, its own `mpicc`, its own launcher — which is where "all
three consumption routes must build and run a program" gets tested against
something nobody tuned it for. The same argument extends to Intel MPI, which is
the `linux-oneapi` job, and to Cray MPICH, which cannot be a public CI row at
all because it ships with an HPE system.

**"And a cheap one" was wrong, and the way it was wrong is the useful part.**
The sentence assumed that MPICH-derived means MPICH's build. It does not:
MVAPICH's bundled libfabric carries two providers MPICH's does not,
`prov/mverbs` and `prov/ucr`, and both include `<infiniband/ib.h>`
unconditionally, so a stock `configure && make` dies at
`prov/ucr/src/ucr_domain.lo` on any machine without rdma-core headers — which is
every CI runner. `libibverbs-dev` and `librdmacm-dev` are enough; no hardware, no
kernel module, and **no `--with-device` and no configure change**, so the
stock-configure rule this section sets for the other two installers survives
intact. That distinction is the finding: the fabric question that looked like
this row's main risk is answered by adding two packages, not by taking a
position on the device.

**Both new rows show a symbol binding neither old one does**, and they agree with
each other: MVAPICH 4.1's `libmpi.so.0` has 672 *weak* `MPI_*` over 672 *strong*
`PMPI_*`, and Intel MPI 2021.15's `libmpi.so.12` has 613 over 613 — where the
distro MPICH and Open MPI rows define both strongly at one address, and macOS
MPICH keeps `PMPI_*` in a separate library. `MPI_Send` and `PMPI_Send` still
resolve to one address in every case, which is the whole of what §2 requires;
what the third pattern establishes is that the requirement is worth stating as
"both names exist and reach the same code" rather than as anything about how
they are bound. That the two MPICH-derived newcomers agree, while Ubuntu's
MPICH does not, says the pattern follows the *build* rather than the family —
which is why `linux-test.sh` keeps printing it per row instead of recording it
once here.

**Intel MPI is the cheaper row, and by a wide margin.** It is a binary
distribution from an apt repository the `compile` job already trusts for icx, so
there is no installer to write and nothing to cache: one package, source
`vars.sh`, done.

**But which Intel MPI is the whole question, and the newest is the wrong
answer.** Wrapping an MPI that already implements the standard ABI is not what
this project is for — a consumer of such a release links the vendor's ABI
library and needs nothing here — so the row is only worth having against a
release that lacks one. Bisecting the apt repository (`dev/third-implementations/`)
puts that line at **2021.17, the first release shipping `libmpi_abi.so`**;
Intel documents it as an MPI-5.0 standard-ABI technical preview, C only, reached
through `mpicc -mpi-abi`, so the default `mpicc` on those releases is still an
ordinary wrap target — but redundantly so.

So `linux-oneapi` pins **2021.15**, the newest release below that line for which
the wrapper builds, and the step refuses if a bump ever brings an ABI library
back. Not 2021.16, and that gap is §13.4's: that one release declares
`MPI_T_event_dropped_cb_function` with `int count` where everything else says
`MPI_Count count`, and the wrapper does not compile against it. **This is the
first place the availability probe's question was the wrong question** — it asks
whether an entry point is declared, not with what signature — and it is worth
noting that a *pinned old* row found it where a floating-newest row never would
have.

**Neither of the two gets an MPICH-C-suite leg, and that is deliberate rather
than pending.** A suite leg gates against a measured expected-failure list, and
a leg without one either gates on nothing or reports forever — the arrangement
`suite/README.md` records getting wrong before the per-implementation and
per-architecture split. Both rows run this project's own thirteen tests and
`check-install.sh`'s six legs, which is what "a third and fourth installation
shape" needs; extending the suite to them means running it once to write
`xfail-ci-mvapich.txt` and `xfail-ci-intelmpi.txt` first, and that is its own
piece of work rather than a line in a matrix.

### Gating

Our own tests, the MPICH C suite, the cross test, and the sanitizer/valgrind
runs.

**Consumer integration is the oracle that matters most and the one no in-house
test replaces.** Each of the four covers something the others do not: HDF5's
parallel driver for `MPI_File_*`, the bitmask `amode` and `datarep`; PETSc for
datatype, op and attribute breadth; mpi4py for the loader scenarios
`dev/dlopen-probe` models in miniature; mpif for the Fortran converters and
status `f2c`/`c2f`. **mpif's own `test/` is deliberately not gating**, so the two
projects' CI do not become coupled; it should be run before releases.

---

## 11. Sequencing, and the work that remains

**Prototype before generator.** Hand-write a representative set of entry points
end to end, all crossing the vtable boundary, and get them passing against one
MPI. Only then write the generator, and require it to reproduce them. Designing
the generator before the shape of its output is known is the main way this goes
wrong. That principle has been discharged (`HISTORY.md` §3, S1–S2); its residue
is `dev/s1-reference/` and the `prototype-reproduced` test, which keep it from
being a claim made once.

Eight stages have run: S0 through S7, in the order S0 → S1 → S2 → S3 → S7, with
S5 and S6 floating and S4 overlapping S3. Two remain.

### S8 — Consumer integration

HDF5, PETSc, mpi4py, mpif — each covering something the others do not (§10).

**Exit check.** Each project builds against a wrapper and passes its own suite;
every failure is triaged into either a bug of ours or a documented gap.

**Its first task is smaller than the stage and should be done first:** finish
the Open MPI triage of S7's expected-failure list, about half of which is
honest placeholders. The method is the one S7 used throughout — build the same
test with the implementation's own `mpicc` and see whether it passes without the
wrapper. `run-suite.sh --gate-only` re-runs the comparison against a TAP file
already in hand, so this does not need a fresh suite run.

**Model: Opus.** This is where omissions that all five oracles pass will
surface, and the work is diagnosis rather than construction. mpif is the only
oracle for the 22 `_c2f`/`_f2c` forms, whose in-house test can only round-trip
through our own code in both directions.

### S9 — Sanitizers, threads, 32-bit

ASan/UBSan with a suppression file documenting the by-design leaks (op slots,
errhandler slots, keyval pairs, datarep state); an `MPI_THREAD_MULTIPLE` stress
test over the pools and maps; the 32-bit row.

**`dlmopen` is not the fallback it was planned to be** (§2), and **the first of
the three options below has now been measured away.** `RTLD_DEEPBIND` really
does disturb the sanitizers: ASan's runtime refuses the `dlopen` rather than
degrading, so a wrapper cannot be loaded at all under it (§12). That leaves:
build the MPI under test with its components static, so nothing is `dlopen`ed at
run time — which `HISTORY.md` §1.5's MPICH 3.1.4 result says would also make
`dlmopen` work; or accept that the sanitizer jobs cover `libmpi_abi` and the
conversion layer rather than the loaded configuration.

**The third is what CI does today**, and it is less thin than it sounds:
`mpiwrapper_selftest` compiles the conversion runtime into itself precisely so
that it can be exercised without a `dlopen`, so the handle maps, the constant
maps, the status blob and the staging policy are all instrumented. What is not
covered is the loaded configuration — the vtable handshake, the bootstrap, and
the lifetime rules across the boundary — which is exactly the half oracle 5 was
introduced for. Making the second option work is the way to get it back, and it
is a change to how CI provisions its MPI rather than to anything here.

**Exit check.** Sanitizer jobs green with every suppression entry explained; the
thread stress test passes repeatedly; the 32-bit variant passes the same gates.

**Model: Opus.** Sanitizer output over an MPI is mostly triage, and an
unexplained suppression is a design change nobody noticed.

### Choosing a model, and the actual principle

**How cheap a model can safely do a piece of work is set by how strong its exit
check is.** A count or a signature comparison is an exact oracle, and a wrong
answer cannot survive it. The dangerous stages are the ones where a
plausible-but-wrong conversion compiles, links, passes the in-house tests, and
produces a wrong answer at 4096 ranks. That asymmetry, not raw difficulty, is
what decides.

Two things the completed stages taught about applying it:

- **The fencing works where it can see the property.** The ledger accounts for
  all 688, the frozen tallies fail on any reclassification, the "no ABI-typed
  parameter reaches the call" assertion runs over the emitted text, and
  `prototype-reproduced` catches a regression in any shape S1 tested. Mechanical
  work behind those fences is cheap-model work.
- **It cannot see a lifetime, and it cannot see a class no signature carries.**
  A body that releases a staged temporary at return instead of at completion
  passes every check listed above and corrupts memory under load. An attribute
  value converted with the wrong family returns `MPI_SUCCESS`. Those want the
  strongest model available, and an outside oracle besides.

The split held exactly where it was tried: a harness is mechanical, and deciding
whether a failure is our bug, the test's assumption, or the implementation's is
not.

### Session hygiene

Moved to `CLAUDE.md`, which every session loads. The rules are working style
rather than design, and they could not do their job from deep inside the file
they exist to keep out of context.

---

## 12. Risks worth measuring

- Whether a dynamically created implementation handle can be bit-cast into an
  ABI handle without colliding with `0x20`..`0x2eb`. Probably yes for both
  implementations (§5.1), but the probe has to be a runtime test, not a
  configure-time one — and it exists, in `mpiwrapper_selftest`.
- ~~Whether `RTLD_DEEPBIND` survives ASan.~~ **Measured, and it does not.** The
  sanitizer runtime refuses the load outright — "you are trying to dlopen ...
  with RTLD_DEEPBIND flag which is incompatible with sanitizer runtime"
  (google/sanitizers#611) — so the five tests that load a wrapper cannot run
  under ASan on any MPI, and no flag of ours changes it. The measurement is the
  `sanitize` job of `.github/workflows/ci.yaml`, which took one run to answer a
  question that had been open since S9 was written. What it costs is in §11's
  S9; the short version is that the third of its three options is now the only
  one, since `dlmopen` was already unavailable.
- musl: no `dlmopen`, and `RTLD_DEEPBIND` is accepted but ignored. Neither
  mechanism is available, so the probe needs a musl row before anyone claims
  Alpine support.
- ASan/valgrind noise from the implementations, which determines how useful
  those runs are. Expect the first ASan run over Open MPI to be mostly triage of
  noise that has nothing to do with this code.
- Whether an interposed profiling tool between the wrapper and the
  implementation behaves as decision 7 assumes. The two-slot design exists for
  it and nothing has tested it; Score-P or mpiP linked into the wrapper's
  dependency chain would.

Settled, and kept as pointers because each was a real open question:
`RTLD_DEEPBIND`'s transitivity (yes, `dev/dlopen-probe/`); whether the request
map's key is unique (no, `dev/request-identity/`); whether the MPICH C suite
compiles against the ABI header at all and what that costs (it does; eleven
build failures, all of them entry points MPI-3.0 deleted, plus QMPI).

---

## 13. What is missing, broken, or undecided

### 13.1 Known defects

None outstanding in the documents or the generated artifacts. The class is kept
as a heading because it keeps recurring and is worth a place to put the next
one: **a statement an artifact contradicts** — a comment that miscounts the
struct beneath it, a tally attributed to the wrong stage, a count copied from
another sentence rather than from the thing it describes. Each is one edit and
none is found by any test, which is the whole problem with them.

One is deliberately *not* closed by an edit, because an edit cannot close it:

- **The MPICH-suite per-run test totals are unknown.** Four documents once gave
  four numbers (1212, 1229, 1230, 1231) because the totals moved twice as the
  lists were retriaged and the copies did not all follow. They have been removed
  rather than guessed at; the failure counts that remain are `wc -l` on the
  lists and are reliable. A total comes back only from a run, and
  `run-suite.sh` prints the three configure decisions that change it.

### 13.2 Limitations that are real

Each is a behaviour a user can hit, and all seven should be in the release
notes. Six are deliberate — the design chose them, and §6.2 says why for the
first two. The seventh, the `libmpi_abi.so` name collision at the end of this
section, is not a choice anyone made: it is a consequence of picking the obvious
library name, and it was found by adding a CI row rather than by thinking about
it. The staged-leak one used to be a conformance bug rather than a
choice: a legal call answered with `MPI_ERR_INTERN`. That is fixed, and what is
left in its place is a bounded leak, which is a limitation and not a bug; the
wrong reading that hid it is kept in `HISTORY.md` §2.6a.

The `INT_MAX` cap and the envelope-constructor entry apply **only over an
implementation that lacks the `_c` entry points** — every released Open MPI, and
MPICH before 4.0, but not MVAPICH 4.1 or Intel MPI 2021.15, which have them
(§9's version table). Named rather than counted because this list has grown a
seventh entry at the end since the sentence was written. Over MPICH ≥ 4.0 or
Open MPI `main` neither exists, because the narrowing fallback they come from is
not compiled at all (§5.10).

- **A large count is capped at `INT_MAX` where the implementation has no `_c`
  form.** The fallback narrows the count onto the small twin and returns
  `MPI_ERR_VALUE_TOO_LARGE` for a value that will not fit, so a program that
  genuinely sends more than 2^31−1 elements in one call gets an error where a
  native MPI-4 implementation would succeed. Raising the ceiling means
  describing the payload with a temporary derived datatype and passing a count
  of 1 — which works for point-to-point, RMA `Put`/`Get`, file I/O and the
  uniform-count collectives, but **not** for anything applying a predefined op
  elementwise (`MPI_Reduce`, `MPI_Allreduce`, `MPI_Scan`, `MPI_Exscan`,
  `MPI_Reduce_scatter*`, `MPI_Accumulate`), because predefined ops are not
  defined on derived types. Chunking those into several calls serves the
  blocking forms and cannot serve the nonblocking or persistent ones, which owe
  a single request. `MPI_Pack_c`/`MPI_Unpack_c` are blocked separately: the
  small twin's `position` is an `int *`, so a buffer above 2 GiB cannot be
  walked without reimplementing packing. So the ceiling is not one decision but
  a family of them, and it is deliberately left where it is until a user appears
  who is above it.
- **A datatype's envelope reports the constructor the wrapper used, not the one
  the caller called.** Measured in `dev/large-count-envelope/`: the envelope is
  a property of the constructor, so `MPI_Type_contiguous_c(5, MPI_INT)` reports
  one *large count* on MPICH and the small `MPI_Type_get_envelope` refuses the
  type outright with `MPI_ERR_TYPE`. Under the fallback that same call builds a
  small-count type, so `MPI_Type_get_envelope_c` reports one *integer* and the
  small `MPI_Type_get_envelope` succeeds. The answers are self-consistent — the
  envelope-then-contents-then-rebuild round trip every real consumer performs
  gives the same datatype either way — and the difference is visible only to a
  program asserting about how the implementation classifies its own types.
  §5.10 has why the alternative, a side table keyed on a handle the
  implementation owns and recycles, is worse than the limitation.

- **Fixed-capacity tables are exhaustible, and ordinary programs reach them.**
  1024 op trampolines per variant, 256 errhandler slots per class, 1024 keyval
  pairs. MPICH's own suite exhausts two of them (§6.2). Raising the numbers
  moves the wall; only a reclamation rule removes it, and §6.2 is why there is
  none. Whether these should be configure options is undecided (§13.3).
- **`MPI_BUFFER_AUTOMATIC` is emulated where the implementation lacks it**, with
  a fixed 8 MiB block, against a standard that says "a buffer of sufficient
  size" and means unbounded. A program that would have run against a real
  automatic buffer can still see `MPI_ERR_BUFFER`. Doing better means
  reimplementing buffered mode above the implementation — intercepting every
  `MPI_Bsend` to grow the buffer — which is a second implementation of the
  feature.
- **A staged operation that gets a shared built-in request leaks its staged
  array.** §6.3 has the shape: two zero-work staged operations posted before
  either is waited on hand the request table the same key twice, and it can
  hold only one block per key. The second block is never freed. Bounded by how
  often that happens — never, on everything measured, once (a) and (b) below
  are in the way — counted by `mpiwrapper_staged_leaked()`, and warned about
  once per process in a debug build.

  The two shapes that reach it, from `dev/request-identity/reproduce.c` over
  stock Open MPI 5.0.6 — no CVAR, no MCA parameter — and green now on both
  implementations, macOS and Linux:

  - two `MPI_Ialltoallw` with all counts zero, posted before either is waited
    on;
  - two `MPI_Ineighbor_alltoallw` on a degree-0 topology — where both extents
    are zero, so there was *nothing* to keep alive and the wrapper attached a
    zero-length block only because the attach was unconditional.

  Until (a) and (b) below existed, those two answered `MPI_ERR_INTERN` — a
  conformance bug this section called unreachable. `HISTORY.md` §2.6a keeps
  that correction and which probe row misled it; `probe-staged.c` is the row to
  read now.

  Where the shortcut comes from, read out of the sources:

  - **Open MPI** applies it as one blanket rule to every libnbc collective:
    `NBC_Schedule_request` answers an empty schedule with `ompi_request_empty`,
    and `a2aw_sched_linear` emits nothing for zero-span sends and receives. So
    a rank that exchanges nothing with anybody has an empty schedule **on a
    communicator of any size**, not only `MPI_COMM_SELF`.
  - **MPICH** reaches it only through the generic transport
    (`MPIR_TSP_sched_start` short-circuits `total_vtcs == 0`), which its default
    algorithm selection for `ialltoallw` does not use; the sched transport
    always allocates. `MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM=tsp_inplace` on a
    size-1 communicator reaches it, and that is a documented configuration
    rather than an erroneous program.
  - **Neither** shares a *persistent* request, and neither may: a persistent
    request must be independently startable and freeable, so it cannot be a
    singleton. Measured distinct for both `_init` forms on both.

  **The fix is implemented**: (a), (b) and (c) below, all three in
  `mpiwrapper_staged_keep` (`src/mpiwrapper/staging.c`), which is the one call
  the eight generated bodies make and the one place the policy is written down.
  Five mechanisms were considered; the first two were not built and the reasons
  are kept — the first in `HISTORY.md` §1.22, the second below — because both
  are natural ideas that will occur again.

  - *Probe the built-in request values once at initialization, then free rather
    than attach when a staged operation returns one.* **Rejected** — the probed
    set cannot be complete, and the values drift. `HISTORY.md` §1.22.
  - *Let one key hold more than one block.* The table is open-addressed and
    `mpiwrapper_staged_release` already frees exactly *one* matching entry, so
    the whole change is one arm of `mpiwrapper_staged_attach`: where it meets
    its own key and returns 0, it should keep probing and take a fresh slot.
    **Demoted, not rejected.** It removes the refusal while learning nothing
    about the implementation, which is its virtue; but it leaves the shared key
    *in* the table, so unrelated completions keep draining entries; it spends a
    slot per zero-work operation on a block nobody will read; and it converts
    the missed-completion detector into a silent slot leak that surfaces later
    as a table-full error and less diagnosably. (a) and (b) below get the same
    conformance fix without any of that, so this is worth keeping only as a
    refinement of (c)'s leak path.
  - **(a) Do not attach when there is nothing staged.** All four bodies compute
    `nstaged` and then `malloc(nstaged * …)`; with a degree-0 topology that is
    `malloc(0)`, and the attach proceeded with the non-NULL pointer glibc and
    macOS return. Now the block is freed and no slot is spent. Depends on
    nothing, and removes the degree-0 reproducer outright.
  - **(b) Do not attach an operation that is already complete.** MPI-5.0 §3.7.6
    ("Non-Destructive Test of status") gives `MPI_Request_get_status`: "Sets
    flag = true if the operation is complete... However, unlike test or wait, it
    does not deallocate or inactivate the request" — MPI-2.0, so below the
    floor. On the four `I*` forms, ask it after the call and `free(block)`
    without an entry when the answer is true; any other answer, including an
    error, attaches, because attaching is always safe. The licence to free is
    the sentence §5.7 already quotes: the implementation may read the arrays
    *until the operation completes*, so once it has, there is nothing to
    preserve.

    **The trap is real and is why this is per-kind.** The standard's very next
    sentence — "One is allowed to call MPI_REQUEST_GET_STATUS with a null or
    inactive request argument. In such a case the procedure returns with
    flag = true and empty status" — plus §3.9 making a persistent request
    *inactive* from creation, means a fresh `MPI_Alltoallw_init` request answers
    "complete", and freeing its block on that answer is a use-after-free on the
    first `MPI_Start`. Measured: `flag = 1` on both implementations. So the four
    `_init` forms always attach and are never asked.

    **The generator takes the split from the signature, not the spelling**, per
    §5.7: among the routines that hand back a request and stage an in-direction
    array, the persistent inits are exactly the ones that also take an
    `MPI_Info`. `staged_kind` cross-checks that against the `_init` suffix and
    raises if they disagree, so a ninth entry point joining this set cannot be
    misclassified in silence — which here would be a use-after-free rather than
    a redundant conversion.

    That is not a per-kind rule bought for nothing — the generator already knows
    which of the eight it is emitting, and the split buys three things the
    demoted mechanism does not:

    1. It **keeps the detector**. A duplicate key still means a completion we
       failed to observe, and refusing is still the honest answer — the false
       positives are gone, not permitted.
    2. It **narrows §6.3's exposure**, which §13.3 says permitting duplicates
       cannot do. A shared built-in is only ever handed out for an operation
       complete on return — MPICH sets `cc = 0`, Open MPI sets
       `req_complete = REQUEST_COMPLETED`, and a shared object *cannot* hold
       per-operation completion state — so under (b) a shared key is never in
       the table at all, and the `MPI_PROC_NULL` wait, the shm `MPI_Rput` and
       the eager `MPI_Isend` cannot free anyone's block. What is left of the
       inference is only recycling, which the release-at-every-completion-site
       rule already underwrites.
    3. It frees earlier, so no slot is held for a block nobody reads.

    **Measured, on the path that decides it.** A complete request answers in
    2.3 ns (Open MPI) / 6.2 ns (MPICH). The incomplete path is the one (b) takes
    on every in-flight staged post and the one that can enter the progress
    engine, so it was benchmarked separately, two ranks, minimum of fifteen
    trials, interleaved so drift hits both arms:

    | 2 ranks | poll an incomplete request | added to one `MPI_Ialltoallw`+`MPI_Wait` |
    |---|---|---|
    | Open MPI 4.1.6, `ubuntu:24.04` | 6.5 ns | 1.4 ns, +0.7% |
    | MPICH 4.2.1, `debian:13` | 16.3 ns | 6.8 ns, +1.7% |

    So (b) is free enough, and the zero-hot-path-cost variant — ask only *when
    the attach meets its own key*, same conformance fix, benefit 2 forfeited —
    is not needed. It is recorded because it is the fallback if a host ever
    disagrees.

    **A host did disagree**: the development laptop reports numbers three
    orders of magnitude worse, an artefact of `scripts/host-env.sh`'s own
    `FI_PROVIDER` workaround. The container rows are the numbers;
    `HISTORY.md` §2.17 keeps the trap.

    Open MPI makes this exact move itself: `ompi_coll_base_retain_datatypes_w`,
    which attaches per-operation state to a request, opens with
    `if (REQUEST_COMPLETE(req)) return OMPI_SUCCESS;`.
  - **(c) A duplicate key leaks the block and succeeds.** Whatever survives (a),
    (b) and a false inference is a legal call the caller could have done nothing
    about, and the block cannot be freed — the operation is in flight and the
    implementation may be reading it. So it leaks, counted, with a one-shot
    debug-build warning. Aborting a legal job to avoid leaking one array is the
    wrong trade, and §6.2 has already accepted "never reclaimed" for the
    callback pools and the attribute callbacks, so it is not a new kind of
    concession.

    **A full table is not the same thing and is deliberately still an error.**
    An earlier draft of this section merged them; that was wrong twice over.
    Capacity is the limit every fixed table in this design has, it names a
    build-time constant the user can raise, and it is the *only* black-box
    oracle the release path has — `abi_prototype_test` and `abi_arrays_test`
    both detect a missing release by cycling 1200 staged operations through a
    1024-entry table and watching for `MPI_ERR_INTERN`. Degrading that to a
    silent leak would have taken away the diagnosis and the test in one move.
    So `mpiwrapper_staged_attach` reports *which* fate, and only the duplicate
    one is swallowed. The block leaks in both.

  What the tests check now. `mpiwrapper_selftest` gained `test_staged_policy`,
  which is the only place any of this is reachable: (a) and (b) are invisible
  from outside when they work — they free a block and say nothing — and (c)'s
  duplicate arm now succeeds, so the black-box oracle those cases had is gone by
  design. It asserts (a) spends no slot, that the *persistent* kind attaches the
  very request the nonblocking kind declines (the trap, as a one-argument
  difference), and that a duplicate leaks exactly one block and counts it.
  `mpiwrapper_staged_leaked()` is the counter it reads. `reproduce.c` is the
  behavioural check and is green.

  One coverage note worth keeping, because a passing run reads as more than it
  is: `abi_prototype_test`'s nonblocking round only exercises the table **at two
  ranks or more**. At one rank the `MPI_Ialltoallw` is complete on return, (b)
  frees the block, and nothing enters the table at all — correct, and it means a
  single-rank pass says nothing about the release path. `MPI_ABI_TEST_USE_LAUNCHER`
  is ON by default and `MPI_ABI_EXPECT_RANKS` makes the count an exit status
  rather than a preference, so `ctest` does cover it — but only in a build
  configured that way, and `-DMPI_ABI_TEST_USE_LAUNCHER=OFF` is a supported
  configuration in which it does not.

  **Only (b) touches §6.3's inference**, and only by narrowing it; nothing here
  retires it. Attaches happen at 8 entry points and releases at 11, for *every*
  request, staged or not: on Open MPI, where one `ompi_request_empty` is shared
  across all kinds, a wait on a `MPI_PROC_NULL` `MPI_Isend` decrements a key it
  never incremented. That is harmless exactly when the inference holds, and it
  is where the memory-safety exposure lives. §13.3 has what would retire it.
- **A program that creates unboundedly many ops or keyvals over its lifetime is
  outside what this design serves.** Stated plainly because the table in §6.2
  reads like a corner case and is not.
- **On macOS, a binary linked against this `libmpi_abi` cannot be re-pointed at
  another implementation's, because ours exports `MPI_*` as *strong* symbols
  and the convention is weak.** A Mach-O client linked against a weak-exporting
  `libmpi_abi` binds `MPI_*` through a weak-def-only lookup that a strong
  definition does not satisfy, so mixing the two export styles breaks
  substitution in one direction. Open MPI's ABI branch and MPICH's binding
  generator both emit weak definitions; mpif emits them too, and its
  `check-mpi-install.sh` refuses a prefix without them — which is how this was
  found, and it is Darwin-only, ELF's lookup not distinguishing the two.

  **It is not merely a missing `#pragma weak`, which is why it is recorded here
  rather than fixed.** §2's macOS isolation rests on our exports being strong,
  read from the other side: dyld's weak-definition coalescing chooses among the
  images that *have* weak definitions, and an all-strong `libmpi_abi` never
  enters that contest, so it cannot capture the wrapper's outward call
  (§2, `dev/weakdef-probe/`). Making our definitions weak enters us into it —
  and `libmpi_abi` is loaded first, being what the application links.

  **`dev/macos-weak-symbols/` measures the cost, and it is one cell of four.**
  A client linked against a weak-exporting `libmpi_abi` will not start against
  a strong one — `dyld: Symbol not found` — and the other three combinations
  run, the reverse included: a weak definition satisfies an ordinary lookup, so
  a binary built against *this* project runs against a vendor's library.

  The failing direction is the one that matters: **build against a vendor's ABI
  library, run through this wrapper over a site MPI**, which is the case the
  ABI exists for.

  **The other half — what going weak would cost — is now measured too, by the
  two probes together.** `dev/macos-weak-symbols/` part 2 holds the
  implementation two-level and weak and varies *our* export style: strong is
  isolated, weak is captured. `dev/weakdef-probe/` holds our exports strong and
  varies the implementation: a two-level implementation is never captured,
  weak or strong. The two agree and compose into one rule — **coalescing is
  decided among the weak-exporting images, and the first loaded wins** — with
  our library loaded first in every configuration.

  That reading also retires the reason part 2 was first recorded as suspect. It
  looked like it contradicted `HISTORY.md` §2.3, which measured a *strong*
  `libmpi_abi` being captured against a real ABI-built Open MPI; §2.19 removed
  the contradiction by finding that capture to be a `-flat_namespace`
  implementation build rather than a coalescing outcome at all. Three
  mechanisms, not one: substitution (part 1), coalescing (part 2), and flat
  lookup (§2.19).

  So the remedy's price is no longer unknown: **exporting `MPI_*` weakly would
  trade the substitution asymmetry for the loss of macOS isolation** against
  exactly the two-level ABI-implementing MPI that §10's oracle 5 now wraps
  there. It stays recorded rather than fixed for that reason, and it is **the
  open question a 1.0 should state rather than the bug it looks like**. What no
  measurement covers yet is a weak-exporting `libmpi_abi` in the real
  configuration rather than the mock; the four mpif rows of §10 are Linux, so
  CI would not catch a regression here.

- **A wrapped MPI that ships its own `libmpi_abi.so` can capture ours through
  `LD_LIBRARY_PATH`.** `libmpi_abi.so` is what this project builds, and it is
  also what an MPI implementing the standard ABI natively installs, so the two
  collide by filename in the one environment guaranteed to contain both: the
  wrapped implementation's own. **Measured over Intel MPI 2021.18**, which ships
  `libmpi_abi.so.1` with 670 `MPI_*` symbols and puts its `lib` directory on
  `LD_LIBRARY_PATH` from `vars.sh`. Our binaries carry `NEEDED libmpi_abi.so`
  with a `DT_RUNPATH`, and **`DT_RUNPATH` is searched after `LD_LIBRARY_PATH`**,
  so the loader binds them to the implementation's library: five of thirteen
  tests fail — precisely the five that load a wrapper — and all thirteen pass
  with the variable cleared (`dev/third-implementations/`).

  What makes this a limitation rather than a CI detail is the failure it *can*
  produce. Here it was five red tests, because our version script gives
  `MPIABI_1` a verdef the implementation's library has no match for and `ld.so`
  says so. A consumer whose program happens to use only entry points the real
  ABI implements would instead run correctly against the wrong library and never
  know this project was bypassed — which is `HISTORY.md` §2.14's failure mode by
  a new route.

  Three things bound it, and none is a fix. `check-install.sh` clears
  `LD_LIBRARY_PATH` for every consumption route, which is why its six legs
  passed on the run where `ctest` did not — that disagreement is what exposed
  this. `ci.yaml`'s `linux-oneapi` job does not export the variable and needs
  nothing from it, since `mpicc -show` bakes an RPATH to the implementation's
  `lib`; it also pins Intel MPI 2021.15, below the 2021.17 that first shipped
  such a library, and fails loudly if a bump brings one back — so no green run
  depends on the workaround. And the collision needs the implementation's `lib`
  on the loader path, which an ordinary consumer of an installed prefix has no
  reason to put there.

  **The real fixes are undecided and both cost something:** a `SOVERSION`, so
  our SONAME is `libmpi_abi.so.0` against MPICH's and Intel's `.so.1`, which
  changes this project's installed ABI surface; or `-Wl,-z,nodeflib` and
  friends, which do not help, because the problem is search *order* rather than
  default directories. §13.3 is where this belongs once someone chooses. Note
  that it is **not** §10's fifth oracle: deliberately wrapping an
  ABI-implementing MPI is a row that does not exist yet and needs
  `MPIWRAPPER_WRAP_ABI_IMPL`. MPICH 5.0 and MVAPICH 4.1 both carry
  `--enable-mpi-abi` as well, so the hazard is general; Intel MPI 2021.17+ is
  only the one that ships it built.

### 13.3 Open design questions

- **What does this layer owe an erroneous program?** An *erroneous* argument
  that the implementation would have diagnosed becomes a **crash**, because
  conversion interposes a local: `MPI_Comm_create(comm, group, NULL)` is
  `MPI_ERR_ARG` on MPICH and a segfault through the wrapper, since the body
  writes the converted handle through the caller's null pointer. Twenty tests in
  MPICH's `errors/` directory are exactly this, and `MPI_IN_PLACE` passed where
  it is illegal is the same shape one level up — §5.3 translates a sentinel only
  where it is *legal*, so where it is not, the ABI's value reaches the
  implementation as an ordinary address.

  Every such program is erroneous and MPI grants an implementation licence to do
  anything at all with it. But "anything" here is a crash where the wrapped
  implementation had a diagnosis, and **interposition is what took the diagnosis
  away**. What would answer it is a null check per written-through OUT
  parameter, emitted by the generator — a stage of its own, and one whose cost
  (a branch per OUT argument, on every call) belongs in a benchmark rather than
  in a paragraph. This is the largest open question in the design.
- **Is §6.3's request-sharing inference sound?** The staged-temporary table
  frees by handle value, and the argument that this cannot free an in-flight
  block is derived from `MPI_Wait`'s semantics rather than quoted from the
  standard, which says nothing about handle uniqueness across operations. It is
  load-bearing for *memory safety*, so it deserves better than an argument.

  **Permitting duplicate keys does not help here**, and the reason is worth
  keeping because the idea is a natural one: counting activations per key fixes
  the attach side and leaves the release side exactly as exposed, since
  releases arrive from the 11 completion entry points for every request while
  attaches happen at only 8. The decrement that frees the wrong block comes
  from an operation that never incremented.

  **§13.2's (b) narrows it without retiring it**, which is the one thing on the
  attach side that does. Declining to attach an already-complete operation keeps
  every shared built-in out of the table, and a shared built-in is the only
  handle two operations of *different kinds* have ever been observed to share.
  What is left is one mechanism rather than two: a handle *recycled* after a
  completion the wrapper failed to observe. That is still an inference, and it
  is still not a citation, but it is the one the release-at-every-completion-site
  rule is built to underwrite, and a counterexample would be an implementation
  bug rather than a licensed shortcut.

  Two redesigns would genuinely retire even that, and both are decision-10
  changes that want a benchmark rather than an argument:

  - **Count every live operation holding the handle, not just the staged
    ones.** Then the count is a fact rather than an inference: two operations
    sharing a handle each increment, and the block goes when the second
    completes, whether or not either was ever complete on return. The price is
    instrumenting every request-*producing* entry point — the `I*` forms,
    nonblocking collectives, nonblocking I/O, request-based RMA, generalized
    requests, the persistent inits — roughly a hundred rather than eight, and a
    table insert on the hot path of every `MPI_Isend`, where decision 10
    currently pays one relaxed load and a compare against zero. It also brings
    back a per-kind rule, since `MPI_Start`/completion would take a persistent
    request's count to zero while its block must live to `MPI_Request_free`
    (`HISTORY.md` §1.10 is the flag this resurrects).
  - **Key the table on something we own** rather than on the implementation's
    handle, which removes the question instead of answering it — and costs an
    interning step on every request.

  Meanwhile `dev/request-identity/` can be extended further to hunt for a
  counterexample. It can only ever find one, never prove absence, which is why
  it is listed last — but note that extending it once already found the
  attach-side one this section's earlier draft called unreachable, so "listed
  last" should not be read as "unlikely to pay".
- **The availability probe cannot see an entry point that is declared and not
  defined.** §3 has the case and the reason the five known instances were solved
  differently. The general fix is a **link stage** in the probe: linking is
  compile-time, so cross-compilation forbids only *running*, and done by
  bisection it needs no parsing of linker diagnostics at all, only their exit
  status. It is the right answer for any future instance and is not implemented.
- **Capacity defaults for the fixed-size tables**, and whether they should be
  configure options rather than compile-time constants. Related to §13.2's first
  bullet: a configure option does not remove the wall, and it does move the
  decision to someone who knows the workload.
- **Attribute copy/delete callback lifetimes in detail.** The implementation
  invokes them during `MPI_Comm_dup`, `MPI_Comm_free` and `MPI_Finalize`, so the
  `{user_fn, user_extra}` pairs must outlive everything the user holds. §6.2
  concludes they are never freed, which is safe; what is undecided is whether
  anything better is possible at finalize.

### 13.4 Platforms and configurations not yet supported

- **Intel MPI 2021.16 specifically**, and it is a one-release hole rather than a
  version floor. That release declares

  ```c
  typedef void (MPI_T_event_dropped_cb_function)(int count, ...);
  ```

  where MPI-4.1, MPICH, MVAPICH and Intel MPI 2021.17+ all say `MPI_Count
  count`, so `src/mpiwrapper/toolevents.c`'s trampoline gets `conflicting types
  for 'mpiwrapper_t_event_dropped_tramp'` and the wrapper does not build.
  2021.15 and earlier do not declare that callback at all, so the
  availability probe leaves the trampoline out and they build clean; 2021.17
  fixed the type. **The `MPIWRAPPER_HAVE_*` probe answers "is it declared", and
  this is the first case where the answer needed to be "declared with what
  signature".** A fix is a configure-time probe of the parameter type plus a
  second trampoline body, which is §8 work rather than a CI change, and nobody
  has needed 2021.16 yet — `ci.yaml`'s `linux-oneapi` job pins 2021.15 and says
  why. Measured in `dev/third-implementations/`.

- **Windows/mingw**: a `dlopen` → `LoadLibrary` shim, no RTLD flags, and it is
  not settled which MPI is even the target there.
- **musl / Alpine**: neither isolation mechanism exists (§12). Expected to be
  refused at load, which is the correct outcome but not support.
- **FreeBSD**: **not supported.** Measured on 14.3, and the reason is
  structural rather than a bug of ours. The whole project builds,
  `RTLD_DEEPBIND` is declared, and the `dlopen` succeeds — but the wrapper's
  `MPI_*` calls still resolve back into `libmpi_abi`, and the outward-resolution
  check refuses at load, which is §2's reliability property working.

  The flag is implemented there, and more narrowly than on glibc. FreeBSD's
  `dlopen(3)` says it puts "symbols **from the loaded library**" before global
  ones; glibc's puts "the lookup scope of the symbols in this shared object"
  ahead, and that scope includes the object's `DT_NEEDED` subtree — which is why
  `dev/dlopen-probe/` measured it applying transitively (§2). We need
  `libmpiwrapper`'s `MPI_Send` to reach **`libmpi`**, a dependency;
  `libmpiwrapper` defines no such symbol itself, so the narrower rule has
  nothing to promote and the global scope wins. Identical wording in 14.3 and
  15.1, so this is not a release that will age out. **Not the same as musl**,
  which ignores the flag outright; an earlier draft said it was.

  `PMPI_*` routing is not the escape — §2: "we export those too, so both names
  are captured". The only untested candidate is linking the implementation
  statically into `libmpiwrapper` with `-Bsymbolic`, so the names are defined
  inside it and bind locally; that is §11's S9 option in another costume.

  **The CI row is dropped**, having established this: a permanently red row
  teaches nothing after the first run. `git show 236b99a` is the recipe, and
  what would settle the mechanism beyond the man page is a FreeBSD
  `dev/dlopen-probe/` — a library whose *dependency* defines a symbol a global
  object also defines.
- **32-bit**: exercised for the loader probe and the type-identity probe, not
  for the library. S9's row.
- **Static libraries**: §9.
- **An ABI-implementing MPI on macOS**: works when the implementation is a
  two-level build, measured against Open MPI main `--enable-standard-abi`
  (§10's oracle 5) but with no CI row yet; a `-flat_namespace` implementation
  build is refused at load, which is the correct outcome (§2, `HISTORY.md`
  §2.19). An earlier version of this line said "impossible, not merely
  unsupported", from measuring only the flat build.

---

## Sources

Facts above were read from these files rather than from memory or from secondary
documentation.

| what | where |
|---|---|
| ABI header (mpi-abi-stubs, patched) | `gen/include/mpi.h`; originally `~/src/mpif/build/mpi/mpich-gcc/include/mpi.h` |
| MPICH header | `~/src/mpif/build/mpi-src/mpich-gcc/mpich/src/include/mpi.h` |
| Open MPI header | `~/src/mpif/build/mpi-src/openmpi-gcc/ompi/ompi/include/mpi.h.in` |
| MPI-5.0 standard | `doc/mpi50-report.pdf`, read with `pdftotext -layout` |
| CI and build precedent | `~/src/mpif/ci-scripts/README.md`, `~/src/mpif/CMakeLists.txt` |
| loader, weak-definition, symbol-versioning, dispatch, handle-map, request, type and extent behaviour | the nine probes in `dev/`, each with its own README |

Standard sections cited: §2.5.2 (opaque object deallocation), §3.2.5 and §3.7.5
(the status error field), §3.7.6 (`MPI_REQUEST_GET_STATUS`, and that it answers
`true` for an inactive request), §3.9 (a persistent request is inactive from
creation), §6.11 (`MPI_IN_PLACE` in `alltoallw`), §6.12 and §6.13
(nonblocking and persistent collective argument lifetimes), §7.7
(`MPI_COMM_FREE_KEYVAL`), §9.4 (`MPI_ERRHANDLER_FREE`), §9.5 (dynamic error
classes), §11.4.1 (finalize and `MPI_COMM_SELF` attributes), §12.3.4
(`MPI_ACCUMULATE` forbids user-defined ops), §14.2.2 (`MPI_Pcontrol`'s extra
arguments), §14.3 (`MPI_DISPLACEMENT_CURRENT`), §15.2.1 (profiling interface
requirements), §15.3 and 15.3.6–15.3.9 (`MPI_T` events, null OUT pointers),
§20.2.1 (the ABI library must be named `mpi_abi` and be the sole direct
dependency), §20.4 (the C–Fortran converters are outside the ABI), §20.4.3 (the
Fortran status layout), §20.4.5 (handle serialization values).

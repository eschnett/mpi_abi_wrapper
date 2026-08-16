# MPI ABI wrapper — design

The authoritative design document: what this project is, why it is shaped the
way it is, and what about it is missing, broken or undecided. One of three:

| | holds |
|---|---|
| `CODE.md` | what the repository contains now, and the number behind every claim |
| **`NOTES.md`** | the design, its reasons, and what is missing, broken or undecided |
| `HISTORY.md` | roads not taken, beliefs that were measured false, and the stage record |

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
  namespace. That assumption is load-bearing — a macOS build must never acquire
  `-flat_namespace` by accident — and it is checked at load rather than at
  configure time (§2).
- **The ABI surface is complete and is MPI-5.0** (plus the Fortran extension of
  `doc/mpi.h.patch`). A function the implementation lacks is **reported at run
  time**, never omitted from the ABI: the slot returns
  `MPI_ERR_UNSUPPORTED_OPERATION` and the generator lists it in
  `gen/report.txt`. An application must be able to link and start against any
  wrapper and discover at run time what is missing.
- **The implementation is expected to provide the MPI-4.0 API.** That is what
  makes the common case a 1:1 mapping with no large-count narrowing fallback,
  since the `_c` variants exist there. It is an expectation, not a hard floor.

  **And it is not met by any released Open MPI.** Open MPI 5.0.10 defines
  `MPI_VERSION 3` / `MPI_SUBVERSION 1` and has **no `_c` entry point at all** —
  `MPI_Send_c`, `MPI_Type_create_struct_c` and the rest of the family are simply
  absent from its header. So the MPI-4.0 configure check is a warning rather
  than a hard error, and the enforced floor is MPI-3.0. What that costs is
  exactly the large-count half of the surface: those slots become decision 6's
  stubs. It is worth knowing that the mechanism is **load-bearing on day one**
  and not a contingency for exotic implementations.

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
   that matters, and `HISTORY.md` §2.3 has the measured case where the two
   answers differ — dyld coalesces weak definitions across images, so our strong
   `MPI_Send` can win over an implementation's weak one *while taking the
   symbol's address still resolves correctly*, and the symptom is silent double
   execution rather than recursion. So the ABI side makes one call through the
   vtable and sees whether the call comes back. `MPI_Get_version` is the probe:
   legal before `MPI_Init` in every version of the standard, no side effects.
   (`MPI_Wtime` reads better and is wrong — `HISTORY.md` §2.5.)

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

One consequence for the design: **a wrapper cannot be layered over an
ABI-implementing MPI on macOS at all.** Refusing at load is the best available
outcome and is not fixable — nothing in the two-level namespace overrides weak
coalescing. The fix that would exist is to route the wrapper's calls through
`PMPI_*`, which is strong everywhere; the cost is decision 7's
implementation-level interposition, so it would have to be opt-in and loud
rather than a silent fallback. See oracle 5 in §10.

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
| status layout | **no runtime recourse exists** — nowhere to put a private part exceeding 20 bytes, and side storage keyed on a status address is unsound because statuses are freely copied → build failure |
| dynamic handle collision | one compare, and only on object creation → run time |

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
   passes all six tests at two ranks. It needs an older toolchain to build at
   all (gcc 9; gcc 11 and 13 both reject its own headers) and a newer CMake than
   Ubuntu 20.04 ships, which is worth knowing before promising a distro matrix.
2. **No self-wrapping.** Hard error if the found MPI prefix is *our own*
   installation, detected by the presence of `mpiwrapper_vtable.h` — a file only
   we install. Neither `MPI_ABI_VERSION` nor the library name discriminates,
   since a genuine ABI-implementing MPICH installs `libmpi_abi` too. The
   accident is easy to hit, because `find_package(MPI)` is looking for exactly
   the `mpi.h` and `libmpi_abi` we install, and its symptom is a startup loop
   rather than a diagnostic. `mpi.h` itself stays pure — the stub header plus
   `doc/mpi.h.patch`, with no marker macro. Wrapping a *genuine* ABI MPI is
   permitted behind an explicit flag, with a warning; see oracle 5.
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
| MPICH >= 4.0 | the only implementation that actually provides the `_c` surface, so the large-count half of the ABI is exercised at all |
| Open MPI >= 5.0 | sessions, and the current component architecture; 4.1 is *wrappable* and is a legitimate extra row rather than an excluded one |
| MPICH 3.1.4 | the MPI-3.0 floor itself, verified rather than declared |

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

   **It is a Linux-only oracle.** An ABI-implementing MPI declares its `MPI_*`
   weak, and on macOS dyld's weak-definition coalescing binds the wrapper's
   outward calls back to us; the wrapper detects this at load and refuses, which
   is the correct outcome and not a fixable one (§2). On ELF `RTLD_DEEPBIND`
   resolves it, because scope order there beats weak-vs-strong. So this oracle
   runs on the Linux rows, and the macOS rows get its refusal as a test instead.

   **It does not currently build against Open MPI's own ABI mode**, for a reason
   that has nothing to do with the wrapper: `mpicc_abi`'s `mpi.h` does not
   declare `MPI_Fint`, so `internal.h`'s `sizeof(MPI_Fint) ==
   sizeof(MPIABI_Fint)` assertion has nothing to name. The right fix — probing
   for `MPI_Fint` the way entry points are probed, or dropping the assertion
   where the Fortran interface is absent — belongs with whoever makes that
   oracle a CI row.

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
- **MPICH's C test suite** is the first oracle nothing in this repository wrote,
  and it earned that position: three conversion bugs no in-house check could
  have seen (`HISTORY.md` §3, S7). Its gate reads the expected-failure list in
  both directions and rejects a line with no reason.
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

**MVAPICH is worth a row, and a cheap one.** It is MPICH-derived, so its handle
values, error classes and status layout are MPICH's and the conversion tables
are already exercised; what it adds is a third *installation* shape — its own
library naming, its own `mpicc`, its own launcher — which is where "all three
consumption routes must build and run a program" gets tested against something
nobody tuned it for. The same argument extends to Intel MPI and Cray MPICH.

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

**`dlmopen` is not the fallback it was planned to be** (§2). The options that
remain, in the order worth trying: measure whether `RTLD_DEEPBIND` really does
disturb the sanitizers rather than assuming it; build the MPI under test with
its components static, so nothing is `dlopen`ed at run time — which
`HISTORY.md` §1.5's MPICH 3.1.4 result says would also make `dlmopen` work; or
accept that the sanitizer jobs cover `libmpi_abi` and the conversion layer
rather than the loaded configuration.

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

- **Read only the sections a task names.** These files are long; loading all of
  them wastes the context the work needs. `CODE.md` §2 and `gen/report.txt`
  answer most "where is X" questions on their own.
- **Do not re-litigate the numbered decisions.** They have reasons recorded,
  several of them measured.
- **New findings go in the section that owns the rule, not only in a stage
  narrative.** This is the failure mode that a review found three times: S7's
  `status.MPI_ERROR` rule, its displacement sentinel and its attribute-value
  class were all recorded accurately in a per-stage account and left out of §5,
  which is the section a later session actually reads. A finding is not filed
  until the rule it changes says so.
- **New measurements go in `dev/`.** A claim in a commit message is a claim
  nobody will find again.
- **Prefer a benchmark or a probe to an argument** for anything performance- or
  loader-related, and **check any benchmark against its own disassembly** — two
  of the three benchmarks here reported confidently wrong numbers first
  (`HISTORY.md` §2.11).
- **Check every count against the artifact, not against another sentence.**
  Eleven have been wrong, each one `grep` from being right. `CODE.md` carries an
  authority column for this reason; if a number is worth writing down, write
  down how to re-derive it.
- **End committed and green.** Work that leaves the tree red has not produced
  something the next session can build on.

---

## 12. Risks worth measuring

- Whether a dynamically created implementation handle can be bit-cast into an
  ABI handle without colliding with `0x20`..`0x2eb`. Probably yes for both
  implementations (§5.1), but the probe has to be a runtime test, not a
  configure-time one — and it exists, in `mpiwrapper_selftest`.
- Whether `RTLD_DEEPBIND` survives ASan, which is the case it is most likely to
  disturb. **Measure before assuming**, and note that the answer no longer
  selects `dlmopen`, which is not available for this (§2, §11's S9).
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

Each is a behaviour a user can hit, and all four should be in the release notes.
All four are now deliberate — the design chose them, and §6.2 says why for the
first two. The fourth used to be a conformance bug rather than a choice: a legal
call answered with `MPI_ERR_INTERN`. That is fixed, and what is left in its
place is a bounded leak, which is a limitation and not a bug. The history is
kept under it because the wrong reading that hid it is worth not repeating.

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

  **Until it was fixed this was a conformance bug, and this section said it was
  unreachable.** Both halves of that are worth keeping.

  *Memory safety* was never at risk — that is what declining bought, and it is
  why §6.3 calls it safe. *Conformance* was broken whenever it fired: a legal
  call got an error the wrapped implementation would not have given, and the
  default error handler turned that into an aborted job. *Reachability* **was
  not nil.** `dev/request-identity/reproduce.c` got `MPI_ERR_INTERN` out of the
  wrapper over stock Open MPI 5.0.6, no CVAR and no MCA parameter, from two
  shapes:

  - two `MPI_Ialltoallw` with all counts zero, posted before either is waited
    on;
  - two `MPI_Ineighbor_alltoallw` on a degree-0 topology — where both extents
    are zero, so there was *nothing* to keep alive and the wrapper attached a
    zero-length block only because the attach was unconditional.

  Both are green now, on both implementations and on macOS and Linux.

  What misled the earlier reading was `dev/request-identity/probe.c`'s
  `MPI_Ialltoallw` row, which reports distinct handles and is the one shape that
  gets them: distinct buffers and a count of 1, which is exactly what builds a
  non-empty schedule on a one-rank communicator. `probe-staged.c` is the row to
  read now, and `HISTORY.md` §2.6a keeps the correction, because it is that
  section's class twice over: reasoning about what an implementation *would not*
  do, and reading a measurement's scope as wider than it was.

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
  are kept, because both are natural ideas that will occur again.

  - *Probe the built-in request values once at initialization* — a
    `MPI_PROC_NULL` `MPI_Isend` and an `MPI_Ibarrier` on `MPI_COMM_SELF` reveal
    them — then free rather than attach when a staged operation returns one.
    **Rejected:** the set is only as complete as the operations provoked, and
    nothing caps how many built-ins an implementation may have. A second reason
    has since turned up: the values are not stable. MPICH's are
    `0x6c000000 | kind`, the kind enum grows, and two of MPICH's own named
    `MPIR_REQUEST_COMPLETE_*` macros are stale by exactly that drift.
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

    Measured cost: 2.3 ns/call (Open MPI) and 6.2 ns/call (MPICH) against a call
    already doing an `MPI_Comm_size`-class query, an O(P) `malloc` and O(P)
    conversions. The incomplete path enters the progress engine and wants its
    own benchmark before this is called free. If one ever objects, the
    zero-hot-path-cost variant is to ask only *when the attach meets its own
    key* — same conformance fix, but benefit 2 is forfeited.

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
  frees the block, and nothing enters the table at all — correct, and also why
  `ctest` alone no longer covers the release path. `ci-scripts/linux-test.sh`
  runs `ctest` single-rank; the two-rank runs are by hand
  (`run-linux-docker.sh`, then `mpirun -n 2` on the test binaries).

  **Only (b) touches §6.3's inference**, and only by narrowing it; nothing here
  retires it. Attaches happen at 8 entry points and releases at 11, for *every*
  request, staged or not: on Open MPI, where one `ompi_request_empty` is shared
  across all kinds, a wait on a `MPI_PROC_NULL` `MPI_Isend` decrements a key it
  never incremented. That is harmless exactly when the inference holds, and it
  is where the memory-safety exposure lives. §13.3 has what would retire it.
- **A program that creates unboundedly many ops or keyvals over its lifetime is
  outside what this design serves.** Stated plainly because the table in §6.2
  reads like a corner case and is not.

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

- **Windows/mingw**: a `dlopen` → `LoadLibrary` shim, no RTLD flags, and it is
  not settled which MPI is even the target there.
- **musl / Alpine**: neither isolation mechanism exists (§12). Expected to be
  refused at load, which is the correct outcome but not support.
- **FreeBSD**: `RTLD_DEEPBIND` is the intended mechanism and is unverified.
- **32-bit**: exercised for the loader probe and the type-identity probe, not
  for the library. S9's row.
- **Static libraries**: §9.
- **An ABI-implementing MPI on macOS**: impossible, not merely unsupported (§2).

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
| loader, dispatch, handle-map, request, type and extent behaviour | the six probes in `dev/`, each with its own README |

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

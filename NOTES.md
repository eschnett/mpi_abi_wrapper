# MPI ABI wrapper — design

The authoritative design document for this project. Self-contained: it does not
depend on any other document in this repository.

Every numeric claim about the ABI or about MPICH and Open MPI was read out of the
actual headers; see "Sources" at the end for the paths, so any of it can be
re-checked.

---

## 1. Goal, deliverables, scope

Implement the MPI-5.0 C ABI on top of an existing MPI implementation that does not
itself provide the ABI. Each ABI call is forwarded to that implementation ("the MPI
library" below), converting arguments in both directions.

**Deliverables**

| | built | needs an MPI? |
|---|---|---|
| `libmpi_abi` + `mpi.h` | once, implementation-independent | no |
| `libmpiwrapper` | once per MPI installation | yes |

`mpi.h` comes from [mpi-forum/mpi-abi-stubs](https://github.com/mpi-forum/mpi-abi-stubs)
with `doc/mpi.h.patch` applied. That patch does two things: it adds the
Fortran-support declarations (`MPI_Fint`, `MPI_F08_Status`, the four
`MPI_F*_STATUS(ES)_IGNORE` macros with real storage behind them, and the 26
handle/status converters), and it corrects an error in the stub header where
`MPI_Psend_init`/`MPI_Precv_init` were given `int count` with a separate `_c`
variant instead of `MPI_Count count`.

**Scope, counted from the patched header rather than estimated**

- **688 entry points**, and `MPI_*`/`PMPI_*` are exactly symmetric — every one has
  a twin, with no exceptions in either direction.
- Of those: **611 core**, **51 `MPI_T_*`**, **26 Fortran converters** (22 handle
  converters for 11 handle types, plus 4 status converters).
- 31 are marked deprecated in the header. Deprecated still means provided.
- So: **688 vtable slots**, **1376 exported symbols** in `libmpi_abi`.

**Consumer.** [mpif](https://github.com/eschnett/mpif) provides MPI Fortran
bindings over the ABI and is a downstream consumer of this project; requesting
changes there is acceptable. The two projects also share problem shapes (both
generate code for the whole MPI standard), and several conventions here are lifted
from it deliberately.

**Constraints**

- **Thread safety.** MPI applications may be multi-threaded. Prefer atomics over
  anything needing a support library.
- **Portability.** Linux and macOS are required; FreeBSD and Windows (mingw) if
  possible. Compiler-specific mechanisms are acceptable.
- **Cross-compiling is supported**, which has a sharp consequence: *nothing may be
  determined by running a program at configure time.* Every check is either a
  compile-time assertion or a runtime check inside the library.
- On macOS it is acceptable to assume the MPI library uses a two-level namespace;
  if anything comes to rely on that, verify it at configure time.
- **Minimum implementation is MPI-4.0.** Every ABI function then maps 1:1 to one
  implementation call and no large-count narrowing fallback is needed. This
  excludes Open MPI 4.x. MPI-4.1-only features still need `#ifdef` handling.

---

## 2. Architecture

```
application
    |  MPI_Send(...)                        ABI types only
    v
libmpi_abi.so          exports MPI_* and PMPI_* (1376 one-line functions)
    |                  includes the ABI mpi.h and nothing else
    |  vt->MPI_Send(...)                    ABI types only, 688 slots
    v
libmpiwrapper.so       exports mpiwrapper_get_vtable and nothing else
    |                  includes the implementation's mpi.h + generated mpiabi.h
    |                  owns every conversion, trampoline and map
    |  MPI_Send(...)                        implementation types, direct call
    v
libmpi.so              linked normally, not dlopened
```

**Why two libraries.** `libmpi_abi` must export `MPI_Send`, and so does `libmpi`.
That collision is the only reason `dlopen` is involved: `libmpi_abi` never links
the MPI library, it loads `libmpiwrapper` at run time, and `libmpiwrapper` pulls in
`libmpi` through an ordinary `DT_NEEDED`.

**Why the boundary is ABI-typed.** Because the conversions live on the far side of
it, `libmpi_abi` contains no implementation types, needs no MPI to build, and is
built once for all implementations. `libmpiwrapper` links the MPI library directly,
so it calls `MPI_Send` and reads `MPI_COMM_WORLD` as ordinary code — no `dlsym` of
MPI functions, no preprocessor prelude renaming the implementation's ~700 function
names out of the way, and no need to export predefined handle values as symbols.

An earlier design put the conversions in `libmpi_abi`, which then had to include
the implementation's `mpi.h`, `#define` every implementation function name aside to
avoid colliding with the ABI functions it defines, `dlsym` all 688, and obtain
predefined handles through a helper library that existed only to give them
linker-visible names. All of that disappears. The cost is one extra *direct* call
per MPI call; the number of indirect calls is unchanged at one.

**The vtable.** `libmpiwrapper` exports exactly one symbol:

```c
const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t layout_hash, size_t size);
```

returning NULL with a diagnostic on mismatch. `abi_version` is the header's
`MPI_ABI_VERSION` (currently 1); `layout_hash` is generated from the slot list, so
a regeneration that reorders or inserts a slot is caught rather than silently
calling through a shifted one. A getter rather than an exported struct, because you
would otherwise have to trust the layout in order to read the version out of it —
and because the getter is a natural place to build the reverse handle map before
returning.

**PMPI costs no slots.** `MPI_Send` and `PMPI_Send` are two separate one-line
definitions in `libmpi_abi` calling the same slot. Two definitions rather than a
weak alias: macOS aliases need `-Wl,-alias` or `__asm__` labels, and a one-line
body makes an alias pointless. Interposition still works correctly — a tool
interposes the application's `MPI_Send`, calls `PMPI_Send`, and lands in
`libmpi_abi`.

**Locating the wrapper.** An environment variable, falling back to a path baked in
at build time. One `libmpi_abi` binary can therefore be pointed at any wrapper,
which is the practical payoff of the split. The wrapper's installed name encodes
its MPI (`libmpiwrapper-mpich-4.3.so`) so several coexist in one prefix.

**Bootstrap.** A library constructor in `libmpi_abi`, plus an idempotent
acquire-load guard on the vtable pointer so that a plugin `dlopen`ed before the
constructor runs still works. The guard is needed regardless of the constructor
because `MPI_Initialized`, `MPI_Get_version`, `MPI_Wtime` and the `MPI_T_*` calls
are all legal before `MPI_Init`, so the load cannot hang off `MPI_Init`. One
predictable branch per call.

### Dispatch cost on the ABI side

`dev/dispatch-bench/` measures the candidate shapes, calling across a DSO boundary
with the vtable made opaque, on Linux/gcc and macOS/clang (aarch64).

| shape | trivial callee | ~320 ns callee | instructions (gcc) | `.text` x1376 |
|---|---|---|---|---|
| static function pointers | 1.085 ns | −1.32% | 4 | 22,252 B |
| vtable copied into our storage | **1.075 ns** | −0.82% | 4 | 22,252 B |
| vtable via pointer | 1.084 ns | −0.86% | 5 | 22,252 B |
| pointer + atomic acquire + lazy branch | **1.630 ns** | −0.15% | **23** | **95,436 B** |

Two conclusions, and they point different ways:

- **Drop the atomic and the lazy-init branch.** It is the only shape that measures
  worse, and the reason is that a possible cold call to the initializer forces a
  stack frame into every entry point: 23 instructions instead of 4, and **95 KB of
  text instead of 22 KB** across 1376 entry points. On a call that does real work
  the time is invisible (−0.15%), so this is a code-size decision.
  It is safe to drop because anything that can call `MPI_Send` must link
  `libmpi_abi`, hence depends on it, hence its constructors run after ours — so the
  window in which the pointer is NULL contains no code that can reach an entry
  point. Keep an `assert` in debug builds only.
- **Do not bother copying the vtable.** 1.075 against 1.084 ns is noise and the
  `.text` is byte-identical: the extra load is off the dependency chain, so an
  out-of-order core issues it in parallel. Keeping the single pointer also leaves 8
  bytes of writable function pointer in our data instead of 5.5 KB of it.

Both benchmark bugs found on the way there are recorded in that directory's README,
because both produced confident wrong numbers: measuring each shape to completion
let thermal drift land on one shape (reporting +213% for one extra load), and
building the vtable from a `static` in the same TU let the compiler devirtualize two
shapes into direct calls. Only the disassembly caught the second. **Any benchmark of
this layer has to be checked against its own disassembly**, since what is being
measured is exactly what an optimizer most wants to remove.

### Symbol resolution when loading the wrapper

This is the most delicate thing in the design, and an earlier draft of these notes
got it backwards by recommending `RTLD_GLOBAL`. The correct answer is
`RTLD_LOCAL` plus active isolation, matching what MPItrampoline already does
(`MPItrampoline/src/mpi.c:477`).

**The problem is not what the wrapper exports; it is how the wrapper's own
references resolve.** On ELF a `dlopen`ed object resolves its *undefined* symbols
against the global scope **first** and its own dependency subtree second — the
asymmetry `RTLD_DEEPBIND` exists to invert. The application links `libmpi_abi`, so
`libmpi_abi` is in the global scope, so `libmpiwrapper`'s reference to `MPI_Send`
binds to *our* `MPI_Send`:

```
libmpi_abi::MPI_Send -> vtable -> w_MPI_Send -> libmpi_abi::MPI_Send -> ...
```

Infinite recursion, in the default configuration. `RTLD_LOCAL` does not fix it:
`LOCAL`/`GLOBAL` controls what the loaded object *exports*, not how its references
resolve. **Isolation is mandatory, not an optimization.**

**Why `RTLD_GLOBAL` is actively harmful**, three independent reasons:

- It puts `libmpi`'s `MPI_Send` into the global scope, so a plugin `dlopen`ed
  *later* binds to the native MPI — global is searched before the plugin's own
  local scope, where `libmpi_abi` lives — and is then handed ABI-typed handles and
  a 32-byte status. Silent corruption, and only in the second plugin. This is a
  normal configuration, not an exotic one: mpi4py plus a second MPI-using extension
  module in one Python process.
- The implementation's own internals are written against MPI in places (Open MPI's
  ROMIO and io components). Capturing those is not merely wrong but
  **memory-unsafe**: a component calling `MPI_Recv` passes a 24-byte
  `ompi_status_public_t`, and our ABI `MPI_Recv` writes 32 bytes into it.
- Handles would survive such a capture *by accident* — dynamic ones bit-cast to
  themselves, and predefined implementation values sit outside the ABI's
  `0x20`..`0x2eb` range so they bit-cast through too — which makes the failure
  intermittent and data-dependent rather than immediate.

Calling `PMPI_*` internally does not save the implementation: we export those too,
so both names are captured.

**Measured, not reasoned.** `dev/dlopen-probe/` is a mock-up of the three-library
structure with no MPI in it, run on Linux (glibc 2.36, aarch64 and arm32v7) and
macOS (arm64). Three tests: the wrapper's own `MPI_Send` call (T1), the
*implementation's internal* `MPI_Send` call (T2, modelling ROMIO), and a
second later-`dlopen`ed plugin (T3, modelling mpi4py plus a second extension).

| mode | T1 | T2 | T3 |
|---|---|---|---|
| Linux `RTLD_LOCAL` | **CAPTURED** | **CAPTURED** | OK |
| Linux `RTLD_GLOBAL` | **CAPTURED** | **CAPTURED** | **BYPASSED** |
| Linux `RTLD_LOCAL \| RTLD_DEEPBIND` | OK | OK | OK |
| Linux `dlmopen(LM_ID_NEWLM)` | OK | OK | OK |
| macOS `RTLD_LOCAL` | OK | OK | OK |
| macOS `RTLD_LOCAL`, wrapper `-flat_namespace` | **CAPTURED** | OK | — |

`CAPTURED` = our `MPI_Send` re-entered, i.e. infinite recursion. `BYPASSED` = the
caller reached the native MPI without passing through the ABI layer and would be
handed ABI-typed arguments. 32-bit matched 64-bit exactly. Full traces, the
`LD_DEBUG=scopes` output showing scope 0 is the global scope, and the
`LD_DEBUG=bindings` line proving the T3 bypass are in that directory's README.

Four things the probe settled that reasoning had left open:

1. **`RTLD_DEEPBIND` applies transitively.** T2 passes, so it redirects the
   *implementation's own* internal `MPI_*` references and not merely the wrapper's,
   even though `libmpi` is a dependency loaded by the same call rather than the
   object named in it. This was the question that decided the Linux default.
2. **`RTLD_GLOBAL` really does promote dependencies** into the global scope, and T3
   demonstrates the consequence rather than predicting it.
3. **macOS is safe *because of* the two-level namespace**, confirmed by forcing
   `-flat_namespace` and watching T1 capture. So the assumption is load-bearing:
   a macOS build must never acquire `-flat_namespace` by accident.
4. **The load-time isolation check works** — it reported failure on the
   flat-namespace build before any call was made, and `dladdr` still resolves across
   a `dlmopen` namespace boundary on this glibc, so the check survives that mode too.

**Per platform, as measured:**

| | how | why |
|---|---|---|
| macOS | `RTLD_LOCAL` | the two-level namespace binds `libmpiwrapper`'s `MPI_Send` to `libmpi` at link time, so there is nothing to capture |
| Linux | `RTLD_LOCAL \| RTLD_DEEPBIND` by default, `dlmopen(LM_ID_NEWLM)` selectable | both measured sufficient; `DEEPBIND` is simpler and has no namespace limit |
| FreeBSD | `RTLD_LOCAL \| RTLD_DEEPBIND` | `dlmopen` does not exist |

Keep both Linux modes selectable at run time, as MPItrampoline does, because each
has known costs: `dlmopen` is semi-abandoned, caps namespaces at glibc's `DL_NNS`
(16), and gives the wrapper a separate libc; `RTLD_DEEPBIND` interferes with
`malloc` interposition and with sanitizers — which makes the sanitizer CI jobs the
concrete reason `dlmopen` has to stay available. Remember the namespace id from the
first load and reuse it, so the wrapper and its dependencies stay in one namespace.

**Binding mode defaults to `RTLD_LAZY`, not `RTLD_NOW`** — also a correction.
`RTLD_NOW` forces every undefined symbol in `libmpi` and its dependency closure to
resolve, and real MPI installations have symbols that are never called. Overridable.

**Check the outcome, not the mechanism.** `dlinfo(handle, RTLD_DI_LMID)` confirms
which namespace you got but not that every reference resolved the way the namespace
was meant to make it resolve. So `libmpi_abi` passes the address of one of its own
functions to `mpiwrapper_get_vtable`, and the wrapper `dladdr`s that together with
the `MPI_Send` it actually resolved, refusing if the two share a base object. That
catches the capture at load, positively, on every platform, whatever the loader did
— and it does not depend on knowing whether `RTLD_DEEPBIND` propagates to
dependencies. `examples/mpiwrapper_convert.c` implements it.

`size` is `sizeof(struct mpiwrapper_vtable)` as the *caller* understands it. A
wrapper may accept a smaller size than its own and serve the common prefix; it must
refuse a larger one, since the caller would read past the end.

**Naming, and the renaming rules for `mpiabi.h`.** `MPIABI_` uniformly
(`MPIABI_Comm`, `MPIABI_COMM_WORLD`). Not `MPI_ABI_`, because the stub header
already uses `MPI_ABI_Comm` as a *struct tag* and reusing it as a typedef name is
legal but confusing. Three rules, each of which earns its keep:

1. **Typedef names, macro names and enumerator names are prefixed.** That is what
   lets both views coexist in one translation unit, so a wrapper body can say
   `MPI_INT` for the implementation's datatype and `MPIABI_INT` for the ABI's on
   the same line.
2. **Struct tags are left alone.** `MPIABI_Comm` stays `struct MPI_ABI_Comm *`,
   which is *the same type* as the ABI header's own `MPI_Comm`. Renaming the tag
   would make them incompatible and force a cast in all 1376 forwarders on the ABI
   side — casts that would then silently absorb a genuine type error. This is why
   `examples/mpi_abi_side.c` forwards without a single cast.
3. **Struct member names are left alone.** Members live in a per-struct namespace,
   so `MPIABI_Status.MPI_SOURCE` and the implementation's `MPI_Status.MPI_SOURCE`
   cannot collide, and keeping them identical means the status conversion reads the
   way the standard describes it.

---

## 3. The generator

**688 entry points are written by a generator, in Python, with a named set of
roughly 50 written by hand.** Roughly 640 are mechanical translation; the rest
involve per-function judgement and are listed in §8.

### Why a generator

**The unit of change is the translation rule, not the function.** New MPI releases
being a year apart is the wrong clock. The clock that matters ticks daily for the
first months: how a handle is represented, how a status crosses the boundary,
whether error codes are mapped eagerly or lazily, how a request array is staged.
Each such decision must land identically at 600-1400 sites. As a generator edit
that is one line and a regeneration; as 688 explicit functions it is 688 edits,
repeatedly, and the sites a sweep misses are silently wrong. mpif records exactly
this at far smaller scale: a prefix change threaded through its generator still
missed `MPI_Cart_sub`, because a mechanical sweep is not a proof.

**Uniformity is the correctness property, and only generated code makes it
checkable.** The claim to establish is not "`MPI_Send` is right" 688 times. It is:
*every ABI handle argument is converted exactly once on the way in, every
out-handle exactly once on the way out, every sentinel is translated, and no
untranslated ABI value reaches the implementation.* Over generated text that is an
assertion the generator runs on its own output. Over 688 independently written
functions there is no such assertion, and the symptom of an omission is a wrong
answer at 4096 ranks, not a crash.

**There is no per-function judgement to spend on the mechanical 640.** Both inputs
are machine-readable:

- **The ABI `mpi.h`** is parseable by construction — the Forum's own `update.py`
  parses it with one regex. It gives all 688 signatures one per line, the 104
  predefined handle constants in `((MPI_Datatype)0x00000219)` form (which encodes
  class *and* value), the sentinels, the error classes and other integer constants
  as one-per-line anonymous enums, and the status layout.
- **`apis.json`** (vendored, ~2 MB, as mpif already does) gives the orthogonal half
  the header cannot: `param_direction`, which array is sized by which other
  argument, `root_only`, `constant`, `func_type` for callback parameters, and the
  `POLY*` prefix pairing each small form with its `_c` form. Critically, it also
  gives each parameter's *kind* — which is what makes a rank distinguishable from a
  tag (§5.4).

**Two compilers do the whole signature half of the verification, for free.** The
`libmpi_abi` side includes the ABI `mpi.h` only, so any signature the generator
gets wrong is a compile error. The `libmpiwrapper` side calls the implementation
directly, so the implementation's own declarations check every call site — which is
a stronger check than a typed function pointer, and it means no hand-maintained
function-signature table needs to exist. Every constant map is a generated `switch`
over the implementation's own macro names, so implementation-side values are never
transcribed by hand.

**The ledger.** The generator holds an explicit `HAND_WRITTEN` set and **fails if
any of the 688 entry points is neither generated nor in that set**. This is what
makes "nothing was silently dropped" a checked property rather than a hope.

### Argument classes

Every parameter falls into one of these. Site counts are deliberately omitted here:
the generator's frozen tallies are the authority, and a number written in prose
rots.

| class | what it emits |
|---|---|
| passthrough scalar (`int`, `MPI_Aint`, `MPI_Offset`, `MPI_Count`, `double`, `const char*`) | nothing |
| handle scalar, in | one conversion |
| handle scalar, out / inout | local of the implementation's type, then convert back |
| handle array, in | staged temporary |
| handle array, out / inout | staged temporary + write-back |
| status, scalar | §5.2 |
| status, array | §5.2, staged |
| choice buffer with sentinel (`MPI_BOTTOM`, `MPI_IN_PLACE`, `MPI_BUFFER_AUTOMATIC`) | one test |
| error code, return value | mapping call |
| error code, array (out) | in-place element mapping |
| rank | role-specific mapping (§5.4) |
| tag | role-specific mapping (§5.4) |
| other mapped integer constant (`MPI_ORDER_*`, `MPI_DISTRIBUTE_*`, `MPI_COMBINER_*`, `MPI_THREAD_*`, `MPI_TYPECLASS_*`, `MPI_LOCK_*`, `MPI_SEEK_*`) | `switch` |
| bitmask constant (`MPI_MODE_*`) | OR-decomposition (§5.5) |
| attribute keyval | mapping with a bias (§5.6) |
| callback (7 typedef families) | trampoline install (§6) |
| output string buffer with implementation-defined maximum | staged temporary, truncate or error (§5.8) |
| string array with sentinel (`MPI_ARGV_NULL`, `MPI_ARGVS_NULL`) | null test |
| weights array with sentinel (`MPI_UNWEIGHTED`, `MPI_WEIGHTS_EMPTY`) | one test |
| attribute value / extra state (`void*`) | passthrough |
| `MPI_T` handle classes | conversion |
| varargs (`MPI_Pcontrol` only) | hand-written |

Per-function and per-parameter exceptions live in **named, prose-commented tables
keyed on `(routine, parameter)`** — never as `if name == "MPI_Foo"` scattered
through the generator body. mpif has only 8 such ad-hoc tests in 3346 lines, and
that is why its special-casing stays auditable.

### Naming convention in generated code

ABI-side names carry an `abi_` prefix; implementation-side names are bare:

```c
int MPI_Send(const void *abi_buf, int abi_count, MPIABI_Datatype abi_datatype,
             int abi_dest, int abi_tag, MPIABI_Comm abi_comm)
{
  const void *buf              = buffer_fromabi(abi_buf);
  const int count              = count_fromabi(abi_count);
  const MPI_Datatype datatype  = datatype_fromabi(abi_datatype);
  const int dest               = rank_fromabi(abi_dest);
  const int tag                = tag_fromabi(abi_tag);
  const MPI_Comm comm          = comm_fromabi(abi_comm);
  const int ierror = MPI_Send(buf, count, datatype, dest, tag, comm);
  return errorcode_toabi(ierror);
}
```

This is not cosmetic. It makes the load-bearing generator assertion a grep:
**no parameter of an ABI-typed signature may appear in the argument list of the
implementation call** — only locally declared converted values may. If the
generator emits `MPI_Send(abi_buf, ...)`, that is a hard stop.

`examples/` carries a compiling worked version of each shape, and
`examples/check.sh` compiles them — the generator is required to reproduce them.

### One portability trap in generated switches

Case labels must be **numeric, with the symbolic name in a comment**:

```c
case 0x00000209: return MPI_INT;  /* MPIABI_INT */
```

not `case (uintptr_t)MPIABI_INT:`. `MPIABI_INT` expands to
`((MPIABI_Datatype)0x00000209)`, and casting an integer constant to a pointer type
and back is not an integer constant expression in standard C — gcc and clang accept
it, but a case label is exactly where that extension is not worth relying on. The
generator parsed the header, so it has the numeric value and transcribes nothing
either way.

### Four disciplines lifted from mpif

- **Committed output, never hand-edited.** A bug goes back into the generator.
- **An on/off switch per axis**, so a refactor that should change nothing is
  *shown* to change nothing by regenerating to an empty diff.
- **Frozen tallies**, so a new `apis.json` or a new ABI header reclassifies loudly
  rather than silently.
- **Post-hoc assertions over the emitted text**, as above.

Language is Python, not Julia: none of mpif's Fortran-descriptor machinery
transfers, and a C project's contributors and CI already have Python. Use
`pympistandard` as a dev-time cross-check only, never a build dependency — it is
lightly maintained, has no tags, and its `LICENSE` file is empty (MIT is declared
in `pyproject.toml` alone). Vendor `apis.json`, which is what it would provide
anyway.

### Alternatives rejected

- **A model writing each of the 688 by hand.** The artifact is ~40k lines nobody
  will re-read, every cross-cutting change is a 688-site edit, and the uniformity
  claim becomes unverifiable. The initial writing is not the cost; the second month
  is.
- **Porting mpif's `dev/mpiapi.jl`.** Its transferable content is ~200 lines of
  tables and its discipline; its 1150-line dispatch is Fortran-specific.
- **A hand-written per-function spec file** (wi4mpi's `functions.json`, 328 KB).
  Duplicates what `apis.json` already knows and must be re-maintained per MPI
  version.
- **X-macros / preprocessor wrappers.** Macros cannot do type-directed
  per-argument dispatch over varying arity without an X-macro list that *is* the
  table a generator would emit, with worse diagnostics.

### Generated artifacts

| artifact | includes | checked by |
|---|---|---|
| `gen/include/mpi.h` | — | stub header plus `doc/mpi.h.patch`, names untouched |
| `gen/include/mpiabi.h` | — | renamed view, prototypes dropped |
| `gen/include/mpiwrapper_vtable.h` | `mpiabi.h` | shared by both halves, carries the layout hash |
| `gen/mpi_abi/entrypoints.c` | ABI `mpi.h` | compiles against the real header; `nm` vs its symbol set both ways |
| `gen/mpiwrapper/wrappers.c` | impl `mpi.h` + `mpiabi.h` | compiles against the implementation's header |
| `gen/mpiwrapper/constants.c` | both | every `case` names a real implementation macro |
| `gen/report.txt` | — | hand-written ledger, frozen tallies, unsupported list |

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

Predefined handle constants occupy `0x00000020`..`0x000002eb` — 104 values, all
< 748.

### 4.2 Status layouts

| | layout | sizeof | private bytes |
|---|---|---|---|
| ABI | `int MPI_SOURCE, MPI_TAG, MPI_ERROR; int MPI_internal[5]` | 32 | **20** |
| MPICH | `int count_lo, count_hi_and_cancelled, MPI_SOURCE, MPI_TAG, MPI_ERROR` | 20 | 8, at the front |
| Open MPI | `int MPI_SOURCE, MPI_TAG, MPI_ERROR, _cancelled; size_t _ucount` | 24 | 12, at the back |

Both fit the ABI's 20 scratch bytes with room to spare. (Open MPI's 24 is its
*total* size; only 12 of it is private. A design that assumed otherwise would have
rejected the simple scheme for no reason.)

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
| `MPI_IN_PLACE` | `(void*)1` | `(void*)-1` | `(void*)1` |
| `MPI_BOTTOM` | `(void*)0` | `(void*)0` | `(void*)0` |
| `MPI_BUFFER_AUTOMATIC` | `(void*)2` | MPI-4.1 | MPI-4.1 |
| `MPI_STATUS_IGNORE` | `(MPI_Status*)0` | `(MPI_Status*)1` | `(MPI_Status*)0` |
| `MPI_STATUSES_IGNORE` | `(MPI_Status*)0` | `(MPI_Status*)1` | `(MPI_Status*)0` |
| `MPI_UNWEIGHTED` | `(int*)10` | — | `(int*)2` |
| `MPI_WEIGHTS_EMPTY` | `(int*)11` | — | `(int*)3` |
| `MPI_ARGV_NULL`, `MPI_ARGVS_NULL`, `MPI_ERRCODES_IGNORE` | NULL | NULL | NULL |
| `MPI_BSEND_OVERHEAD` | 512 | 96 | 128 |

`MPI_STATUS_IGNORE` and `MPI_STATUSES_IGNORE` are *the same value* in the ABI, so
they cannot be told apart — which is fine, since arity is known per call site. But
NULL must be mapped to MPICH's `(MPI_Status*)1`.

### 4.4 `MPI_MAX_*` and the Fortran status size

The ABI took the maximum over MPICH and Open MPI for every `MPI_MAX_*` (the stub
header documents both values in a comment beside each), so neither implementation
can overflow a buffer the application sized with an ABI constant. That is a
property of those two, not of the ABI.

| | `MPI_F_STATUS_SIZE` | index of SOURCE, TAG, ERROR |
|---|---|---|
| ABI | 8 | 0, 1, 2 |
| MPICH | 5 | 2, 3, 4 |
| Open MPI | `OMPI_FORTRAN_STATUS_SIZE` (6) | 0, 1, 2 |

8 ints = 32 bytes = exactly the ABI C status, with the named fields at the same
indices. **The ABI Fortran status is the ABI C status**, which is what
`doc/mpi.h.patch`'s `typedef MPI_Status MPI_F08_Status` assumes. Against MPICH it
is not: the named fields move. So the four status converters are real conversions
and go through the C status as intermediary. 8 >= 5 and 8 >= 6, so an mpif status
buffer sized by the ABI constant is never too small.

---

## 5. Conversion rules

### 5.1 Handles

ABI -> implementation is a dense `switch` over `0x20`..`0x2eb` (which the compiler
turns into a jump table), else a bit-cast.

Implementation -> ABI needs the reverse: predefined implementation handle values are
*not* compile-time constants in general (Open MPI's are addresses), so the map is
built at initialization inside `mpiwrapper_get_vtable` — a small open-addressing
hash keyed on `uintptr_t`, ~256 slots, one probe typical. A linear scan over 104
predefined handles is too slow for the datatype case, which is the hot one.

**Collision.** A bit-cast dynamic implementation handle is wrong if it lands in
`0x20`..`0x2eb`. It never does today: MPICH's handles carry a kind field in the
high bits so all real handles are >= 0x04000000, and Open MPI's are object
addresses. But cross-compiling forbids probing this at configure time, and 32-bit
targets have no spare high bits for a tagging scheme. So: **check in the `toabi`
direction only** — that is object creation, not every `MPI_Send` — and fail with
`MPI_ERR_INTERN`. A test also probes it at run time (§10).

### 5.2 Status

Copy the three named fields; `memcpy` the implementation's private bytes into all
20 bytes of `MPI_internal`.

The private bytes are the complement of the named-field block: at the *front* in
MPICH, at the *back* in Open MPI. A head range and a tail range of
`offsetof`/`sizeof`-derived constant length cover both, and one of the two is empty
in each, so it compiles to a single 8-byte copy for MPICH and a single 12-byte copy
for Open MPI. `_Static_assert` the contiguity of the three named fields and
`sizeof(impl status) - 12 <= 20`; an implementation that interleaves private bytes
between the named fields fails the build rather than being silently mishandled.

`_ucount` is `size_t`-aligned while `MPI_internal` is `int`-aligned, so convert
through an aligned local and `memcpy`. Zero the unused private bytes on the way
out, and zero the whole implementation status before filling it on the way back —
not for correctness, but so ABI statuses are bit-reproducible and implementation
stack garbage never reaches the user, which matters when this is debugged under
valgrind or MSan.

**No validity marker and no synthesis fallback.** An earlier design added a magic
word in `MPI_internal[0]` plus reconstruction via `MPI_Status_set_elements_x` for
statuses the implementation had never touched. Both are unnecessary:

- In the generalized-request flow the blob is always valid. The implementation
  calls our query trampoline with a status it owns; we capture its blob into the
  ABI status; the user's query function calls our `MPI_Status_set_elements`, which
  restores that same blob before forwarding.
- For a genuinely uninitialized status, garbage out is the *correct* behaviour — it
  is what the native implementation does for the same user error, and this shim
  owes no more safety than what it wraps. Neither implementation dereferences
  anything from the private bytes, and MPI already requires statuses to be freely
  copyable by the user, which forces those bytes to be position-independent and
  self-contained. That requirement is what makes the scheme sound in general, not
  just for these two.

So all 20 bytes hold the blob, there is no second code path, and
`MPI_Status_set_elements_x` is never called except when forwarding it.

**Only ten functions need an implementation status built from an ABI status:**
`MPI_Get_count`, `_count_c`, `MPI_Get_elements`, `_c`, `_x`,
`MPI_Test_cancelled`, `MPI_Status_set_cancelled`, and the three
`MPI_Status_set_elements*`. The rest are pure ABI-side:
`MPI_Status_get_source`/`_tag`/`_error` and `MPI_Status_set_source`/`_tag`/`_error`
touch only named fields, and all four Fortran converters are memcpy-shaped.

### 5.3 Sentinels

Pointer values with special meaning (`MPI_BOTTOM`, `MPI_IN_PLACE`,
`MPI_BUFFER_AUTOMATIC`, `MPI_UNWEIGHTED`, `MPI_WEIGHTS_EMPTY`,
`MPI_STATUS(ES)_IGNORE`, `MPI_ARGV(S)_NULL`, `MPI_ERRCODES_IGNORE`) are fixed in
the ABI and may be non-constant in the implementation — possibly `extern void *`,
i.e. constant at link time but not at build time. Translated the same way as
handles: one test per site.

### 5.4 Ranks and tags are different classes

In the ABI, `MPI_ANY_TAG` is -2 and `MPI_PROC_NULL` is -3; in MPICH both
`MPI_ANY_TAG` and `MPI_PROC_NULL` are -1. So **an `int` cannot be translated
without knowing whether it is a rank or a tag**, and `rank_fromabi` and
`tag_fromabi` must be separate functions driven by the parameter's kind in
`apis.json`. This is load-bearing, not decorative — it is why `apis.json` is
required and the header alone is insufficient.

The same applies in the out direction: `status.MPI_SOURCE` can be `MPI_PROC_NULL`
or `MPI_ANY_SOURCE`, and `MPI_Group_translate_ranks` can return `MPI_UNDEFINED`.

### 5.5 Bitmasks

`MPI_MODE_*` are OR-combined and the bit assignments differ completely (ABI
`RDONLY` = 16 against 2 in both implementations; `NOCHECK` 1024 against Open MPI's
1). These need OR-decomposition, not a `switch`. Sites: `MPI_File_open`'s `amode`,
and the `assert` argument of `MPI_Win_post`/`_start`/`_fence`/`_lock`. The ABI puts
file modes (1..512) and window asserts (1024..16384) in one enum with disjoint
bits, so a single bitmask mapper serves both roles.

### 5.6 Keyvals and dynamic error codes

Both are plain `int`s handed out by the implementation at run time, and both can
collide with ABI predefined values.

- **Keyvals.** ABI predefined values are at 501-504 and 601+; Open MPI hands out
  small sequential ints, which could in principle reach 501. No pointer slack is
  available, so dynamic keyvals need an additive bias or a high-bit tag.
- **Error codes.** `MPI_ERR_LASTCODE` is 16383 in the ABI against MPICH's
  0x3fffffff, so codes from `MPI_Add_error_class`/`_code` must be *renumbered* into
  the ABI's range above its last predefined class, not passed through.
  Bidirectional table, atomic append, capped at 16383.

Generalized requests and datarep names need the same treatment.

### 5.7 Arrays: always temporaries, never in place

An earlier design contemplated rewriting array arguments in place when the standard
allows it and there is space. It does not survive contact with the standard.

- §6.12 (nonblocking collectives): "Once initiated, all associated send buffers and
  buffers associated with input arguments (such as arrays of counts, displacements,
  or datatypes in the vector versions of the collectives) should not be modified,
  and all associated receive buffers should not be accessed, until the collective
  operation completes."
- §6.13 (persistent collectives): "After initialization, all arrays associated with
  input arguments ... must not be modified until the corresponding persistent
  request is freed with MPI_REQUEST_FREE."

Both forbid the *user* from modifying. **Neither forbids the user from reading.**
So an application may legally read its own datatype array while a nonblocking
collective is in flight, and in-place conversion would show it implementation
handles.

Four independent reasons, each sufficient:

1. **`const` may mean physically read-only.** `MPI_Type_create_struct`,
   `MPI_Alltoallw` and friends take `const MPI_Datatype array_of_types[]`. An
   application's `static const MPI_Datatype types[3] = {MPI_INT, MPI_DOUBLE,
   MPI_CHAR};` lives in `.rodata`; writing to it crashes a legal program.
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
- **Persistent:** freed at `MPI_Request_free`, **not** at completion — the request
  is re-armed by `MPI_Start` repeatedly, so freeing at completion is a
  use-after-free on the second `MPI_Start`. `MPI_Waitall` can mix both kinds, so
  the request-map entry needs a flag.
- **`MPI_STATUSES_IGNORE`** (NULL in the ABI) must short-circuit before any
  temporary is allocated.

The set needing staging is small: datatype arrays, request arrays, status arrays,
and the out-direction errcode/rank arrays — roughly 35-40 of the 688.

**The one legitimate in-place case:** OUT arrays whose element type is the same
size and which need only value mapping — `array_of_errcodes` in `MPI_Comm_spawn`,
`ranks2` in `MPI_Group_translate_ranks`. Let the implementation write into the
user's array, then map each element in place. No `const`, no concurrent-read
expectation, no packing, no restore path. `array_of_indices` in `MPI_Waitsome`
needs no mapping at all.

**Rejected although sound:** expanding status arrays in place. The
implementation's 20/24-byte statuses fit in the front of the user's 32-byte array
and expand backwards without overlap (destination of element `i` starts at `32i`;
source of element `i-1` ends at `24i`). It saves one allocation and costs a
non-obvious invariant.

**The same bug class on scalars:** any out or inout handle needs a local of the
implementation's type; never reinterpret the ABI storage.
`MPI_Comm_free(MPI_Comm *comm)` forwarding `(MPI_Comm *)abi_comm` writes MPICH's
4-byte handle into an 8-byte ABI slot and leaves the upper half garbage — while on
Open MPI it works by accident, so it would pass tests on one implementation and
corrupt on the other. No `_Static_assert` catches this; the "no ABI-typed parameter
reaches the implementation call" assertion does.

### 5.8 Output string buffers

Ten functions have an output string buffer and **no explicit length argument**, so
an implementation whose `MPI_MAX_*` exceeds the ABI's would write past the caller's
array: `MPI_Error_string`, `MPI_Get_library_version`, `MPI_Get_processor_name`,
`MPI_Comm_get_name`, `MPI_Type_get_name`, `MPI_Win_get_name`,
`MPI_Info_get_nthkey`, `MPI_Open_port`, `MPI_Lookup_name`, and
`MPI_File_get_view`'s `datarep`.

Everything else is safe because the caller passes the size: `MPI_Info_get_string`
has `buflen`, `MPI_Session_get_nth_pset` has `pset_len`, deprecated `MPI_Info_get`
has `valuelen`. `MPI_MAX_STRINGTAG_LEN` is an input limit only.

A `_Static_assert` on `MPI_MAX_*` was considered and **rejected**: both limits are
known at the call site, so the situation is handleable, and a build failure would
block porting to a new MPI over a contingency that may never fire for the functions
that user needs.

**Truncate or error is a per-parameter judgement**, and belongs in the named
`(routine, parameter)` table:

- **Prose -> truncate silently**, which is what an implementation does with its own
  too-short buffer anyway: `MPI_Error_string`, `MPI_Get_library_version`, and the
  three `*_get_name`.
- **Identifiers fed back into MPI -> return an error**, because a truncated one
  fails mysteriously much later: `MPI_Open_port` and `MPI_Lookup_name` (a truncated
  port name fails at connect), `MPI_Info_get_nthkey` (used to look up a value),
  `MPI_File_get_view`'s `datarep`.
- `MPI_Get_processor_name` is the awkward one: it reads as prose, but applications
  use it for rank-to-node mapping, where truncation can make two nodes
  indistinguishable — a silently wrong answer. Truncate anyway, since MPI already
  permits implementations to truncate to their own maximum, but the table entry
  needs a comment explaining the call.

**Always stage; never `#if` the staging.** Conditional on `impl_max > ABI_max`, the
temporary never compiles on MPICH or Open MPI, so the first implementation that
needs it runs code nobody has executed. All ten are cold paths, so staging
unconditionally costs nothing and keeps one code shape exercised on every run. Set
`*resultlen` to what was actually copied, so `string[*resultlen] == '\0'` holds. On
the error paths return `MPI_ERR_INTERN` (the limitation is ours, not the caller's)
*and* write the truncated NUL-terminated string anyway, so a caller that ignores
the return code does not read uninitialized memory.

The opposite direction is not fixable and is correct to pass through: where the
implementation's limit is *smaller* than the ABI's (Open MPI's `MPI_MAX_INFO_KEY`
is 36 against the ABI's 256), a long key is rejected by the implementation with
`MPI_ERR_INFO_KEY`, which we map and return.

### 5.9 When to assert at compile time

**Static-assert where a runtime check would cost something on a hot path; handle at
run time where the check is free.**

| | why |
|---|---|
| `MPI_MAX_*` | cold paths only -> run time |
| `sizeof(MPI_Count)`/`MPI_Aint`/`MPI_Offset` | a narrowing check would land on `MPI_Send_c` and every large-count call -> `_Static_assert` |
| status layout | **no runtime recourse exists** — nowhere to put a private part exceeding 20 bytes, and side storage keyed on a status address is unsound because statuses are freely copied -> build failure |
| dynamic handle collision | one compare, and only on object creation -> run time |

---

## 6. Callbacks

### 6.1 Which need trampoline pools

Seven typedef families, 16 registration functions. The ones with an extra-state
argument can carry a heap-allocated `{user_fn, user_extra}` pair; the ones without
need a pool of generated static trampolines, each knowing its own index.

| registrar | mechanism |
|---|---|
| `MPI_Op_create`, `_c` | **pool** (2, one per `MPI_User_function` variant) |
| `MPI_Comm`/`File`/`Win`/`Session_create_errhandler` | **pool** (4) |
| `MPI_Comm`/`Type`/`Win_create_keyval`, `MPI_Keyval_create` | extra state |
| `MPI_Grequest_start` | extra state |
| `MPI_Register_datarep`, `_c` | extra state |
| `MPI_T_event_register_callback` | registration-keyed map |
| `MPI_T_event_set_dropped_handler` | registration-keyed map |

Error handlers are the non-obvious pool case: `MPI_Comm_errhandler_function` and
its three siblings have no extra-state argument, so `MPI_Op_create` is *not* the
only exception.

`MPI_T` events need a map rather than a pool: `MPI_T_event_set_dropped_handler` has
no `user_data` parameter even though `MPI_T_event_dropped_cb_function` takes one,
so state cannot be smuggled through the registration — but both callbacks receive
`event_registration`, a handle we convert anyway, so one map keyed on the
implementation's registration handle serves both.

A user reduction trampoline receives an implementation datatype and must convert it
back to an ABI datatype, so it needs the reverse predefined-handle map (§5.1).

Errhandler trampolines must be **declared variadic**, matching
`MPI_Comm_errhandler_function`'s `...`. Nothing needs forwarding — the extra
arguments are implementation-specific and the user's ABI-side function is variadic
too — but variadic and non-variadic calling conventions differ on arm64 macOS, so a
non-variadic declaration would be a silent ABI bug. Taking the type from the
implementation's own typedef gets this right for free. A useful related property: an
errhandler trampoline runs when the process is already in trouble, and a pool
lookup allocates nothing.

### 6.2 Lifetimes: almost nothing can be reclaimed

MPI-5.0 §2.5.2 governs: "A call to a deallocate routine invalidates the handle and
marks the object for deallocation... MPI need not deallocate the object
immediately. Any operation pending (at the time of the deallocate) and decoupled
MPI activity (see Section 2.9) that involves this object will complete normally;
the object will be deallocated afterwards."

**So a free call is not a reclamation point.** Checked against the standard for all
16:

| registrar | safe reclamation point |
|---|---|
| `MPI_Op_create`, `_c` | **none observable** (§2.5.2) |
| `*_create_errhandler` | **none**: §9.4 "deallocated after all the objects associated with it (communicator, window, or file) have been deallocated" |
| `*_create_keyval` | **none before finalize**: §7.7 "not erroneous to free an attribute key that is in use, because the actual free does not transpire until after all references ... have been freed" |
| `MPI_Grequest_start` | **our `free_fn` trampoline** — invoked exactly once, object deallocated after it returns |
| `MPI_Register_datarep`, `_c` | **never** — MPI has no deregistration call |
| `MPI_T_event_register_callback` | **the free callback**: §15.3 invoked "when it is able to guarantee that no further event instances ... will be raised" |
| `MPI_T_event_set_dropped_handler` | same |

Consequences:

- **Op slots are not reclaimed at `MPI_Op_free`.** Precise reclamation would need a
  per-slot refcount incremented by every operation that can invoke the op and
  decremented on completion: the six reduction families in blocking, nonblocking
  and persistent form (x2 for `_c`), ~36 functions, plus `MPI_Start`/`MPI_Startall`
  re-arming a persistent reduction. RMA is *excluded* — §12.3.4 says of
  `MPI_Accumulate`'s `op` that "user-defined functions cannot be used".
  **v1 does not reclaim.** 1024 trampolines per variant is ~24 KB of text;
  exhaustion after 1024 op creations over the process lifetime returns a clean
  error naming the tunable. Not reclaiming is trivially safe, whereas a refcount
  bug is a use-after-free surfacing as a wrong reduction result at scale.
- **Errhandler slots are permanent.** The association is with communicators,
  windows, files and sessions, inherited through `MPI_Comm_dup` and surviving to
  `MPI_Finalize`; tracking it means reimplementing the implementation's
  refcounting. 256 slots per class.
- **Keyval pairs are never freed.** Not even at finalize: the implementation invokes
  attribute delete callbacks on `MPI_COMM_SELF` from inside `MPI_Finalize`
  (§11.4.1), so the earliest safe point is after the implementation's
  `MPI_Finalize` returns, which is worthless when the process is exiting.
- **Generalized requests are the one clean case:** free the pair inside our
  `free_fn` trampoline after calling the user's, unconditionally — MPI deallocates
  even if the user's `free_fn` returns an error.

### 6.3 Concurrency

All shared tables are fixed-capacity and lock-free: CAS slot allocation for the
trampoline pools, atomic append for the keyval / error-code / datarep maps,
open-addressing CAS insert for the staged-request hash, release/acquire on the
vtable pointer. No mutex anywhere. Overflow is a documented limit returning
`MPI_ERR_INTERN`.

Staged temporaries that must outlive their call (the `MPI_Ialltoallw` family and
the persistent `_init` forms) live in the request-keyed hash, guarded by a global
atomic count so that completion calls pay one relaxed load and a compare against
zero when the application never uses those routines.

---

## 7. Decisions

1. **Conversions live in `mpiwrapper`, behind an ABI-typed vtable.** §2.
2. **Status: blob only** — no validity marker, no synthesis fallback. §5.2.
3. **Minimum implementation is MPI-4.0.** §1.
4. **`mpiwrapper` exports exactly one symbol**, a getter carrying
   `MPI_ABI_VERSION` and a generated layout hash. §2.
5. **`mpi_abi` finds the wrapper from an environment variable**, falling back to a
   build-time path. §2.
6. **Functions the implementation lacks return `MPI_ERR_UNSUPPORTED_OPERATION`**
   from generated `#ifdef` stubs, and the generator reports them.
7. **PMPI needs no vtable slots**; two definitions rather than a weak alias. §2.
8. **Bootstrap by constructor into a plain pointer** — no atomic, no lazy-init
   branch, no NULL check outside debug builds. The wrapper is loaded `RTLD_LOCAL`
   and *isolated* — `RTLD_DEEPBIND` or `dlmopen` on Linux, the two-level namespace
   on macOS — never `RTLD_GLOBAL`, and `RTLD_LAZY` by default. The wrapper then
   proves at load that its `MPI_*` calls resolved outward. §2, §2a.
9. **Naming: `MPIABI_` uniformly** for the renamed view. §2.
10. **Staged temporaries outliving a call** go in a request-keyed hash behind a
    global atomic count. §6.3.
11. **All shared tables fixed-capacity and lock-free.** Op and errhandler slots are
    process-lifetime; only generalized-request and `MPI_T`-event state is
    reclaimed. §6.2, §6.3.
12. **One generated file per artifact**, seven in all. §3.
13. **No in-place argument conversion**, except same-size OUT arrays needing only
    value mapping. §5.7.
14. **`MPI_MAX_*` mismatches are handled at run time, not asserted.** §5.8.
15. **Static-assert only where a runtime check would cost something hot.** §5.9.
16. **Shared libraries only in v1.** §9.
17. **Prototype fifteen entry points before writing the generator.** §11.

---

## 8. The hand-written set

Roughly 50 functions where per-function judgement is needed. The generator's
`HAND_WRITTEN` ledger names them and fails if the two sets do not together cover
all 688.

- **Bootstrap and lifecycle:** `MPI_Init`, `MPI_Init_thread`, `MPI_Finalize`,
  `MPI_Abort`, `MPI_Initialized`, `MPI_Finalized`, `MPI_Session_init`/`_finalize`.
- **No error code to map:** `MPI_Wtime`, `MPI_Wtick` return `double`.
- **The ten status-consuming functions** of §5.2, plus the four Fortran status
  converters.
- **Callback registration** — the 16 of §6.1, each installing a trampoline or a
  pair.
- **Genuinely variadic:** `MPI_Pcontrol`.
- **Dynamic error codes:** `MPI_Add_error_class`, `_code`, `_string`.
- **Spawn:** `MPI_Comm_spawn`, `_multiple` (`argv`, `array_of_argv`,
  `array_of_errcodes`).
- **Buffers:** `MPI_Buffer_attach`, `_detach`, and the `MPI_Comm_`/`Session_`
  variants, including `MPI_BUFFER_AUTOMATIC` under an MPI-4.0 minimum.
- **Staged temporaries outliving the call:** the `MPI_Ialltoallw` family and the
  persistent `_init` forms.
- **The 22 Fortran handle converters**, which are the reason mpif can run over any
  MPI.
- **`MPI_File_get_view`** (`datarep` truncation) and the other nine output-string
  functions of §5.8.

---

## 9. Building

### Repository layout

```
dev/               the Python generator and dev-time cross-checks
                     apis.json (vendored), check-c-bindings.py (Appendix A.2)
gen/               committed generated output, never hand-edited
src/mpi_abi/       hand-written: bootstrap, dlopen, vtable acquisition
src/mpiwrapper/    hand-written: the ~50, trampolines, maps, status conversion
test/              our own tests
ci-scripts/        MPI install and build-shape checks
ci-scripts/suite/  MPICH C suite runner, xfail list, mpiexec filter
scripts/           the same recipes locally
doc/               mpi.h.patch, mpi50-report.pdf
```

The `ci-scripts/` versus `ci-scripts/suite/` split is worth copying deliberately.
mpif's own `ci-scripts/README.md` records getting the cache key wrong *twice*: the
MPI-install cache must hash the install scripts and must **not** hash the suite's
expected-failure list, or every edit to a reason rebuilds MPI on every variant.
Note also that `ci-scripts/*` does not mean what it looks like in `@actions/glob` —
a matched directory expands to all of its descendants.

### CMake

**One project, two independently configurable targets.** `libmpi_abi` needs no MPI;
`libmpiwrapper` needs `find_package(MPI)`. A standalone wrapper build against a new
MPI consumes the installed `mpiwrapper_vtable.h` via `find_package(mpi_abi)`, so
the layout hash necessarily matches the `libmpi_abi` that will load it.

**Four configure-time checks for the wrapper, all compile-only** so
cross-compiling works:

1. `MPI_VERSION >= 4`.
2. **No self-wrapping.** Hard error if the found MPI prefix is *our own*
   installation, detected by the presence of `mpiwrapper_vtable.h` — a file only we
   install. Neither `MPI_ABI_VERSION` nor the library name discriminates: §20.2.1
   requires that a library implementing the standard ABI "must be named mpi_abi",
   so a genuine ABI-implementing MPICH installs `libmpi_abi` too. The accident is
   easy to hit, since `find_package(MPI)` is looking for exactly the `mpi.h` and
   `libmpi_abi` we install, and its symptom is a startup loop rather than a
   diagnostic. `mpi.h` itself stays pure — the stub header plus `doc/mpi.h.patch`,
   with no marker macro. Wrapping a *genuine* ABI MPI is permitted behind an
   explicit flag, with a warning by default; see oracle 5.
3. The `_Static_assert` battery of §5.9.
4. Every generated constant `case` naming a real implementation macro (free — it is
   a compile error).

**Generated code stays committed**, with a `regenerate` target outside `all` and a
CI job asserting it produces an empty diff. Python is a dev dependency, never a
build dependency.

**Symbol visibility:** `-fvisibility=hidden` plus an explicit export macro, a
version script on ELF and `-exported_symbols_list` on macOS. `libmpi_abi` exports
only `MPI_*`/`PMPI_*`; `libmpiwrapper` only `mpiwrapper_get_vtable`. Worth doing
deliberately because `RTLD_GLOBAL` puts `libmpi`'s own `MPI_Send` into the global
namespace — unavoidable, but our internals should not join it.

**Shared only in v1.** Static linking would require splitting `entrypoints.c` into
688 translation units, because MPI-5.0 §15.2.1(2) requires that "those MPI
functions that are not replaced may still be linked into an executable image
without causing name clashes" — for an archive that means one entry point per
member (mpif's `split-wrappers.sh` is the precedent; for a shared library ordinary
interposition satisfies it). `dlopen` is central to this design anyway, so a static
`libmpi_abi` is an odd configuration. The split would be a build step rather than a
generator output, so decision 12 survives if this is revisited.

### Provisioning MPI in CI

Pinned released tarballs, built from source and cached. This is much simpler than
mpif's equivalent, because mpif needs an MPI that *already* implements the ABI —
hence its MPICH-from-`main` builds, its substitution of the Forum's `mpi.h` over
the implementation's own, and its pruning of everything the ABI does not define.
None of that applies here: stock configure, no pruning, no header substitution.

One constraint on version choice: **Open MPI 4.1 fails the MPI-4.0 minimum** (no
`_c` variants), so distro LTS packages do not serve. MPICH >= 4.0, Open MPI >= 5.0.

---

## 10. Testing

### Five oracles

1. **The ABI header, by compilation — no MPI, no launcher, seconds.** Wrong
   signatures are build errors; `nm` on `libmpi_abi` against the 1376-symbol list
   extracted from the header, **in both directions**; and nothing else exported. A
   total completeness check as the cheapest job in CI.
2. **The implementation's header, by compilation**, plus `nm` asserting
   `libmpiwrapper` exports exactly one symbol.
3. **The generator's assertions on its own output** — every handle and sentinel
   translated exactly once, no ABI-typed parameter in an implementation call
   argument list, frozen tallies, unknown kind a hard stop — plus the empty-diff
   regeneration.
4. **MPI-5.0 Appendix A.2 via `pdftotext -layout`** on `doc/mpi50-report.pdf`, as
   an independent route from the same LaTeX that produced `apis.json`. Keep mpif's
   two properties from `dev/check-f08-bindings.jl`: the parse validates itself
   (every argument declared exactly once, else the text was misread and no
   comparison is trustworthy), and exemptions are named, explained, and fail the
   run when they stop firing.
5. **The identity configuration: wrap an MPI that already implements the ABI.**
   Every conversion becomes an identity — predefined handle values, error codes,
   sentinels, `MPI_MAX_*`, ranks and tags all match — so the conversion tables are
   neutralized and any difference from native pass-through is a bug in the
   *plumbing*: vtable handshake, bootstrap, staged temporaries, trampoline pools,
   lifetime rules. This isolates the half of the system that is otherwise hardest
   to attribute, since a wrong reduction result does not say whether the datatype
   map or the temporary was at fault. It also lands the status assertion exactly on
   its boundary: an ABI implementation's status is 32 bytes with the named fields at
   0/4/8, so its private part is 20 bytes and fits with nothing to spare. mpif's
   `build/mpi/*` already holds ABI-capable MPICH and Open MPI prefixes. It does
   *not* test the conversion tables, where most of the risk lives.

### Behavioural tests, in increasing cost

- **`mpiwrapper_selftest`** — in-process, single rank, no launcher: every constant
  map round-trip, all 104 predefined handles in both directions (mpif's
  `test/predefined_types_c.c` is the model), and the **dynamic-handle collision
  probe** — create many objects of each class and assert none bit-casts into
  `[0x20, 0x2ec)`. That probe is specifically the runtime replacement for the
  configure-time test cross-compiling forbids.
- **Status round-trips** — through arrays, `MPI_STATUS_IGNORE`, and a generalized
  request.
- **MPICH's C test suite** against both implementations, with a per-variant
  expected-failure list carrying reasons. Expect some expected failures to be
  *build* failures where a test reaches for MPICH internals or `MPIX_*`.
- **The cross test, the headline property:** one `libmpi_abi`, one test binary, run
  against an MPICH wrapper and an Open MPI wrapper by changing only the environment
  variable. mpif's `cross` stage rebuilds against each implementation; ours
  rebuilds nothing, which is a strictly stronger claim. The job needs both MPI
  prefixes restored at once. Also `nm` the test binary to assert `libmpi_abi` is
  its only MPI dependency, per §20.2.1.
- **Threads** — `MPI_THREAD_MULTIPLE` concurrency over the trampoline pools and the
  maps.
- **Sanitizers and valgrind.** The suppression file is part of the design record:
  op slots, errhandler slots, keyval pairs and datarep state are leaks *by design*,
  so an unexplained entry means the design changed. Expect the first ASan run over
  Open MPI to be mostly triage of noise that has nothing to do with this code.

### Matrix

MPICH >= 4.0 and Open MPI >= 5.0; gcc and clang; Linux and macOS required, FreeBSD
via a VM on a Linux runner (mpif's precedent), Windows/mingw later.

**32-bit is load-bearing, not routine coverage.** ABI handles are pointer-sized, so
i386/arm32v7 is the only place the "no spare high bits for tagging" constraint of
§4.1 is visible. mpif already has Docker images for both.

### Gating

Our own tests, the MPICH C suite, the cross test, and the sanitizer/valgrind runs.

**mpif's own `test/` is deliberately not gating**, so the two projects' CI do not
become coupled. It is still the end-to-end composition proof and the only thing
that exercises the Fortran converters and the status `f2c`/`c2f` paths, so it
should be run before releases.

---

## 11. Sequencing

**Prototype before generator.** Hand-write fifteen representative entry points
end-to-end, all crossing the vtable boundary, and get them passing against one MPI.
Only then write the generator, and require it to reproduce those fifteen.
Designing the generator before the shape of its output is known is the main way
this goes wrong.

| function | what it forces |
|---|---|
| `MPI_Init`, `MPI_Finalize` | bootstrap, vtable handshake |
| `MPI_Comm_size` | trivial handle in |
| `MPI_Send`, `MPI_Recv` | rank/tag classes, status out |
| `MPI_Get_count` | status in |
| `MPI_Waitall` | request array staging, `MPI_STATUSES_IGNORE` |
| `MPI_Allreduce` | `MPI_IN_PLACE`, user op trampoline pool |
| `MPI_Comm_split` | out-handle, reverse map |
| `MPI_Type_create_struct` | `const` handle array, `_c` pairing |
| `MPI_Error_string` | output string staging and truncation |
| `MPI_Comm_c2f` | Fortran converter |
| `MPI_Comm_create_errhandler` + `MPI_Comm_set_errhandler` | trampoline pool *without* extra state, variadic trampoline |
| `MPI_Ialltoallw` | staged temporaries outliving the call |
| `MPI_File_open` | bitmask arguments, second handle class |

---

## 12. Risks worth measuring early

- Whether a dynamically created implementation handle can be bit-cast into an ABI
  handle without colliding with `0x20`..`0x2eb`. Probably yes for both
  implementations (§5.1), but probe it rather than assume it — and note the probe
  has to be a runtime test, not a configure-time one.
- `MPI_Op` needs a lookup regardless, because the trampoline must find the user
  function; and non-blocking `MPI_Alltoallw` needs a request->array map, which puts
  a branch on every request-completion call. The EuroMPI'23 ABI paper notes these
  as the only two places translation is not trivial, and both are addressed in
  §6.3.
- ~~Whether `RTLD_DEEPBIND` applies transitively.~~ **Settled** by
  `dev/dlopen-probe/`: it does. See §2.
- Whether `RTLD_DEEPBIND` survives ASan, which is the case it is most likely to
  disturb, and therefore whether the sanitizer CI jobs have to select `dlmopen`.
- musl: no `dlmopen`, and `RTLD_DEEPBIND` is accepted but ignored. Neither mechanism
  is available, so the probe needs a musl row before claiming Alpine support.
- Whether the MPICH C suite compiles at all against the ABI header, and how many
  exclusions that costs.
- ASan/valgrind noise from the implementations, which determines how useful those
  runs are.

---

## 13. Still open

- Windows/mingw: `dlopen` -> `LoadLibrary` shim, no RTLD flags, and which MPI is
  even the target there.
- `MPI_Pcontrol`'s varargs — forward `level` only, since implementations may ignore
  the rest, but confirm that reading is right.
- `MPI_Comm_spawn`'s `argv`/`array_of_argv` handling in detail.
- Attribute copy/delete callback lifetimes in detail: the implementation invokes
  them during `MPI_Comm_dup`, `MPI_Comm_free` and `MPI_Finalize`, so the
  `{user_fn, user_extra}` pairs must outlive everything the user holds.
- Whether to ship an `mpicc`-style compiler wrapper and/or pkg-config files.
- Capacity defaults for the fixed-size tables, and whether they should be configure
  options.

---

## Sources

Facts above were read from these files rather than from memory or from secondary
documentation.

| what | where |
|---|---|
| ABI header (mpi-abi-stubs, patched, 1986 lines) | `~/src/mpif/build/mpi/mpich-gcc/include/mpi.h` |
| MPICH header | `~/src/mpif/build/mpi-src/mpich-gcc/mpich/src/include/mpi.h` |
| Open MPI header | `~/src/mpif/build/mpi-src/openmpi-gcc/ompi/ompi/include/mpi.h.in` |
| MPI-5.0 standard | `doc/mpi50-report.pdf`, read with `pdftotext -layout` |
| CI and build precedent | `~/src/mpif/ci-scripts/README.md`, `~/src/mpif/CMakeLists.txt` |

Standard sections cited: §2.5.2 (opaque object deallocation), §6.12 and §6.13
(nonblocking and persistent collective argument lifetimes), §7.7
(`MPI_COMM_FREE_KEYVAL`), §9.4 (`MPI_ERRHANDLER_FREE`), §11.4.1 (finalize and
`MPI_COMM_SELF` attributes), §12.3.4 (`MPI_ACCUMULATE` forbids user-defined ops),
§15.2.1 (profiling interface requirements), §15.3 (`MPI_T` events), §20.2.1 (the
ABI library must be named `mpi_abi` and be the sole direct dependency).

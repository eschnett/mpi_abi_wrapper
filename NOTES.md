# MPI ABI wrapper — design

The authoritative design document for this project. Self-contained: it does not
depend on any other document in this repository. `STAGES.md` sequences the work
into sessions. Those two are the durable pair; `REVIEW-BRIEF.md` and `TODO.md`
are working notes that should be folded back into these and deleted, and every
`README.md` under a directory describes only what is in that directory. Anything
else is a document that will rot.

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
- **12** are marked deprecated in the header — `MPI_Attr_delete`/`_get`/`_put`,
  `MPI_Keyval_create`/`_free`, `MPI_Info_get`, `MPI_Info_get_valuelen`,
  `MPI_Get_elements_x`, `MPI_Status_set_elements_x`, `MPI_Type_get_extent_x`,
  `MPI_Type_get_true_extent_x`, `MPI_Type_size_x`. (An earlier draft said 31,
  which is not what the header says: `grep '; /\* deprecated' gen/include/mpi.h`
  finds 12 prototypes, plus the two deprecated callback typedefs
  `MPI_Copy_function` and `MPI_Delete_function`. The markers sit on the `MPI_`
  prototypes only, not on their `PMPI_` twins.) Deprecated still means provided.
- So: **1376 vtable slots** (§2 explains why `PMPI` is not folded onto the `MPI`
  slots) and **1376 exported symbols** in `libmpi_abi`.

**Consumers.** The point of the ABI is that large MPI-dependent projects can be
built once and run against any implementation, so the consumers that matter are the
widely-used libraries and applications:

- **HDF5**, whose parallel driver is the heaviest real user of `MPI_File_*` and
  therefore of the `MPI_File` handle class, the bitmask `amode`, and
  `MPI_File_get_view`'s `datarep`.
- **PETSc**, which exercises collectives, derived datatypes, user-defined operations
  and attributes about as broadly as anything does.
- **mpi4py**, which is the case that motivates the plugin scenarios: a host
  executable that knows nothing about MPI, loading extension modules that do.
- **[mpif](https://github.com/eschnett/mpif)**, MPI Fortran bindings over the ABI,
  and the only consumer that reaches the 26 Fortran converters and the status
  `f2c`/`c2f` paths. Requesting changes there is acceptable; it also shares problem
  shapes with this project, and several conventions here are lifted from it.

These are not interchangeable as tests: HDF5 covers file I/O that nothing else
touches, PETSc covers datatype and op breadth, mpi4py covers the loader scenarios,
mpif covers Fortran. §10 treats them as distinct oracles rather than as a list.

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
- **The ABI surface is complete and is MPI-5.0** (plus the Fortran extension of
  `doc/mpi.h.patch`). All 688 entry points are always exported. A function the
  implementation lacks is **reported at run time**, never omitted from the ABI: the
  slot returns `MPI_ERR_UNSUPPORTED_OPERATION` and the generator lists it in
  `gen/report.txt`. An application must be able to link and start against any
  wrapper and discover at run time what is missing.
- **The implementation is expected to provide the MPI-4.0 API.** That is what makes
  the common case a 1:1 mapping with no large-count narrowing fallback, since the
  `_c` variants exist there. It is an expectation, not a hard floor — MPI-4.1 and
  5.0 additions are handled by the runtime-reporting stubs above, and so in
  principle is anything else absent.

  **And it is not met by any released Open MPI**, which S1 found the hard way:
  Open MPI 5.0.10 defines `MPI_VERSION 3` / `MPI_SUBVERSION 1` and has **no `_c`
  entry point at all** — `MPI_Send_c`, `MPI_Type_create_struct_c` and the rest of
  the family are simply absent from its header. §9's "Open MPI >= 5.0" is
  therefore about what is *wrappable*, not about the MPI-4.0 expectation, and the
  MPI-4.0 configure check has to be a warning rather than the hard error §9
  originally listed. What that costs is exactly the large-count half of the
  surface: those slots become the `MPI_ERR_UNSUPPORTED_OPERATION` stubs of
  decision 6, which is the mechanism working as designed rather than a gap. It is
  worth knowing that the mechanism is load-bearing on day one and not a
  contingency for exotic implementations.

---

## 2. Architecture

```
application
    |  MPI_Send(...)                        ABI types only
    v
libmpi_abi.so          exports MPI_* and PMPI_* (1376 one-line functions)
    |                  includes the ABI mpi.h and nothing else
    |  vt->MPI_Send(...)                    ABI types only, 1376 slots
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
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t abi_subversion,
                      uint32_t layout_hash, size_t size,
                      const void *abi_probe, const char **diagnostic);
```

returning NULL with a diagnostic on mismatch. The header carries **both** `MPI_ABI_VERSION` (currently 1) and
`MPI_ABI_SUBVERSION` (currently 0), and both are reported and checked. The layout
hash does not cover the subversion: one that added no slot would leave the hash
unchanged while still meaning the halves were generated from different
specifications. `layout_hash` itself is generated from the slot list, so a
regeneration that reorders or inserts a slot is caught rather than silently calling
through a shifted one. A getter rather than an exported struct, because you
would otherwise have to trust the layout in order to read the version out of it —
and because the getter is a natural place to build the reverse handle map before
returning.

**PMPI gets its own slots**, so 1376 rather than 688. `MPI_Send` and `PMPI_Send` are
two definitions in `libmpi_abi` (not a weak alias — macOS aliases need `-Wl,-alias`
or `__asm__` labels, and a one-line body makes an alias pointless) reaching *two*
slots, whose wrapper bodies call the implementation's `MPI_Send` and `PMPI_Send`
respectively.

Routing both names to a single `MPI_X` slot is cheaper and was the earlier plan; it
is wrong at the second of the two levels where interposition can occur:

- *ABI level*, a tool between the application and `libmpi_abi`: works either way.
- *Implementation level*, a tool between `libmpiwrapper` and `libmpi` (Score-P, TAU,
  mpiP — reachable only if deliberately linked into the wrapper's dependency chain,
  since `RTLD_DEEPBIND`/`dlmopen` defeats `LD_PRELOAD`). With one slot, an
  application calling `PMPI_Send` to bypass profiling still passes through that tool:
  it bypassed the ABI-level layer only. `PMPI_X` means "the implementation with no
  profiling wrapper", and a layered shim must not silently reintroduce one.

A second reason, nearly as good: it makes the ledger **1:1**. With one slot the
generator emits 1376 entry points but 688 bodies, a 2:1 mapping with a special case;
with two, "each of the 1376 has exactly one slot and one body" is a uniform
invariant.

Cost: +5.5 KB of vtable, and the generated bodies double — emitted from one template
per function differing only in the call target, so nothing doubles in what is
maintained by hand.

**No configure probe is needed**, contrary to an earlier draft of these notes. In
both implementations the `PMPI_*` names are the *strong* definitions and the `MPI_*`
names are weak aliases at the same address:

```
MPICH   libmpich.so   0x159d40 W MPI_Send   0x159d40 T PMPI_Send
OpenMPI libmpi.so     0x08d690 W MPI_Send   0x08d690 T PMPI_Send
```

That was read off one pair of builds, and the *binding* half of it does not
generalize even on Linux. Ubuntu 24.04/aarch64, measured in S1:

```
MPICH 4.1     libmpich.so  0x10fe20 T MPI_Send  0x10fe20 T PMPI_Send   668 T, 0 W
Open MPI 4.1  libmpi.so    0x084630 T MPI_Send  0x084630 T PMPI_Send   432 T, 0 W
```

Same address, both *strong*. MPICH 3.1.4, from 2014, is the other way round — 385
weak `MPI_*` against 385 strong `PMPI_*`, exactly the shape the note above
recorded — so the original observation was accurate for its time and MPICH
changed. What holds everywhere, and is all this argument needs, is that **both
names exist and resolve to the same code when no tool is interposed**. Whether the alias is weak or strong is an ELF interposition detail
that matters to profiling tools, not to us — on ELF, scope order decides
interposition, not binding strength.

That is how the profiling interface works at all — a tool's strong `MPI_Send`
overrides the weak alias while `PMPI_Send` stays reachable — so both names are
unconditionally present in whatever library is already linked. MPICH ships no
`libpmpich`: 619 `T PMPI_*` symbols and zero `T MPI_*` in `libmpich.so`, with the
same weak-alias pair in the static archive. `PMPILIBNAME` renames a library modern
MPICH does not build separately.

A useful corollary: because the two names share an address, the `MPI` and `PMPI`
bodies behave identically when no tool is present and differ exactly when one is,
which is the intent.

**The weak-alias shape is a Linux/ELF observation and does not generalize**, which
S1 measured on macOS:

```
MPICH 4.3.1 (conda-forge)  libmpi.dylib   T MPI_Send,  no PMPI_ symbols at all
                           libpmpi.dylib  T PMPI_Send
Open MPI 5.0.10            libmpi.dylib   T MPI_Send  @0xae214
                                          T PMPI_Send @0x6fa1c   (two definitions)
```

So on macOS MPICH does ship a separate profiling library, and Open MPI compiles two
distinct functions rather than aliasing. Neither disturbs the design — both names
still resolve, and the conclusion "no probe and no fallback" stands — but it does
mean the wrapper must link **what `mpicc` links** rather than a library it names
itself, since `-lmpi` alone would leave every `PMPI_*` undefined on that MPICH. An
implementation that really lacked the shifted names now fails to link with an
undefined symbol naming one, which is the outcome §5.9 asks for.

**The wrapper's own internal MPI calls use `PMPI_*` unconditionally** — in the
hand-written set of §8, where `MPI_Init` needs a rank or the error-code registry needs a
class. An internal call is not application traffic and must not be counted as such
by an interposed tool. Same discipline implementations follow inside themselves, and
independent of the slot question.

**Locating the wrapper.** An environment variable, falling back to a path baked in
at build time. One `libmpi_abi` binary can therefore be pointed at any wrapper,
which is the practical payoff of the split. The installed name is plain
(`libmpiwrapper.so`); an earlier draft encoded the MPI in it
(`libmpiwrapper-mpich-4.3.so`) so that several could share one prefix, and §9
records why that was dropped in favour of one prefix per MPI installation
(decision 5).

**Bootstrap.** A library constructor in `libmpi_abi` sets a plain pointer, read
with no atomic and no NULL check outside debug builds (decision 8). The load
cannot hang off `MPI_Init`, because `MPI_Initialized`, `MPI_Get_version` and the
`MPI_T_*` calls are legal before it — hence a constructor. It needs no
lazy-init guard beside it: anything that can call an entry point must link
`libmpi_abi`, hence depends on it, hence its own constructors run after ours,
and a plugin `dlopen`ed later is no exception because loading it loads
`libmpi_abi` first. An earlier draft called for an idempotent acquire-load guard
"so that a plugin `dlopen`ed before the constructor runs still works"; there is
no such window, and the next section measures what the guard would have cost.

(`MPI_Wtime` was on that list in an earlier draft and does not belong there:
MPICH 4.3.1 answers a pre-`MPI_Init` `MPI_Wtime` with "Attempting to use an MPI
routine before initializing", and the standard's own list of what may be called
before initialization does not include it. It matters because `MPI_Wtime` is the
obvious choice for the load-time probe above, and would have made that probe fail
on MPICH and only on MPICH.)

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

**Measured, not reasoned.** The table below says what the *loader* does, not
which configurations are usable: the mock has no MPI in it and therefore
`dlopen`s no components, which is exactly what makes its `dlmopen` row not
generalize — see "`dlmopen` does not survive contact with a real MPI" below, and
`dev/dlopen-probe/README.md`'s scope section.

`dev/dlopen-probe/` is a mock-up of the three-library
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
4. **The load-time isolation check works on the mock** — it reported failure on
   the flat-namespace build before any call was made, and `dladdr` still resolves
   across a `dlmopen` namespace boundary on this glibc, so the check survives that
   mode too. That is a statement about the mock: against a real MPI the `dladdr`
   check turned out to answer a different question than the one that matters, which
   is what the behavioural probe below was added for, and even the probe is
   incomplete.

**Per platform, as measured:**

| | how | why |
|---|---|---|
| macOS | `RTLD_LOCAL` | the two-level namespace binds `libmpiwrapper`'s `MPI_Send` to `libmpi` at link time, so there is nothing to capture |
| Linux | `RTLD_LOCAL \| RTLD_DEEPBIND` by default, `dlmopen(LM_ID_NEWLM)` selectable | both isolate the mock; `DEEPBIND` is simpler, has no namespace limit, and is the only one that works with a real MPI (below) |
| FreeBSD | `RTLD_LOCAL \| RTLD_DEEPBIND` | `dlmopen` does not exist |

Keep both Linux modes selectable at run time, as MPItrampoline does, because each
has known costs: `dlmopen` is semi-abandoned, caps namespaces at glibc's `DL_NNS`
(16), and gives the wrapper a separate libc; `RTLD_DEEPBIND` interferes with
`malloc` interposition and with sanitizers. That last cost used to be the concrete
reason `dlmopen` had to stay available, and it no longer is — `dlmopen` does not
work with a real MPI at all (below), so the sanitizer jobs need one of the other
answers S9 has. Keeping the mode selectable is now cheap insurance rather than a
plan. Remember the namespace id from the
first load and reuse it, so the wrapper and its dependencies stay in one namespace.

**`dlmopen` does not survive contact with a real MPI, and that is worth knowing
before S9 depends on it.** Both MPICH 4.1 and Open MPI 4.1 segfault in `MPI_Init`
when the wrapper is loaded that way, in *glibc's own loader* rather than in
anything of ours:

```
add_to_global_resize (elf/dl-open.c:126)
_dl_open (".../pmix/pmix_mca_pcompress_zlib.so", mode=RTLD_LAZY|RTLD_GLOBAL, nsid=-2)
PMIx_Init  <-  PMPI_Init  <-  mpiwrapper_w_MPI_Init  <-  libmpi_abi MPI_Init
```

Every modern MPI loads its components with `dlopen`, and PMIx asks for
`RTLD_GLOBAL`; glibc cannot add to the global scope of a namespace created by
`dlmopen`, because that namespace has no main map. So the failure is structural
rather than incidental: it is not one implementation's quirk but what happens when
anything inside a fresh namespace loads a plugin globally, which is what MPI
runtimes do at startup.

That diagnosis has since been confirmed from the other side. **MPICH 3.1.4 runs
fine under `dlmopen`** — all tests pass in that mode — and it predates PMIx and
loads no components at run time. So `dlmopen` itself is not broken; it is
unusable with any MPI that `dlopen`s plugins, which is every current one. That
narrows what S9 might do: an MPI configured with its components built in is a
real option, not a hope.

The consequence lands on S9. `dlmopen` was the designated fallback for the case
where `RTLD_DEEPBIND` breaks the sanitizers, and that fallback does not exist. The
options S9 actually has are: run the sanitizers with `RTLD_DEEPBIND` and find out
whether it really breaks them (measure before assuming), build the MPI under test
with its components static so nothing is `dlopen`ed at run time, or accept that the
sanitizer jobs cover `libmpi_abi` and the conversion layer rather than the loaded
configuration. The mode stays selectable — it costs one environment variable, and a
future glibc or a component-free MPI may make it work — but nothing may be
*planned* on it.

**Binding mode defaults to `RTLD_LAZY`, not `RTLD_NOW`** — also a correction.
`RTLD_NOW` forces every undefined symbol in `libmpi` and its dependency closure to
resolve, and real MPI installations have symbols that are never called. Overridable.

### Where the loading actually stands

Kept current, because "isolation works" is a claim with a per-platform truth value
and the table above says what *should* happen rather than what has been seen. As of
S1, all of it measured on macOS 26 / arm64 unless stated:

| configuration | mechanism | status |
|---|---|---|
| macOS, native MPICH 4.3.1 | `RTLD_LOCAL` + two-level namespace | **works**, 6/6 tests, two ranks |
| macOS, native Open MPI 5.0.6 | same | **works**, 6/6 tests, one rank (its launcher is broken here, unrelated to loading) |
| macOS, native Open MPI 6.1.0a1 (main) | same | **works**, 6/6 tests, two ranks — despite 698 weak `MPI_*` |
| macOS, wrapper forced `-flat_namespace` | none | **refused at load**, and that refusal is a test |
| macOS, an ABI-implementing MPI (weak `MPI_*`) | none available | **refused at load**; see below |
| Linux glibc, native MPICH 4.1 | `RTLD_LOCAL \| RTLD_DEEPBIND` | **works**, 6/6 tests, two ranks (Ubuntu 24.04, aarch64, in Docker) |
| Linux glibc, native Open MPI 4.1 | same | **works**, 6/6 tests, two ranks |
| Linux glibc, MPICH 3.1.4 (MPI-3.0) | same | **works**, 6/6 tests, two ranks — the configure floor, verified rather than declared |
| Linux glibc, unisolated `dlopen` | none | **refused at load**, with the capture diagnostic |
| Linux glibc, `dlmopen(LM_ID_NEWLM)` | — | **does not work with a real MPI**; see below |
| Linux musl | neither exists | expected to be refused at load; no Alpine support until something else is found (§12) |
| FreeBSD | `RTLD_DEEPBIND` | unverified |
| Windows | — | open (§13) |

What makes the first two macOS rows work is that both implementations define
their `MPI_*` **strongly**, so dyld's weak coalescing never gets a say and the
two-level namespace does the isolation on its own:

| library | `MPI_*` | `PMPI_*` | wrapping it |
|---|---|---|---|
| MPICH 4.3.1 `libmpi.dylib` / `libpmpi.dylib` | 674 strong, 0 weak | 78 + 672 strong | works |
| Open MPI 5.0.6 `libmpi.dylib` | 472 strong, 0 weak | 468 strong | works |
| Open MPI 6.1.0a1 built natively | **0 strong, 698 weak** | 698 strong | **works anyway** |
| Open MPI 6.1.0a1 built `--enable-standard-abi` | **0 strong, 683 weak** | 683 strong | **refused at load** |

Strong `MPI_*` is therefore *sufficient* and not necessary, which the last two
rows make plain: they differ only in a configure flag, both are all-weak, and
only one of them captures. The deciding factor is narrower than symbol binding
and we have not pinned it down — see "Weak `MPI_*` alone does not predict
capture" below. The dangerous row is not an MPI anyone would wrap for its own
sake — it is oracle 5. Where capture does occur the wrapper refuses rather than
running wrong, and the fix available then would be to route the wrapper's calls
through `PMPI_*`, which is strong everywhere; the cost is decision 7's
implementation-level interposition, so it would have to be opt-in and loud
rather than a silent fallback.

The reliability property this all adds up to is worth stating plainly, because it
is weaker than "it always works" and stronger than "it usually works": **either the
wrapper loads and its calls provably reach the implementation, or it refuses to
start and says why.** There is no configuration known to us in which it loads and
silently does the wrong thing — the one that used to exist, weak-symbol capture,
is what the behavioural probe was added for.

**Check the outcome, not the mechanism.** `dlinfo(handle, RTLD_DI_LMID)` confirms
which namespace you got but not that every reference resolved the way the namespace
was meant to make it resolve. So `libmpi_abi` passes the address of one of its own
functions to `mpiwrapper_get_vtable`, and the wrapper `dladdr`s that together with
the `MPI_Send` it actually resolved, refusing if the two share a base object. That
catches the capture at load, positively, on every platform, whatever the loader did
— and it does not depend on knowing whether `RTLD_DEEPBIND` propagates to
dependencies. `src/mpiwrapper/getvtable.c` implements it.

**But `dladdr` answers a subtly different question than the one that matters, and
S1 found a case where the two answers differ.** Wrapping an Open MPI built for the
standard ABI — all 683 of its `MPI_*` weak, its `PMPI_*` strong — macOS gives:

```
dladdr(&MPI_Send) inside the wrapper   ->  the implementation   (correct)
the wrapper's actual call to MPI_Send  ->  libmpi_abi           (captured)
```

dyld coalesces weak definitions across images, so our *strong* `MPI_Send` wins
over the implementation's weak one even under a two-level namespace, while taking
the symbol's address still resolves through the namespace record. The symptom is
not recursion but **silent double execution**: the operation runs once at each
level and returns the right answer. It surfaced only because the second pass tried
to attach a staged temporary to a request already in the table (§6.3).

So the ABI side adds a **behavioural probe**, which asks the question no address
comparison can: it makes one call through the vtable and sees whether the call
comes back. `MPI_Get_version` is the probe — legal before `MPI_Init` in every
version of the standard, no side effects. (`MPI_Wtime` reads better and is wrong:
MPICH refuses it before initialization, contrary to what an earlier draft of these
notes assumed.)

The mechanism keeps the generated code out of it entirely. A captured call
re-enters `libmpi_abi`'s own exported entry point, which does nothing but call
through `mpi_abi_vt` — so pointing `mpi_abi_vt` at a **decoy table** during the
probe both detects the re-entry and stops the recursion, with no forwarder needing
to know a probe exists. *Every* slot of the decoy points at the same recorder, not
just the one being called: the captured call can land on any entry point, since
the implementation's `MPI_Get_version` may reach for another MPI function
internally, and a decoy with one slot filled turns the capture into a jump through
a null pointer instead of a diagnostic — which is how the first version of this
was discovered to be inadequate.

The consequence for the *design* is that a wrapper cannot be layered over an
ABI-implementing MPI on macOS at all: refusing at load is the best available
outcome, and the wrapper says so. See oracle 5 in §10.

**Weak `MPI_*` alone does not predict capture, and the probe is not a complete
detector.** Two later measurements say so, and both are worth keeping because the
tempting rule — "weak implementation symbols mean capture" — is wrong:

- **Open MPI main (6.1.0a1), built natively**, has 698 weak `MPI_*` and 0 strong,
  exactly the shape that captured above. It **wraps correctly**: 6/6 tests at two
  ranks on macOS, with the probe passing. So the deciding factor is something
  narrower than symbol binding, and we have not pinned it down — the Mach-O
  header flags (`WEAK_DEFINES BINDS_TO_WEAK`) and install names are the same in
  both.
- The same Open MPI, with the wrapper deliberately built `-flat_namespace`,
  captures **partially**: `MPI_Send` and the datatype conversions recurse until
  the stack is exhausted, while `MPI_Get_version` — the probe's own call —
  resolves outward and reports the right answer. So the probe returns "not
  captured" and the process dies later of recursion.

Both checks are therefore *sound and incomplete*: what they report is true, and
what they miss is a partial capture whose sampled call happens to bind outward.
The failure mode they leave is a stack overflow, not a wrong answer, which is why
`test/check_isolation.cmake` treats a crash as an acceptable outcome and only a
*successful* run of an unisolated wrapper as a failure. Sampling more entry points
would narrow the gap and not close it; closing it would need the loader to answer
"where does this call site bind", which neither `dladdr` nor `dlsym` will do.

`size` is `sizeof(struct mpiwrapper_vtable)` as the *caller* understands it, and
**it must match exactly.** An earlier contract said a wrapper may accept a
smaller size and serve the common prefix; that was never reachable, because the
three checks in front of it each demand exact equality — `abi_version`,
`abi_subversion`, and a `layout_hash` taken over the *whole* slot list — so a
caller built from a shorter slot list is refused before `size` is looked at. A
provision no input can reach is not forward compatibility, it is a story about
it, and it invited the reader to plan on growth the handshake does not
implement. If additive growth is ever wanted, the honest way to get it is to
hash only the slots up to `size`, and that can be adopted without changing this
signature.

The check is kept rather than deleted, because it is the one thing here the hash
cannot see. The hash is taken over the *text* of the slot list, so two halves
that agree on every declaration and disagree on what those declarations weigh —
a 32-bit `libmpi_abi` against a 64-bit `libmpiwrapper`, or two compilers
differing about a struct's layout — hash identically and differ in `sizeof`.
That is precisely the mismatch that produces a call through a shifted slot, and
it is the case §4.1's pointer-sized handles make plausible rather than exotic.

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

**Four sharp edges found while implementing this in S0**, none visible from the
three rules alone:

- **`MPI_ABI_VERSION`/`MPI_ABI_SUBVERSION`** (the ABI *protocol* handshake version,
  1/0) rename to `MPIABI_ABI_VERSION`/`MPIABI_ABI_SUBVERSION` under the plain rule
  — prefix `MPI_` stripped, `MPIABI_` prepended, same as any other macro. The
  double "ABI" looks odd next to `MPI_VERSION`/`MPI_SUBVERSION` (the MPI *standard*
  level the stub implements, 5/0), which rename to the unadorned
  `MPIABI_VERSION`/`MPIABI_SUBVERSION` — but the two source names differ
  (`MPI_ABI_VERSION` vs `MPI_VERSION`), so the renamed spellings differ too and
  nothing collides. An earlier draft of this generator special-cased `MPI_ABI_`
  to collapse onto `MPIABI_` (dropping the inner "ABI"), which read better but
  created exactly the collision this paragraph used to describe; the plain,
  uniform rule turned out to be both simpler and correct, so the special case was
  removed rather than kept.
- **`MPIX_TYPECLASS_LOGICAL`** is the one enumerator in the stub not spelled
  `MPI_*` — a legacy alias sitting in the same anonymous enum as the
  `MPI_TYPECLASS_*` family. Left unrenamed it collides with `mpi.h`'s own
  definition of the same enumerator the moment both headers are included
  together. Renamed to `MPIABIX_TYPECLASS_LOGICAL` (same scheme, `X` kept).
- **`MPI_T_cb_safety`/`MPI_T_source_order`** are declared `typedef enum
  MPI_T_cb_safety { ... } MPI_T_cb_safety;` — tag and typedef name spelled
  identically, unlike every handle type (whose tag is `MPI_ABI_Foo`, deliberately
  different from its typedef name `MPI_Foo`). Rule 2 protects tags because
  handle-pointer tags are meant to be *shared* with `mpi.h` for type identity; here
  the opposite is true — a real implementation's own `<mpi.h>` also declares a tag
  named `MPI_T_cb_safety` with different enumerator values, so leaving *this* tag
  unrenamed redeclares it incompatibly the moment `mpiabi.h` and an implementation
  header meet in one translation unit (exactly what `libmpiwrapper` does). Both
  occurrences are renamed to `MPIABI_T_cb_safety`, deliberately breaking the tag
  identity with `mpi.h`'s own tag of that name — the two headers were never meant
  to interoperate at this type the way they do for handles.
- **`MPI_Aint`/`MPI_Offset`/`MPI_Count`** are defined through a
  `#if !defined(MPI_ABI_X) / #define MPI_ABI_X <rhs> / #endif / typedef MPI_ABI_X
  MPI_X; / #undef MPI_ABI_X` idiom, and for these three (unlike the handle types)
  `MPI_ABI_X` and `MPI_X` rename to the *same* spelling (`MPIABI_Aint`, etc., since
  stripping `MPI_ABI_` and stripping `MPI_` land on the same tail). A naive
  line-by-line rename turns the typedef line into `typedef MPIABI_Aint
  MPIABI_Aint;`, and the preprocessor macro-expands *both* occurrences — it has no
  notion that one was meant to survive as the newly-introduced name — so the type
  is never actually introduced, and `MPI_Count`'s definition (which references
  `MPI_Offset`) fails one link down the chain. `dev/generate_headers.py` resolves
  these three directly (`typedef intptr_t MPIABI_Aint;`, etc.), using the header's
  own default, no-override branch, rather than reproducing the scaffolding.

**Two more, found in S1**, both about what "the same type on both sides" actually
requires:

- **`MPI_Status` needed a struct tag, and `doc/mpi.h.patch` now adds one.** Rule 2
  keeps tags unrenamed so that `MPIABI_Comm` and `MPI_Comm` are the same type; the
  stub header gives every handle a tag (`MPI_ABI_Comm`) but declared the status as
  a typedef of an *anonymous* struct. Two anonymous structs are two incompatible
  types however identical their layout, so the ABI side could not pass an
  `MPI_Status *` into a vtable slot typed `MPIABI_Status *` — 90-odd forwarders
  would each have needed a cast, and a cast in a forwarder silently absorbs the
  genuine type errors these forwarders exist to catch. The patch names the tag
  `MPI_ABI_Status`, matching the handle convention.

  A tag alone is not enough, because unlike the handle tags this struct is
  *defined* in both views: whichever header comes second would redefine it. So the
  definition is guarded and the typedef is not —

  ```c
  #if !defined(MPI_ABI_STATUS_DEFINED)
  #define MPI_ABI_STATUS_DEFINED
  struct MPI_ABI_Status { ... };
  #endif
  typedef struct MPI_ABI_Status MPI_Status;   /* MPIABI_Status in the other view */
  ```

  — which works in either include order. The guard macro is the one name
  `dev/generate_headers.py` deliberately leaves **unrenamed** (its `KEEP_UNRENAMED`
  set): it coordinates the two views rather than naming anything in the API, and
  renaming it would give the two headers different guards, at which point both
  define the struct and neither compiles beside the other. This is worth
  proposing upstream to mpi-abi-stubs; it costs nothing and nothing about the ABI
  changes.

- **`gen/include` holds `mpi.h` beside `mpiabi.h`, and that is a trap for the
  wrapper's build.** `libmpiwrapper` includes `<mpi.h>` meaning the
  *implementation's*, and `"mpiabi.h"` from our own directory. Put our directory on
  its include path and `<mpi.h>` finds the ABI header instead — which **compiles
  and links**, because the ABI header is a complete valid `mpi.h` and the
  implementation exports the names, and then fails at run time with the
  implementation rejecting `comm=0x101`, i.e. `MPIABI_COMM_WORLD` arriving
  unconverted. It cannot be fixed by ordering the flags: CMake passes an imported
  target's includes as `-isystem`, and every `-I` is searched before every
  `-isystem`. So `mpiabi.h` is staged into a directory containing nothing else,
  and `src/mpiwrapper/internal.h` carries an `#error` on the same condition
  (`MPI_ABI_STATUS_DEFINED` visible before `mpiabi.h` is included) for anyone
  building outside CMake. That `#error` has an escape hatch,
  `MPIWRAPPER_WRAP_ABI_IMPL`, which is exactly oracle 5's configuration.

---

## 3. The generator

**All 688 entry points are accounted for by a generator, in Python, with a named
set written by hand.** Since S2 the split is not an estimate: **568 mechanical
against 120 needing per-function judgement**, and `gen/report.txt` names every
one of the 688. (Earlier drafts said "roughly 600 and ~50" here and in §8,
while §8's own list added up to about twice the 50.) Of the 568 mechanical,
473 are generated today and 95 wait on the argument classes S3 adds; the
generator fails if an entry point is in neither set and not in the ledger.

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

ABI-side names carry an `abi_` prefix; implementation-side names are bare. This is a
wrapper body in `libmpiwrapper`, so it *calls* `MPI_Send` and does not define it —
the exported `MPI_Send` lives in `libmpi_abi` and is a one-line forwarder (§2):

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

**Since S1 the worked version of each shape is the tested code, not
`examples/`**, and the generator is required to reproduce *that* — which since
S2 means `dev/s1-reference/`, frozen, checked by `dev/check_prototype.py`. The examples were written before any of
this ran, and three of their details turned out to be wrong once it did: the
reverse-map tables are `static const uint64_t` arrays initialized from handle
macros, which is not a constant expression on an implementation whose handles are
addresses and so does not compile against Open MPI at all; there is one bitmask
mapper where two are needed (§5.5); and the `dlopen` narration predates the
`RTLD_LOCAL`-plus-isolation correction. They are kept as narrated excerpts, with
those three corrected, but a shape that exists in both places is a second source of
truth, and the tested one wins.

### One portability trap in generated switches

Case labels over *handles* must be **numeric, with the symbolic name in a
comment** — the integer families are ordinary enumerators and are switched by
name:

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

The first two are the S0 step and live in `dev/generate_headers.py`, which
`dev/generate.py` imports rather than duplicates, so one command writes all
seven and one `--check` covers all seven.

### What S2 settled

S2 wrote the generator and the mechanical classes. Six things it had to decide
that this section did not already answer, each recorded here because the code
alone would not say why.

**How a generated body knows what the implementation has.** Decision 6 says
"generated `#ifdef` stubs" without saying what the `#ifdef` tests, and every
answer that does not involve a configure test is wrong.

- **A version test under-reports.** Open MPI 5.0.10 announces MPI-3.1 and has
  sessions and partitioned communication. And the gap is not small: the ABI is
  MPI-5.0 and the enforced floor is MPI-3.0, so `#if MPI_VERSION >= 4` would
  stub a couple of hundred entry points that are there.
- **`nm` over-reports.** It cannot see what the header provides as a macro,
  which is how Open MPI provides `MPI_Aint_add`.
- **`#ifdef` on the implementation's own name for a *constant* is worse than
  either, because it fails silently.** `#ifdef` sees macros and does not see
  enumerators, and implementations use both: MPICH spells `MPI_COMBINER_*` and
  `MPI_CART` as enumerators, Open MPI spells `MPI_THREAD_SINGLE`,
  `MPI_COMM_TYPE_SHARED` and `MPI_IDENT` that way. An `#ifdef` on one of those
  answers *no* for a constant that is right there — and then the case drops out
  of the conversion table, the default arm passes the ABI value through
  unmapped, and nothing fails. **Measured rather than argued:** MPICH 4.3.1 has
  `MPI_COMBINER_VALUE_INDEX` as `= 20` in an enum, and an S2 draft that guarded
  it with `#ifdef` stopped translating that combiner without failing anything.

So `dev/probe_impl.py` asks the compiler, at configure time, using the
implementation's own header — the same header the wrapper bodies are compiled
against, and the only thing in the build that sees macros and enumerators
alike. It writes `mpiwrapper_impl_config.h` with one `MPIWRAPPER_HAVE_<name>`
per available entry point *and per available optional constant*, and every
guard in the generated sources tests one of those and nothing else. `internal.h`
`#error`s if that file did not come from the probe, because a *missing* probe
would otherwise turn the whole library into stubs, which links, loads, and
answers `MPI_ERR_UNSUPPORTED_OPERATION` to everything.

**It is one configure test, not one per name.** All the questions go into a
single translation unit, one probe per line, compiled `-fsyntax-only`. A name
that is a macro answers `#ifdef`; anything else has to be declared for its
probe to compile, and the probe differs by what the name is — `sizeof &name`
for an entry point, which is a function, and `sizeof(name)` for a constant,
whose address may not be takeable. When the compile fails it reads the
diagnostics' *line numbers* — never their wording — drops those probes and
compiles again, so the answer is always confirmed by a compile that succeeded.
Measured: **0.3–0.6 s for 521 names**, against MPICH 4.3.1 (7 absent: the five
sized Fortran logicals, `MPI_ERR_ABI`, `MPIX_TYPECLASS_LOGICAL` — plus the 28
entry points it lacks) and Open MPI 5.0.10 (166 absent, mostly the `_c` forms).
Both agree exactly with `nm` where `nm` can answer. Everything about it is
compile-only, so cross-compiling still works (§9).

What is guarded at all stays narrow, and that part is unchanged from S1's rule:
only what the standard makes optional — the sized Fortran types, the predefined
handles and enumerators added after the MPI-3.0 floor, `MPI_T`. Everything else
is emitted unguarded, so an implementation that really lacks `MPI_INT` or
`MPI_COMBINER_NAMED` fails the build naming it rather than quietly dropping a
mapping.

**One implementation declaration disagrees with the standard.** Open MPI 5.0.x
declares `MPI_Pready_list(int length, int partition_list[], MPI_Request)` where
MPI-4.0 and the ABI header say `const int array_of_partitions[]`. That is a
named `(routine, parameter)` entry in the generator, not a cast applied wherever
a call fails to compile — the difference matters, because the second habit is
how a real `const` violation gets absorbed.

**`MPI_IN_PLACE` is not in `apis.json`.** Every choice buffer is just `BUFFER`
there, so which sites accept `MPI_IN_PLACE` is a table, keyed on the base
routine and the parameter name and covering the `I`, `_init` and `_c` forms.
It has to exist: `MPI_IN_PLACE` is `(void *)1` in the ABI and `(void *)-1` in
MPICH, so a site that omits the test hands MPICH an address of 1. S1 needed
three sentinel mappers and S2 needs four — `MPI_Scatter` and `MPI_Scatterv`
take `MPI_IN_PLACE` at a *receive* buffer, which is not `const`.

**Two enum tags really are two types.** Rule 2 of the renaming leaves struct and
enum tags alone precisely so that 1376 forwarders need no cast — but
`MPI_T_cb_safety` and `MPI_T_source_order` spell their tag exactly like their
typedef, so S0 had to rename both occurrences or they would collide with an
implementation's own declaration. The consequence lands here: those two ABI
types and their implementation counterparts are genuinely distinct, and the
four `MPI_T` forwarders that pass them are the only place in `entrypoints.c`
where a cast is correct. `MPI_Pcontrol` is the other exception, for an
unrelated reason: C cannot forward `...`, and MPI-5.0 §14.2 lets a profiling
layer ignore the extra arguments.

**Where the generator stops.** 473 of the 688 are generated, 120 are in the
ledger, 95 are deferred to S3 with the class that blocks them, and
`gen/report.txt` names every one. The deferred set is exactly S3's list —
out and inout arrays, status arrays, output-string buffers, keyvals, callbacks,
`MPI_T` — plus one shape worth naming here because it is not an argument class
at all: the eight `*alltoallw*` forms and six neighbourhood relatives take
arrays whose length is *the size of the communicator*, not a parameter, so the
generator has nothing to size a temporary from until it calls
`PMPI_Comm_size` first, which is what S1's hand-written `MPI_Ialltoallw` does.

### What S3's first half settled

S3a added the array classes: out, inout and status arrays, the extents
`apis.json` records as `*`, and the lifetime rules. It left **518 of the 688
generated, 118 in the ledger and 52 deferred** — the keyvals, output-string
buffers, callbacks and `MPI_T` that S3's second half took, below.
`MPI_Waitall` and `MPI_Ialltoallw` left the ledger, since the classes S1 wrote
them for are generated now.

**The lifetime rules need no flag, because the implementation already keeps
one.** §5.7 says the request-map entry needs a flag to tell a nonblocking
operation's temporaries (freed at completion) from a persistent one's (freed at
`MPI_Request_free`). It does not: at every completion site the discriminator is
whether the implementation set the handle to `MPI_REQUEST_NULL`, which it does
for a nonblocking request at completion and for a persistent one at
`MPI_Request_free` — and *not* for a persistent request that has merely
completed, which is exactly the case freeing early would corrupt. So one
emitted rule covers both lifetimes, S1's `MPI_Waitall` already had it, and the
generator has no per-entry-point state to get wrong. It also makes the release
side uniform: every entry point with an inout request releases, which is the
`MPI_Wait`/`Test`/`Request_free`/`Start` ledger item of §6.3, and `MPI_Start`
and `MPI_Startall` get the same code and never fire it because they null
nothing.

Symmetrically, whether a temporary outlives its call is not a property of the
routine's *name* — a lookup table of the eight `I`/`_init` forms — but of its
signature: an in-direction array that has to be converted, in a routine that
hands back a request. That predicate produces exactly the eight, and the count
is a frozen tally, so a ninth would have to be admitted deliberately.

**Extents that `apis.json` gives as `*`.** These are not one class. The
communicator's size sizes `MPI_Alltoallw`'s datatype arrays (the *remote* size
on an intercommunicator); two different degrees size the neighbourhood forms'
send and receive arrays; the last entry of the index array sizes
`MPI_Graph_create`'s edges; the degrees *sum* to `MPI_Dist_graph_create`'s
destinations. Each is a named `(routine, parameter)` entry, and the ones that
have to ask the implementation go through `src/mpiwrapper/extents.c` — before
the call they serve, so that a failure returns the implementation's own error
and nothing has been allocated. The errors coincide: asking a communicator with
no topology for its neighbour counts fails with the `MPI_ERR_TOPOLOGY` the
neighbourhood collective would itself have returned.

The same table carries a second kind of entry that is easy to miss: an OUT
array the implementation fills only *partly*. `MPI_Graph_get`,
`MPI_Graph_neighbors`, `MPI_Dist_graph_neighbors` and `MPI_Type_get_contents`
all take a caller's maximum and write however much the object actually has, and
converting the tail would convert uninitialized elements — a wrong answer
wherever the garbage collides with an implementation sentinel, and a sanitizer
report always. So those carry an allocation extent and a smaller conversion
extent.

**Two arrays that must not be read at all.** With `MPI_IN_PLACE` at `sendbuf`,
MPI-5.0 §6.11 makes `sendcounts`, `sdispls` and `sendtypes` *ignored*, and a
legal program may pass a null pointer for them. That is harmless for the two
the wrapper merely forwards and fatal for the datatype array it reads element
by element, so the six non-neighbourhood `alltoallw` forms fill their staged
send array with `MPI_DATATYPE_NULL` in that case instead. Measured rather than
assumed: both implementations accept such a call with all three arguments null,
so neither reads them.

**`MPI_Type_get_contents`' `max_datatypes` is not forwarded.** The standard
makes it an upper bound, so the wrapper passes the *envelope's* count instead
and stages an array that size. `dev/get-contents-extent/` measures why: Open
MPI 5.0.6 walks the whole of `max_datatypes` and dereferences each entry it
finds there — which for an OUT parameter is whatever the caller's memory held —
and segfaults on a legal program with no wrapper involved. Passing the
envelope's count satisfies "at least as large as" exactly, is what the
implementation was going to write either way, and keeps our staged array's
uninitialized tail out of its reach; a caller's too-*small* maximum still
reaches the implementation and is still rejected, because the substitution is a
minimum.

**One array crosses unconverted although its kind says otherwise.**
`MPI_Group_range_incl`'s `ranges` is `int[][3]` and `apis.json` calls the whole
triplet a `RANK`. Two thirds of it is one; the third column is a *stride*, and
mapping it would be a wrong answer rather than a redundant one — a stride of
-1 is `MPI_ANY_SOURCE`'s ABI value and would reach MPICH as -2. Neither of the
two genuine ranks can be a sentinel, since both have to name a member of the
group, so the whole array passes through. Named table, with that reason.

**Six entry points are generated without a `MPIWRAPPER_HAVE_` guard**, the only
ones: `MPI_Status_get_source`/`_tag`/`_error` and the three setters read and
write named fields of the caller's own ABI status, which already holds the
ABI's encoding (§5.2). The implementation is not involved, so decision 6's stub
would be a regression rather than a fallback — they answer correctly over an
implementation too old to have the function.

### What S3's second half settled

S3b took the remaining 52 and closed the set: nothing is deferred. **565 of the
688 are generated, 118 are in the ledger, and 5 are answered by `libmpi_abi`
itself** — the entry points MPI-3.0 deleted, which is the section after next. The zero is kept as a
frozen tally, so that a future `apis.json` or ABI header carrying an argument
class the generator cannot place fails there rather than quietly emitting one
more stub.

**Where the callback boundary falls, stated as a rule rather than a list.**
Two of the 52 sat against §6.1's registrars and the split had been a matter of
memory. The rule is **a callback-typed parameter**, not the word "callback" in
a class name. `MPI_T_event_callback_get_info` and `_set_info` take a
`CALLBACK_SAFETY`, which is an enumerator naming a safety level — they convert
an enum and a registration handle and nothing else, so they are generated.
`MPI_T_event_handle_free` takes an `MPI_T_event_free_cb_function`, and
installing it means a trampoline that converts an implementation registration
handle and `cb_safety` back to the ABI's on the way *into* user code — §6.1's
mechanism and §6.2's lifetime question, the same judgement as the two
registrars beside it. So it moved into the ledger, which is why the
hand-written count went to 119. It needs no pool: its own `user_data`
parameter can carry the `{user_fn, user_extra}` pair. (It came back to 118 when
`MPI_Keyval_create` left the ledger; see the deleted-MPI-1 section below.)

**MPI_T lets a caller pass a null pointer for any OUT parameter**, and each of
the five query functions says so in its own words (15.3.6 through 15.3.9).
Nothing else in MPI works this way. A parameter that merely passes through
needs nothing — the null reaches the implementation, which is what the caller
asked for — but a *converted* one is written through a local and copied back,
and both halves have to become conditional or the copy writes through the null.
That includes `MPI_T_event_get_info`'s staged datatype array, where the
destination of the write-back is the null pointer itself. A named set of
routines, with the citation, rather than a rule in the emitter.

Emitting `TARGET(..., abi_x ? &x : NULL, ...)` would have been the short way
and it is refused: the generator's load-bearing assertion is that **no
ABI-typed parameter appears in the argument list of the implementation call**,
and that assertion is a grep. So the test is hoisted into a declared
`T *const x_p = abi_x ? &x : NULL;` and the assertion stays exactly as strong
as it was. A property worth a named local is worth not carving an exception
into.

**MPI_T's six handle classes are deliberately not the eleven's machinery.**
The eleven have up to 103 predefined values apiece, spelled in the
implementation as addresses that are not compile-time constants, which is what
§5.1's perfect-hash reverse map exists for. `MPI_T_enum`, `MPI_T_cvar_handle`,
`MPI_T_pvar_handle`, `MPI_T_pvar_session`, `MPI_T_event_registration` and
`MPI_T_event_instance` have at most *two* apiece, so each direction is one or
two compares — §5.3's sentinel shape. Keeping them out also keeps "handle
classes: 11" and "predefined handles: 103" the frozen tallies they were.

The sentinels really do need translating, which is what makes this more than a
bit-cast: the ABI fixes `MPI_T_PVAR_ALL_HANDLES` at 1, Open MPI 5.0.6 spells it
`-1`, and MPICH 4.3.1 declares it `extern struct MPIR_T_pvar_handle_s * const`
— a value that is not a constant expression at all, so it could not be a case
label anywhere and both directions have to be run-time compares. A dynamic
handle whose bits landed on 0 or 1 would read back as a sentinel, so the
`toabi` direction rejects it exactly as §5.1 does.

**Guarding them needed a probe for a *type*, not for a constant.**
`MPI_T_event_registration` has no null constant in the ABI header to test, and
Open MPI 5.0.6 has cvars, pvars and enums but declares neither event type. It
turns out `dev/probe_impl.py` already answers this: its constant probe emits
`sizeof (NAME)`, which is valid C for a type name too, so
`MPIWRAPPER_HAVE_MPI_T_event_registration` is asked and answered like any other
guard. That is now written down in the probe rather than left as a thing that
happens to work.

**Keyvals are the one mapped integer family with a dynamic half** (§5.6). The
thirteen predefined attribute keys convert through a generated switch like
every other family; a keyval the implementation handed out at run time cannot,
because it is an `int` with no structure and no pointer slack to tag around,
and it can land on top of the ABI's 501-507 or 601-605. So the switch's default
arm asks `src/mpiwrapper/keyvals.c` instead of passing the value through, and
the ABI-side value is *ours to choose* — drawn from a base far above any
predefined key, which makes "predefined or dynamic" a property of the value and
puts the collision beyond reach by construction rather than by luck. The
reverse direction scans newest-first, because an implementation may reuse the
number of a freed keyval and the entry that matters is then the most recent
registration. Nothing is ever removed, per §6.2. The registry is complete now
and `MPI_*_create_keyval` — its only writer — is S4's.

**Output string buffers split exactly where §5.8 said, and the table is what
decides it.** With an explicit length argument the caller bounds the write, a
`char *` is a `char *` on both sides, and buffer and length both pass through
untouched: that is `MPI_Info_get`, `MPI_Info_get_string`,
`MPI_Session_get_nth_pset` and MPI_T's twelve. Without one, an implementation
whose `MPI_MAX_*` exceeds the ABI's writes past the caller's array, and
truncate-or-error is a per-parameter judgement — those ten stay in the ledger.
The named `(routine, parameter)` table gives the length parameter rather than
deriving it, and the generator checks it against the signature, because that
naming is the *only* thing separating the safe class from the dangerous one.

**`obj_handle` is the one parameter whose class is not in its own argument
list.** MPI-5.0 15.3.6 makes it "an address to a local variable that stores the
object's handle", and which class of handle that is comes from the `bind` a
prior `get_info` reported. So the three allocators query it first —
`src/mpiwrapper/toolobj.c`, the same shape as `extents.c`: a `PMPI_` call the
wrapper makes for its own purposes, run before the call it serves, returning an
implementation error code the body maps like any other. Every OUT argument of
that query but `bind` is a null pointer, which is standard rather than a
liberty and avoids a real leak: `MPI_T_EVENT_GET_INFO` is *required* to return
a newly created info object when one is asked for, so asking would leak one per
handle allocation. The switch from `bind` onto a handle class is generated into
`constants.c` rather than hand-written, so that the `MPIWRAPPER_HAVE_` guards
its cases need are probed like every other — at the time, `dev/probe_impl.py`
read the generated sources and not `src/`, which S4a changed; the reason to
generate the switch stands, since it is a switch over handle classes and those
are the generator's.

### What S4a settled

S4a wrote 70 of the ledger's 110 remaining bodies — the converter face — and
five things it settled were not in the plan.

**`_toint`/`_fromint` are not `_c2f`/`_f2c` with a different integer type, and
the difference decides the implementation.** MPI-5.0 §20.4 puts the C-Fortran
converters *outside* the ABI ("the functions defined in Section 19.3.4 and
Section 19.3.5 … are not part of this ABI", because they depend on `MPI_Fint`),
so nothing pins what a Fortran handle value must be and forwarding to the
implementation's own converter — S1's choice for `MPI_Comm`, now the rule for
all eleven classes — is free. §20.4.5's serialization functions *are* part of
the ABI and pin it exactly: "for all predefined handles, the integer value must
be the same as the values listed in Section A", which is the ABI's own table.
So these 22 never call the implementation at all. Forwarding them would be
wrong even where an implementation has them.

**Hence `src/mpiwrapper/serialize.c`.** A dynamic ABI handle is the
implementation's own handle bits, and on Open MPI that is a 64-bit object
address that no cast to `int` can recover — the standard's own rationale names
the remedy, "using a lookup table or hash function". The table is an
append-only intern table with the shape of `keyvals.c`: predefined handles are
answered by a cast (the requirement above), every dynamic handle is interned
from a base far above the predefined range, and a repeat of the same bits
reuses its entry. Dynamic handles are interned **even on MPICH, where they
would survive a cast**, so that one code path is exercised on both
implementations rather than the interesting one running only where it is
needed — §5.8's discipline. A full table answers 0, which no `_fromint`
accepts, because these functions return a handle and have nowhere to report an
error.

**The four status converters are pure ABI-side, and §4.4 said so without quite
saying it.** The ABI's Fortran status is the ABI's C status — eight ints,
named fields at indices 0, 1 and 2, which §20.4.3 fixes and `MPIABI_F08_Status`
is a typedef of — so all four are a 32-byte copy with four `_Static_assert`s
behind them. Forwarding to the implementation's `MPI_Status_c2f` would be
actively wrong rather than merely roundabout: MPICH's Fortran status puts the
named fields at indices 2, 3 and 4, so a caller reading `status(MPI_F_SOURCE)`
out of the result would read a private byte. What makes the copy sound is that
the ABI status's 20 scratch bytes carry the implementation's own private bytes,
so a status that has been to Fortran and back still answers `MPI_Get_count` —
which is what `test/abi_converters_test.c` asks it.

**`dev/probe_impl.py` now reads `src/mpiwrapper/` as well as `gen/`.** It used
to read only the generated sources, on the reasoning that the generator decides
what needs asking. That stopped being true the moment a hand-written body
carried a `MPIWRAPPER_HAVE_` guard: the guard was silently false, and decision
6's stub would have reported `MPI_ERR_UNSUPPORTED_OPERATION` for an entry point
the implementation has. The rule it replaces the old one with is the one that
was actually meant — **whatever guards, gets probed** — and `CMakeLists.txt`
names the two source lists once so the probe's dependencies cannot drift from
the library's.

**Measured, not assumed: Open MPI 5.0.6's `MPI_Get_library_version` reports a
`resultlen` one larger than the string.** 119 for a 118-character string,
confirmed against it natively rather than through the wrapper. So §5.8's
contract is "the string terminates at the length reported", not "the length
reported is `strlen`" — the wrapper passes the implementation's own count
through, and the test asserts the former.

### The five entry points MPI-3.0 deleted, and why they are not slots

S3b's own exit check passed on every implementation and the *identity*
configuration still would not link. That is worth recording as a finding rather
than as a fix, because the cause is a limit of the probe and not a bug in
anything: **`libmpi_abi` from Open MPI main declares all 688 entry points and
defines 683.** The five it omits are `MPI_Attr_delete`, `MPI_Attr_get`,
`MPI_Attr_put`, `MPI_Keyval_create` and `MPI_Keyval_free` — the attribute
functions MPI-2.0 deprecated and MPI-3.0 deleted. Four had been deferred stubs
until S3b generated them, which is why nothing had noticed.

`dev/probe_impl.py` asks the *compiler* whether an entry point is available, on
purpose: §9 keeps it compile-only so the build stays cross-compilable, and `nm`
was rejected because a header may provide an entry point as a macro. Against a
conventional MPI, whose header and library agree, that is exact. Against an MPI
built from the ABI header it is not, and the failure mode is the worst
available: decision 6 promises that a missing entry point keeps its slot and
reports at run time, and instead the whole wrapper fails to **link**.

Two fixes were available and the second is better.

A **link stage** in the probe would fix the general case: linking is
compile-time, and cross-compilation forbids only *running*, so §9's constraint
survives. Done by bisection it needs no parsing of linker diagnostics at all,
only their exit status. It is still the right answer for any *future* entry
point an implementation declares and does not define, and it is not
implemented.

But for these five it treats the symptom. They are not entry points an
implementation happens to lack; they are entry points the standard **removed**,
and each is exactly the MPI-2 function that replaced it under an older name. So
`libmpi_abi` implements them itself, in terms of the replacements, and they get
no vtable slot and no wrapper body at all. What that buys is larger than the
link: they now work over *any* implementation with the MPI-2 attribute
interface, which is all of them, rather than over the ones that kept the MPI-1
spelling — and it is the first case where the right answer was to stop
forwarding rather than to forward more carefully.

Details that make it exact rather than approximate:

- **The set is closed by the header, not by memory.** It is precisely the entry
  points the ABI header marks `deprecated: MPI-2.0`, and the generator fails if
  a sixth appears without a replacement named for it. MPI-3.0's other deletions
  — `MPI_Address`, `MPI_Type_extent`, `MPI_Errhandler_create` and the rest —
  are not in the ABI header at all, so there is nothing to do for them.
- **The equivalence is checked, not asserted**: return type, arity and every
  parameter type. The one place the two differ is `MPI_Keyval_create`'s
  callbacks, where `MPI_Copy_function` and `MPI_Comm_copy_attr_function` are
  two names for one function type — compared by what they expand to rather than
  by spelling, so an ABI that changed either fails generation instead of
  producing a forwarder that miscalls.
- **The sentinels agree, and that is tested at run time.** `MPI_NULL_COPY_FN`
  and `MPI_COMM_NULL_COPY_FN` are both `0x0`, `MPI_DUP_FN` and `MPI_COMM_DUP_FN`
  both `0x1`, `MPI_NULL_DELETE_FN` and `MPI_COMM_NULL_DELETE_FN` both `0x0`, so
  the pass-through is exact and not merely well-typed. A cast to a pointer type
  is not an integer constant expression, so this one cannot be a
  `_Static_assert` (the same reason constants.c switches handles on numeric
  labels) and lives in `test/abi_tools_test.c` instead.
- **The shifted-name rule survives the rename.** `MPI_Attr_get` reaches the
  implementation's `MPI_Comm_get_attr` and `PMPI_Attr_get` its
  `PMPI_Comm_get_attr`, so §2's reason for two slots per entry point — that
  bypassing profiling at the ABI level also bypasses it at the implementation
  level — still holds across the substitution.
- **`MPI_Keyval_create` left the ledger.** It was there as a callback registrar
  (§6.1); it is `MPI_Comm_create_keyval` under an older name, so the trampoline
  judgement now lives in exactly one place instead of two, and S4 has one fewer
  body to write. The ledger is **118** again.
- **The slots really are gone**, rather than kept and left unfilled. That is
  what makes an old `libmpi_abi` paired with a new `libmpiwrapper` a clean
  refusal: the layout hash covers the slot list, so the pairing fails the
  handshake instead of reaching a slot that no longer means what it did.

### Reproducing the prototype

`dev/s1-reference/` holds S1's four hand-written stand-ins, frozen, and
`dev/check_prototype.py` compares them against the generated output item by
item as the `prototype-reproduced` test. **194 items, 190 reproduced exactly,
4 exempted with a reason** — and an exemption that stops firing fails the test,
so the list cannot outlive its reason.

Comparison is over normalized text (comments dropped, macro continuations
joined, whitespace collapsed): the generator does not run `clang-format` and
does not write S1's per-function prose, so a byte comparison would fail on
formatting and say nothing about the code. In practice the simple bodies *are*
byte-identical, alignment included, because the emitter reproduces
`clang-format`'s two alignment rules directly.

The one declared rewrite is the guard mechanism: S1 wrote
`#ifdef <the implementation's own name>` around an optional constant, S2 asks
the probe instead, and the reference's guards are rewritten to the probe's
spelling before comparing. Declaring it once is narrower than exempting the
dozen table functions it touches, since an exemption stops checking an item
altogether — which cases are guarded, and what each maps to, is still compared
exactly.

The four exemptions:

- **`MPI_Comm_rank`.** S1 passed the out rank through, reasoning per-site that
  a process's rank in its own communicator is never a sentinel. True here, false
  one function away — `MPI_Group_rank` answers `MPI_UNDEFINED` — so the
  generator maps every out-rank uniformly. Uniformity is the correctness
  property; per-site reasoning is what a generator must not encode.
- **`MPI_Type_create_struct` and its `_c` form.** Identical but for two
  identifiers: the generator names a staging buffer `<local>_stack` where S1
  wrote `typestack` (and `reqstack`, `ststack` in `MPI_Waitall`). Abbreviating
  per site is not a rule. The `_c` form also lost S1's `#if MPI_VERSION >= 4`
  to the availability probe, which is the same guard made exact.
- **`MPI_Waitall`.** Not generated: an inout request array whose staged
  temporaries are released *at completion* is S3's class. S1's body moved to
  `src/mpiwrapper/handwritten.c` and is named in the ledger with that reason,
  so the slot stays filled and the test that exercises it still passes. S3
  deletes it, and the exemption with it.

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

Predefined handle constants occupy `0x00000020`..`0x000002eb` — **103** values,
all < 748. (Counted out of `gen/include/mpi.h`, and equal to the number of
`PREDEF(...)` rows in `gen/mpiwrapper/constants.c`: 71 datatypes, 15 ops, 4
errhandlers, 3 comms, 2 each of group/info/message, 1 each of
file/request/session/win. Earlier drafts said 104 in this section and in §10
while §5.1 and §11 said 103; 103 is the checkable number.)

### 4.2 Status layouts

| | layout | sizeof (64-bit) | sizeof (32-bit) | private bytes |
|---|---|---|---|---|
| ABI | `int MPI_SOURCE, MPI_TAG, MPI_ERROR; int MPI_internal[5]` | 32 | 32 | **20** |
| MPICH | `int count_lo, count_hi_and_cancelled, MPI_SOURCE, MPI_TAG, MPI_ERROR` | 20 | 20 | 8, at the front |
| Open MPI | `int MPI_SOURCE, MPI_TAG, MPI_ERROR, _cancelled; size_t _ucount` | 24 | **20** | 12 / 8, at the back |

Open MPI's shrinks on 32-bit because `_ucount` is a `size_t` (measured, not inferred
— `arm32v7` says 20). MPICH's is 20 either way. Nothing depends on the exact number;
what matters is the invariant **`sizeof(impl status) <= 32`**, and more precisely
that the private part fits the ABI's 20 scratch bytes. Both do, with room to spare —
Open MPI's 24 is its *total* size and only 12 of it is private, so a design that
read 24 as "does not fit" would reject the simple scheme for no reason.

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
is not: the named fields move. 8 >= 5 and 8 >= 6, so an mpif status
buffer sized by the ABI constant is never too small.

**S4a resolved what that makes the four converters do**, since the sentence
above admits two readings and only one of them is right. They do *not* forward
to the implementation's own `MPI_Status_c2f`: that produces a valid status in
MPICH's layout, where `MPI_F_SOURCE` — an ABI constant, fixed at 0 by MPI-5.0
§20.4.3 — indexes a private byte. They are a 32-byte copy between two spellings
of the same thing, and the implementation is not involved. The 20 scratch bytes
carry its private status bytes through the copy, so the result still answers
`MPI_Get_count`; §3's "What S4a settled" has the rest.

---

## 5. Conversion rules

### 5.1 Handles

ABI -> implementation is a dense `switch` over `0x20`..`0x2eb` (which the compiler
turns into a jump table), else a bit-cast.

Implementation -> ABI needs the reverse: predefined implementation handle values are
*not* compile-time constants in general (Open MPI's are addresses), so the map is
built at initialization inside `mpiwrapper_get_vtable`. It is a **perfect hash** —
the whole key set is known by then, so a multiplier is searched for until no two keys
collide, making a lookup one multiply, one shift, one load and one compare with no
probe loop.

`dev/handle-map-bench/` measured the alternatives against the *real* 77 predefined
MPICH datatype values and against Open MPI-shaped addresses (ns per lookup):

| | mpich hot | mpich sweep | ompi hot | ompi sweep |
|---|---|---|---|---|
| perfect hash | **1.104** | **1.093** | **1.103** | **1.085** |
| open-addressing hash | 1.094 | 1.355 | 1.099 | 1.532 |
| sorted + binary search | 3.879 | 3.724 | 3.893 | 3.717 |
| sorted + interpolation search | **88.067** | 82.523 | 1.367 | 1.633 |

**Sorted arrays are not faster.** Binary search costs 3.4x — seven dependent,
unpredictable comparisons. Interpolation search, the O(log log n) idea, is a *trap*
on the distribution that actually occurs: it assumes uniform keys, and MPICH's are one
value at `0x0c000000`, a dense cluster at `0x4c00xxxx`, and one at `0x8c000004`, where
it degenerates toward a linear scan with a floating-point divide per step and runs
eighty times slower. On Open MPI's uniform addresses it is fine, and still no better
than a hash.

The perfect hash also beats the open-addressing hash originally designed here —
1.36-1.53 -> 1.09 ns whenever the datatype varies between calls — because removing the
probe loop removes the only data-dependent branch. Construction must be bounded and
must fail **loudly at initialization** rather than degrading to probing at run time,
which would put that branch back: widen and retry, then refuse in
`mpiwrapper_get_vtable` with a diagnostic.

**Collision.** A bit-cast dynamic implementation handle is wrong if it lands in
`0x20`..`0x2eb`. It never does today: MPICH's handles carry a kind field in the
high bits so all real handles are >= 0x04000000, and Open MPI's are object
addresses. But cross-compiling forbids probing this at configure time, and 32-bit
targets have no spare high bits for a tagging scheme. So: **check in the `toabi`
direction only** — that is object creation, not every `MPI_Send` — and fail with
`MPI_ERR_INTERN`. A test also probes it at run time (§10).

**Two things S1 added to both directions:**

- **The `fromabi` default arm needs the range test too.** A value inside
  `0x20`..`0x2eb` that reached the default arm is an ABI predefined handle *this
  implementation does not provide* — the sized Fortran types are the realistic
  case — and bit-casting it hands the implementation a fabricated handle, which on
  MPICH is an `int` whose kind bits it will read. Returning the class's null handle
  instead makes the implementation reject the call with its own error code. The
  test is free: a dense switch has to bounds-check anyway.
- **Aliasing in the reverse map is normal and is not a collision.** An
  implementation may give two ABI-distinct predefined handles the same value —
  MPICH answers `MPI_DATATYPE_NULL` for five of the optional sized types, so 103
  ABI handles map onto 98 distinct MPICH values. The perfect-hash construction
  therefore distinguishes *same key inserted twice* (an alias: keep the first,
  which makes the ABI's own order canonical) from *different keys in one slot* (a
  real collision: retry with another multiplier). Conflating them would make map
  construction fail on MPICH and take the wrapper down at load.

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

S3 generates those six, and they are the only generated bodies emitted without
a `MPIWRAPPER_HAVE_` guard: the field they read or write is in the caller's own
ABI status and already carries the ABI's encoding, so nothing about them
depends on the implementation having the entry point — decision 6's stub would
turn a working answer into `MPI_ERR_UNSUPPORTED_OPERATION` over an MPI-3.1
implementation for no reason.

### 5.3 Sentinels

Pointer values with special meaning (`MPI_BOTTOM`, `MPI_IN_PLACE`,
`MPI_BUFFER_AUTOMATIC`, `MPI_UNWEIGHTED`, `MPI_WEIGHTS_EMPTY`,
`MPI_STATUS(ES)_IGNORE`, `MPI_ARGV(S)_NULL`, `MPI_ERRCODES_IGNORE`) are fixed in
the ABI and may be non-constant in the implementation — possibly `extern void *`,
i.e. constant at link time but not at build time. Translated the same way as
handles: one test per site.

Which sentinels are legal is a property of the *parameter*, not of the type, so
choice buffers get four mappers rather than one: send and receive (the C type's
constness decides which), each with and without `MPI_IN_PLACE`. `apis.json`
does not record where `MPI_IN_PLACE` is allowed — every choice buffer is just
`BUFFER` — so that is a named table in the generator, and it needs the receive
form because `MPI_Scatter` and `MPI_Scatterv` take `MPI_IN_PLACE` at the
*receive* buffer (§3).

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
and the `assert` argument of `MPI_Win_post`/`_start`/`_fence`/`_lock`.

The ABI puts file modes (1..256) and window asserts (1024..16384) in one enum with
disjoint bits, and an earlier draft concluded from that that a single bitmask
mapper serves both roles. **It does not, in the out direction, and S1 measured
why:** Open MPI numbers its window asserts

```
MPI_MODE_NOCHECK 1  NOPRECEDE 2  NOPUT 4  NOSTORE 8  NOSUCCEED 16
```

which are *the same bits* it gives `MPI_MODE_CREATE`, `RDONLY`, `WRONLY`, `RDWR`
and `DELETE_ON_CLOSE`. An implementation-side `1` is `CREATE` or `NOCHECK`
depending only on which parameter it came from. So the role belongs in the
function name, exactly as for ranks and tags (§5.4): `filemode_fromabi`/`_toabi`
and `winassert_fromabi`/`_toabi`, chosen per parameter from `apis.json`.

MPICH keeps the two families disjoint (asserts at 1024+), which is what made the
single mapper round-trip there — one more case where the same code passes every
test on one implementation and is wrong on the other. The out direction is not
hypothetical: `MPI_File_get_amode` returns one.

### 5.6 Keyvals and dynamic error codes

Both are plain `int`s handed out by the implementation at run time, and both can
collide with ABI predefined values.

- **Keyvals.** ABI predefined values are at 501-507 and 601-605; Open MPI hands
  out small sequential ints, which could in principle reach 501. No pointer
  slack is available, so dynamic keyvals need an additive bias or a high-bit
  tag. **S3b built it** (`src/mpiwrapper/keyvals.c`): the ABI-side value is
  ours to choose, so it is drawn from a base far above any predefined key, and
  the generated switch's default arm hands off to the registry instead of
  passing the value through. The reverse direction scans newest-first, because
  an implementation may reuse the number of a freed keyval. Its only writer is
  `MPI_*_create_keyval`, which is S4's.
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

**And note where each sentence puts the obligation, because the natural reading is
the wrong way round.** Both put it on the *user*, not on the implementation. There
is no rule that an implementation must copy the counts, displacements and datatype
arrays out at initiation — it is explicitly permitted to keep reading them until
the operation completes, and §6.12's own advice to implementors (nonblocking
collectives "implemented with local execution schedules ... using nonblocking
point-to-point communication") describes exactly the kind of design that would. A
search of MPI-5.0 for any statement that the arrays are copied finds none.

That is what forces the request-keyed table, and it is the whole reason it exists:
the arrays the implementation reads are *ours*, not the caller's, so they must
survive as long as the implementation may read them — until completion for the
nonblocking forms, until `MPI_REQUEST_FREE` for the persistent ones. If the
standard did require the implementation to copy, every one of these temporaries
could be scoped to the call and §6.3's table, its tombstones and its lock word
would all be deletable.

A side effect worth knowing, since it is a behavioural difference from a native
MPI rather than a conformance question: because *we* copy at initiation, an
application that violates the rule and modifies its ABI-side arrays after posting
still gets the right answer through the wrapper, where it might not natively. We
are more permissive than the standard requires, in the direction that hides a user
bug. Nothing to fix; worth knowing when a program works over the ABI and fails
without it.

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
  use-after-free on the second `MPI_Start`. `MPI_Waitall` can mix both kinds.

  **S3 found that this needs no flag in the request map**, which is what this
  paragraph said until then. The two lifetimes have one discriminator, and the
  implementation maintains it: a request it has set to `MPI_REQUEST_NULL` is
  finished with, and it nulls a nonblocking request at completion and a
  persistent one at `MPI_Request_free` — and *not* at a persistent request's
  completion, which is precisely the case an early free would corrupt. So
  "release the pre-call handle wherever the post-call handle is null" is the
  whole rule, it is emitted identically at every completion site, and there is
  no per-entry state for the generator to get wrong. S1's `MPI_Waitall` already
  read this way; `test/abi_arrays_test.c` starts a persistent `MPI_Alltoallw`
  three times over, which is the shape a flag-free wrong answer breaks on.
- **`MPI_STATUSES_IGNORE`** (NULL in the ABI) must short-circuit before any
  temporary is allocated.
- **Stage for value mapping or for representation, never for spelling.** An
  earlier version of this list said to stage any array whose element type was not
  *identical* on both sides, on the grounds that the ABI's `MPI_Aint` and an
  implementation's may be distinct C types. That reasons from the language, and
  this project implements a **system ABI**: what an ABI fixes is representation.
  `dev/type-identity/` measures it — size and signedness are identical for
  `MPI_Aint`, `MPI_Count`, `MPI_Offset` and `MPI_Fint` in every implementation
  and platform tried, and where the *spellings* differ (glibc's `int64_t` is
  `long` while both MPIs spell `MPI_Count` as `long long`) the cost is a cast and
  nothing else. The cast reads correctly at `-O0`, `-O2` and `-O3` with
  `-fstrict-aliasing` even in the single-translation-unit worst case, where the
  optimizer can see the stores and the loads together and apply TBAA; in the real
  system the implementation is a separate shared library that cannot see our
  stores at all. `MPI_Aint` is `long` on both sides everywhere, so the example
  that prompted the original rule does not even arise.

  What licenses the cast is the `_Static_assert` battery in `internal.h`: a
  target whose representations genuinely differed would fail the build rather
  than reach this argument. So `array_of_displacements` and the `_c` forms'
  count arrays are passed straight through, and only the datatype arrays beside
  them are staged.

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
`(routine, parameter)` table. The *other* set — everything with an explicit
length argument — is a named table too (`STRING_OUT_LENGTH`), naming the
parameter that bounds each buffer and checked against the signature, because
that naming is the only thing that separates the safe class from this one:

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

**What `*resultlen` means, measured.** "Set `*resultlen` to what was actually
copied" above is the contract, and the tempting stronger reading —
`strlen(string) == *resultlen` — is not one implementations honour: Open MPI
5.0.6's `MPI_Get_library_version` reports **119 for a 118-character string**,
confirmed against it natively rather than through this wrapper. So the property
to hold and to test is `string[*resultlen] == '\0'`, with the implementation's
own count passed through unchanged; asserting the equality would fail on a
correct wrapper over a real MPI.

### 5.9 When to assert at compile time

**Static-assert where a runtime check would cost something on a hot path; handle at
run time where the check is free.**

But "handle it rather than fail the build" is not universal, and the discriminator is
**whether the degraded behaviour announces itself.** A truncated error string is
visibly truncated, so handling `MPI_MAX_*` at run time is right. By contrast, an
earlier draft proposed a configure probe that would silently fall back from `PMPI_X`
to `MPI_X` where the shifted name was missing (§2); *that* degradation is invisible,
and would have quietly reintroduced the defect the separate slot exists to prevent.
There a link error naming the missing symbol is the better outcome — and it costs
nothing, since both names always exist. So: handle it at run time when the fallback
is observable, fail the build when it is not.

| | why |
|---|---|
| `MPI_MAX_*` | cold paths only, and truncation is visible -> run time |
| `sizeof(MPI_Count)`/`MPI_Aint`/`MPI_Offset` | a narrowing check would land on `MPI_Send_c` and every large-count call -> `_Static_assert` |
| status layout | **no runtime recourse exists** — nowhere to put a private part exceeding 20 bytes, and side storage keyed on a status address is unsound because statuses are freely copied -> build failure |
| dynamic handle collision | one compare, and only on object creation -> run time |

---

## 6. Callbacks

### 6.1 Which need trampoline pools

Seven typedef families, **16** registration functions. The table below sums to
15 and an earlier draft said 16 here and in §6.2 without the extra one ever
being named; S3b found a sixteenth on its own terms and named it — see the note
after the table, and `MPI_T_event_handle_free` in §6.2's. The ones with an
extra-state
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

**A sixteenth registrar, and the rule that finds it.** S3b added
`MPI_T_event_handle_free` to this set, which the table above did not name. The
discriminator is **a callback-typed parameter**, not the word "callback" in an
argument class: `MPI_T_event_callback_get_info` and `_set_info` carry a
`CALLBACK_SAFETY`, which is an enumerator naming a safety level and converts
like any other enum, so those are generated. `MPI_T_event_handle_free` carries
an `MPI_T_event_free_cb_function`, which runs on the way back *into* user code
and must therefore convert an implementation registration handle and
`cb_safety` to the ABI's — a trampoline, and the same judgement as the fifteen
above. It needs no pool: its own `user_data` parameter carries the
`{user_fn, user_extra}` pair.

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
15:

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

**That hash is keyed on the implementation's request handle, and such a handle does
not uniquely identify an operation.** That is the assumption the whole table rests
on — `attach(request, block)` and the `release(request)` that every completion call
performs both find the entry by handle value — and `dev/request-identity/` measures
it false on both implementations, in two independent ways:

| | MPICH 4.3.1 | Open MPI 5.0.6 |
|---|---|---|
| `MPI_Isend` to `MPI_PROC_NULL` x4 | `0x6c000001` x4 | `0x1013f7920` x4 |
| `MPI_Ibarrier` on `MPI_COMM_SELF` x4 | `0x6c00000b` x4 | `0x1013f7920` x4 |
| `MPI_Ialltoallw` on `MPI_COMM_SELF` x4 | distinct | distinct |
| a completed handle, then a new operation | value reused | value reused |

**Shared built-in requests.** An operation that is already complete on return does
not need a per-operation object, and neither implementation allocates one: MPICH
has one built-in per operation kind, Open MPI a single `ompi_request_empty` shared
across all of them. The important part is that `MPI_Ibarrier` is in that list — the
shortcut is not confined to point-to-point, and nothing in the standard stops an
implementation from applying it to a zero-work `MPI_Ialltoallw`, which is precisely
the family that stages temporaries. A legal program that posts two such calls
before waiting would then hand us the same key twice.

**Recycling.** Once an operation completes, its handle value is handed out again
immediately. So a completion we fail to observe does not merely leak: it leaves a
stale entry whose key a later operation will be given.

Only a short list of entry points ever attaches — `MPI_Ialltoallw` in S1, its
family and the persistent `_init` forms in S3 — so the `MPI_PROC_NULL` rows above
are not attach collisions; `MPI_Isend` stages nothing. They reach the table
through the other door: every completion call releases *by handle value*, so a
wait on a `MPI_PROC_NULL` `MPI_Isend` does look its key up, and where an
implementation shares one object across kinds it can free a block belonging to a
different operation.

That is safe for a reason worth stating, because it is what bounds the whole
scheme: MPI requires every request to be independently testable and completable,
so two *live* operations cannot share a handle. Sharing implies the handle carries
no per-operation state, which implies both operations are already complete — and a
block freed then is a block nobody is reading.

Hence the two rules the table follows. It **refuses a second block for a key it
already holds** rather than overwriting — overwriting leaks the first block, and
freeing it would be indefensible even though the argument above says nothing is
reading it —
and *every* completion entry point must release, not just the ones an author
happens to think of (`MPI_Wait`, `Waitall`, `Waitany`, `Waitsome`, `Test`,
`Testall`, `Testany`, `Testsome`, `Request_free`, and the persistent forms, which
release at `MPI_Request_free` rather than at completion). S1 implements
`MPI_Waitall` only; S3 owes the rest, and that list is a ledger item rather than a
matter of memory.

**S3 discharged it, and by construction rather than by list**: the generator
emits the release wherever an entry point has an *inout request* parameter,
which is those eleven and nothing else — `MPI_Start` and `MPI_Startall` are in
the set too and never fire it, because they null nothing. §5.7 records why the
same emitted rule serves both lifetimes.

Refusing is safe but not free: it answers `MPI_ERR_INTERN` to a legal call. It is
not a memory-safety problem — a shared built-in request is by construction already
complete, so nothing is reading the block when it is freed — which points at the
fix for S3: probe the built-in request values once at initialization (a
`MPI_PROC_NULL` `MPI_Isend` and an `MPI_Ibarrier` on `MPI_COMM_SELF` reveal them,
and a runtime probe is available to us where a configure-time one is not), and when
a staged operation returns one, free the block immediately instead of attaching it.
The operation it belonged to has already finished.

None of this touches the callback pools, which are keyed on a slot index we hand
out ourselves, or the predefined-handle maps, which are built once from values that
do not change.

Two details of the same table are worth recording because they are easy to get
subtly wrong: released entries leave a **tombstone** rather than an empty slot, or
they would truncate the probe chain of some other key; and a release **claims the
entry before clearing it** (key -> LOCKED, clear block, then publish TOMBSTONE), or
a concurrent attach could take the entry between the releaser reading the key and
clearing the block, and the releaser would free the new owner's block.

---

## 7. Decisions

1. **Conversions live in `mpiwrapper`, behind an ABI-typed vtable.** §2.
2. **Status: blob only** — no validity marker, no synthesis fallback. §5.2.
3. **The ABI surface is complete MPI-5.0**; functions the implementation lacks are
   reported at run time, never omitted. The implementation is *expected* to provide
   the MPI-4.0 API, which is what makes the mapping 1:1 — an expectation, warned
   about at configure time and not enforced, since no released Open MPI meets it.
   The enforced floor is **MPI-3.0**, verified with MPICH 3.1.4. §1, §9.
4. **`mpiwrapper` exports exactly one symbol**, a getter carrying
   `MPI_ABI_VERSION`, `MPI_ABI_SUBVERSION`, a generated layout hash and the
   vtable's `sizeof`. **All four are checked for exact equality**; there is no
   prefix serving and no forward compatibility, and the size is what catches a
   layout mismatch the text-derived hash cannot. §2.
5. **`mpi_abi` finds the wrapper from an environment variable**, falling back to a
   build-time path. §2. **Both libraries are built together into one prefix per MPI
   installation**, and the split is not user-visible; wrapper libraries are *not*
   name-tagged by MPI. **That prefix is exclusive** — no second wrapper, no other
   MPI, and never the wrapped MPI's own prefix, since we install `mpi.h`,
   `mpicc` and `libmpi_abi` under names it already uses. §9.
6. **Functions the implementation lacks return `MPI_ERR_UNSUPPORTED_OPERATION`**
   from generated `#ifdef` stubs, and the generator reports them. What the
   `#ifdef` tests is `MPIWRAPPER_HAVE_<name>`, written at configure time by
   `dev/probe_impl.py` from the implementation's own header. Not a
   version test: Open MPI 5.0.10 reports MPI-3.1 and has sessions. §3.
7. **PMPI gets its own vtable slots** (1376, not 688), calling the
   implementation's shifted names directly — no probe and no fallback, since both
   names always exist and reach the same code when nothing is interposed. (Which
   of the two is the strong definition varies by implementation and platform and
   does not matter to us. §2.) The wrapper's internal MPI calls use `PMPI_*`
   unconditionally. §2.
8. **Bootstrap by constructor into a plain pointer** — no atomic, no lazy-init
   branch, no NULL check outside debug builds. The wrapper is loaded `RTLD_LOCAL`
   and *isolated* — `RTLD_DEEPBIND` on Linux (`dlmopen` selectable but unusable
   with any MPI that `dlopen`s components), the two-level namespace on macOS —
   never `RTLD_GLOBAL`, and `RTLD_LAZY` by default. The wrapper then proves at
   load that its `MPI_*` calls resolved outward, by address *and* behaviourally.
   §2.
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
17. **Prototype before writing the generator.** Planned at sixteen entry points;
    S1 delivered 29, because the original list was not testable on its own. §11.
18. **The predefined-handle reverse map is a perfect hash**, built at
    initialization, failing loudly there rather than degrading to a probe loop.
    Sorted arrays are slower, and interpolation search is far slower on the real key
    distribution. §5.1.
19. **Ship `mpicc`, CMake package files (including a `FindMPI` shim) and
    pkg-config**, each exercised by CI rather than merely parsed. §9.

---

## 8. The hand-written set

Functions where per-function judgement is needed. The generator's `HAND_WRITTEN`
ledger names them and fails if the two sets do not together cover all 688.

**The count in this section was "roughly 50" and the list does not support it.**
Adding the bullets up gives ~100 entry points, and the gap is not decorative: it
is what S4 is sized against, and §3's "roughly 640 are mechanical" is its
complement. Two of the bullets are what makes the difference, and each is a real
question rather than a counting slip:

- **Buffers are 18 entry points, not four** — `MPI_Buffer_attach`/`_detach`/
  `_flush`/`_iflush`, the `MPI_Comm_` and `MPI_Session_` variants of each, and
  the `_c` forms. Only the attach/detach forms need judgement
  (`MPI_BUFFER_AUTOMATIC`, and the buffer's ownership); the six flush forms are
  mechanical and belong to the generator.
- **The staged-temporary forms are claimed by two stages at once.** This section
  lists them as hand-written (S4) while §3's argument-class table, §5.7's
  lifetime rules and STAGES.md's S3 all treat staging as a *generated* class.
  S1 hand-wrote `MPI_Ialltoallw` because the generator did not exist yet, which
  is not an argument for hand-writing the other seven. The set is exactly eight:
  `MPI_Ialltoallw`, `_c`, `MPI_Ineighbor_alltoallw`, `_c`,
  `MPI_Alltoallw_init`, `_c`, `MPI_Neighbor_alltoallw_init`, `_c` — small
  enough, and uniform enough, that generating them is the better answer.
  **Resolution: they are S3's, generated, and this section keeps only the shape
  S1 wrote as the template.**

With those two settled the list below is what remains, and it adds up to **89**,
not ~50.

**S2 exists now, so the tally is the generator's and it is 120**, in
`HAND_WRITTEN` in `dev/generate.py` with a reason on every line and in
`gen/report.txt` grouped by reason. The list below is still what the set is
*for*; three groups differ from it, and each is a decision rather than a
counting slip:

- **`MPI_Wtime` and `MPI_Wtick` are generated**, not hand-written. This section
  lists them under "no error code to map", but a `double` return with no error
  code is mechanical — and S1 put `MPI_Wtime` in `wrappers.c` rather than
  `handwritten.c`, which is the artifact that decides it.
- **The MPI-5.0 `_toint`/`_fromint` handle converters are hand-written too**, so
  the converter group is 44 rather than 22. They are the same conversion against
  a different integer type, and this section predates them.
- **`MPI_Remove_error_class`/`_code`/`_string` join `MPI_Add_error_*`**: they are
  the other half of §5.6's dynamic error-code registry.

**S3's first half took `MPI_Waitall` and `MPI_Ialltoallw` back**, which S2 had
kept here as S1's stand-ins for the two array classes it could not yet
generate, so the ledger is **118**. Both are generated now, with every other
member of their families, and `src/mpiwrapper/handwritten.c` is down to S1's
eight.

**S4a wrote 70 of them**, so the ledger's **118 entries hold 78 bodies and 40
stubs**. The 70 are the converter face: the 44 handle converters and the four
status converters (`src/mpiwrapper/hw_converters.c`, with `serialize.c` behind
`_toint`/`_fromint`), the ten status-consuming functions (`hw_status.c`), the
ten output-string buffers with no length argument (`hw_strings.c`) and the six
`MPI_Abi_*` calls (`hw_abi.c`). Four of S1's eight moved into those files with
their families; what stays in `handwritten.c` is what S4b finishes. Two of the
bullets below were decided rather than merely implemented, and §3's "What S4a
settled" has both: `_toint`/`_fromint` are ABI-side and not converters, and the
four status converters are a copy and not a forward.

**S3's second half added `MPI_T_event_handle_free`** and took
`MPI_Keyval_create` away again, so the ledger is **118**. Nothing is deferred
any more, so it is now the whole of what the generator does not emit, beside
the five §3 records as answered by `libmpi_abi` itself. The addition
is the sixteenth callback registrar of §6.1 — the rule that finds it is a
callback-typed parameter, which the two `MPI_T_event_callback_*_info` calls do
not have and this one does.

- **Bootstrap and lifecycle** (8): `MPI_Init`, `MPI_Init_thread`, `MPI_Finalize`,
  `MPI_Abort`, `MPI_Initialized`, `MPI_Finalized`, `MPI_Session_init`/`_finalize`.
- **No error code to map** (2): `MPI_Wtime`, `MPI_Wtick` return `double`.
- **The ten status-consuming functions** of §5.2, plus the four Fortran status
  converters (14).
- **Callback registration** — 15 of the 16 of §6.1, each installing a
  trampoline or a pair. `MPI_Keyval_create` is the sixteenth and is not here:
  it is `MPI_Comm_create_keyval` under a name MPI-3.0 deleted, so `libmpi_abi`
  forwards it and the judgement is made once.
- **Genuinely variadic** (1): `MPI_Pcontrol`.
- **Dynamic error codes** (3): `MPI_Add_error_class`, `_code`, `_string`.
- **Spawn** (2): `MPI_Comm_spawn`, `_multiple` (`argv`, `array_of_argv`,
  `array_of_errcodes`).
- **Buffer attach/detach** (12): `MPI_Buffer_attach`/`_detach`, the
  `MPI_Comm_`/`Session_` variants and the `_c` forms, including
  `MPI_BUFFER_AUTOMATIC` where the implementation has it.
- **The 22 Fortran handle converters**, which are the reason mpif can run over any
  MPI.
- **`MPI_File_get_view`** (`datarep` truncation) and the other nine output-string
  functions of §5.8 (10).

---

## 9. Building

### Repository layout

```
dev/               the Python generator and dev-time cross-checks
                     generate.py (the generator), generate_headers.py (the S0
                     step it imports), layout_hash.py, probe_impl.py
                     (the configure-time availability probe), check_prototype.py
                     apis.json (vendored), check-c-bindings.py (Appendix A.2)
                     and the five probes whose results this file cites
dev/s1-reference/  S1's four hand-written stand-ins, frozen; not compiled.
                     What check_prototype.py measures the generator against
gen/               committed generated output, never hand-edited
src/mpi_abi/       hand-written: bootstrap, dlopen, vtable acquisition
src/mpiwrapper/    hand-written: the ledger's bodies, trampolines, maps,
                     status conversion
examples/          narrated excerpts of each shape; src/ is the reference (§3)
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

**`cmake && make install` builds both libraries against the MPI it finds, into one
prefix.** The two-library split is an *internal* matter and must not be user-visible:
a user configures against an MPI and gets a working `mpi.h`, `libmpi_abi`,
`libmpiwrapper`, compiler wrapper and package files. Nobody should have to know that
`libmpi_abi` does not itself need MPI.

That property is still real and still valuable internally — it is what makes the
cross test possible (§10) and what lets one `libmpi_abi` be pointed at another
wrapper — but it is exposed only as a developer option
(`-DMPI_ABI_BUILD_WRAPPER=OFF`), not as part of the normal flow.

**One prefix per MPI installation, and the prefix is exclusive.** Everything a
build produces — `mpi.h`, `libmpi_abi`, `libmpiwrapper`, `mpicc` and the package
files — goes into one prefix, and that prefix holds **nothing else**: not a
second wrapper, not another MPI implementation, and in particular **not the MPI
this build wraps**.

The reason is file names, and it is not a matter of taste. Everything in that
list is a name the wrapped MPI also installs: `mpi.h` against `mpi.h`, `mpicc`
against `mpicc`, and — because MPI-5.0 §20.2.1 requires a library implementing
the standard ABI to be named `mpi_abi` — `libmpi_abi` against `libmpi_abi`. So
installing beside the MPI does not merely risk a clash, it *is* one, on the two
files that decide which MPI a consumer compiles against. An earlier draft said
"ideally beside the MPI it wraps, so the paths baked into it and the module
environment that produced it stay together"; that reads well and is wrong.
Colocation buys nothing the baked-in wrapper path and the module environment do
not already give, and it costs the two headline files.

It also breaks the build that produced it: §9's no-self-wrapping check refuses to
configure when `find_package(MPI)` lands on a prefix containing our
`mpiwrapper_vtable.h`, so an installation made into the MPI's own prefix turns
that MPI into one this project can no longer be built against. The failure
arrives at the *next* configure, on someone else's machine.

An earlier draft also proposed encoding the MPI in the
library's *name* (`libmpiwrapper-mpich-4.3.so`) so several could share a prefix; that
is dropped, and the exclusive-prefix rule is what replaces it. On real HPC systems the things that make two wrappers incompatible are
mostly not in the name — loaded modules, compiler and its runtime, fabric libraries,
MPI build options — so the name would give a false sense of safety while adding
complexity, and would catch few real errors. The version-and-layout handshake in
`mpiwrapper_get_vtable` is what actually catches mismatches, and it does so at load
time rather than by convention.

**Four configure-time checks for the wrapper, all compile-only** so
cross-compiling works:

1. `MPI_VERSION >= 3`, **hard**, and `>= 4` as a **warning**. An earlier draft made
   MPI-4.0 the hard floor; S1 found that no released Open MPI clears it (5.0.10
   reports MPI-3.1 and ships no `_c` entry points, §1), so a hard error would
   refuse the second of the two implementations this project exists to support.
   The warning names the consequence — the `_c` slots become
   `MPI_ERR_UNSUPPORTED_OPERATION` stubs — and the floor sits at MPI-3.0, below
   which the ABI's own surface stops being expressible.

   **The floor is tested, not just declared:** MPICH 3.1.4, which reports
   `MPI_VERSION 3` / `MPI_SUBVERSION 0`, passes all six tests at two ranks. It
   needs an older toolchain to build at all (gcc 9; gcc 11 and 13 both reject its
   own headers) and a newer CMake than Ubuntu 20.04 ships, which is worth knowing
   before S6 promises a distro matrix — our `cmake_minimum_required(3.20)` is
   above that release's 3.16.
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

**Ship the three consumption routes**, since a library nobody can consume portably
has not been delivered:

- **`mpicc`** and friends (`mpicxx`; `mpifort` deferring to mpif) — a compiler
  wrapper naming this prefix's `mpi.h` and `libmpi_abi`, with the rpath set so the
  produced executable starts without help. It must *not* name `libmpiwrapper`:
  MPI-5.0 §20.2.1 requires `mpi_abi` to be the sole direct MPI dependency of the
  application binary, and the wrapper is reached by `dlopen`.
- **CMake package files** — `mpi_abiConfig.cmake` exporting an imported target so
  `find_package(mpi_abi)` works, plus enough of a `FindMPI` shim that a consumer
  written against `find_package(MPI)` — HDF5, PETSc, nearly everyone — works
  unmodified.
- **pkg-config** — `mpi_abi.pc`, the route nothing else covers.

All three generated from one source of truth for flags, and CI must *use* each of them
to build and run a program rather than merely check that they parse. mpif's
`check-pkg-config.sh` is the precedent, and the reason is that `--libs` is worthless
if the executable it produces cannot start.

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

**Version choice, restated after S1.** An earlier draft said "Open MPI 4.1 fails
the MPI-4.0 minimum (no `_c` variants), so distro LTS packages do not serve".
That reason no longer holds: released Open MPI 5.0.x has no `_c` entry points
either (§1), so no Open MPI version clears the MPI-4.0 *expectation* and none is
excluded by it — the floor is MPI-3.0 and it is a hard error, everything above
it is a warning. What the versions are now chosen for is coverage, not
admissibility:

| | why this version |
|---|---|
| MPICH >= 4.0 | the only implementation that actually provides the `_c` surface, so the large-count half of the ABI is exercised at all |
| Open MPI >= 5.0 | sessions, and the current component architecture; 4.1 is *wrappable* and is one of the rows S1 ran on Linux, so it is a legitimate extra row rather than an excluded one |
| MPICH 3.1.4 | the MPI-3.0 floor itself, verified rather than declared (§9's check 1) |

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

   **It is a Linux-only oracle, which S1 established by trying it.** An
   ABI-implementing MPI declares its `MPI_*` weak (Open MPI's does, all 683 of
   them) and `libmpi_abi` declares them strong, so on macOS dyld's
   weak-definition coalescing binds the wrapper's outward calls back to us and the
   configuration is not merely neutralized but unsound — see §2. The wrapper now
   detects this at load and refuses with a diagnostic naming the cause, which is
   the correct outcome and not a fixable one: nothing in the two-level namespace
   overrides coalescing. On ELF `RTLD_DEEPBIND` and `dlmopen` both resolve it,
   because scope order there beats weak-vs-strong. So this oracle runs on the
   Linux rows of the matrix, and the macOS rows get its refusal as a test instead.
   The build option is `-DMPI_ABI_WRAP_ABI_IMPL=ON`, warned about by default.

   **It does not currently build against Open MPI's own ABI mode**, and the
   reason has nothing to do with the wrapper: `mpicc_abi`'s `mpi.h` does not
   declare `MPI_Fint`, so `internal.h`'s `sizeof(MPI_Fint) == sizeof(MPIABI_Fint)`
   assertion has nothing to name. Noticed in S2, confirmed to predate it (the
   same configuration fails identically at S1's commit), and left alone: it is a
   property of that header, and the right fix — probing for `MPI_Fint` the way
   the entry points are probed, or dropping the assertion where the Fortran
   interface is absent — belongs with whoever makes that oracle a CI row.

### Behavioural tests, in increasing cost

- **`mpiwrapper_selftest`** — in-process, single rank; it calls `PMPI_Init`
  directly, so CMake runs it under `mpiexec -n 1` by default and
  `-DMPI_ABI_TEST_USE_LAUNCHER=OFF` makes it a singleton. Every constant
  map round-trip, all 103 predefined handles in both directions (mpif's
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

MPICH >= 4.0 and Open MPI >= 5.0 as the primary rows, plus MPICH 3.1.4 as the
MPI-3.0 floor row and Open MPI 4.1 where a distro provides it (both were run in
S1; see the version table in §9 for why neither is excluded); gcc and clang;
Linux and macOS required, FreeBSD
via a VM on a Linux runner (mpif's precedent), Windows/mingw later.

**32-bit is load-bearing, not routine coverage.** ABI handles are pointer-sized, so
i386/arm32v7 is the only place the "no spare high bits for tagging" constraint of
§4.1 is visible. mpif already has Docker images for both.

**MVAPICH is worth a row, and a cheap one.** It is MPICH-derived, so its handle
values, error classes and status layout are MPICH's and the conversion tables are
already exercised; what it adds is a third *installation* shape — its own library
naming, its own `mpicc`, its own launcher — which is where §9's "all three
consumption routes must build and run a program" gets tested against something
nobody tuned it for. Same argument extends to the other MPICH derivatives (Intel
MPI, Cray MPICH) wherever one is available to CI.

### Gating

Our own tests, the MPICH C suite, the cross test, and the sanitizer/valgrind runs.

**Consumer integration** is the oracle that matters most and the one no in-house test
replaces. Each of the four covers something the others do not: **HDF5**'s parallel
driver for `MPI_File_*`, the bitmask `amode` and `datarep`; **PETSc** for datatype, op
and attribute breadth; **mpi4py** for the loader scenarios `dev/dlopen-probe` models
in miniature; **mpif** for the Fortran converters and status `f2c`/`c2f`. Building and
running each project's own suite against a wrapper is where omissions that all five
oracles pass will actually surface.

**mpif's own `test/` is deliberately not gating**, so the two projects' CI do not
become coupled. It is still the end-to-end composition proof and the only thing
that exercises the Fortran converters and the status `f2c`/`c2f` paths, so it
should be run before releases.

---

## 11. Sequencing

**Prototype before generator.** Hand-write a representative set of entry points
end-to-end, all crossing the vtable boundary, and get them passing against one MPI.
Only then write the generator, and require it to reproduce them.
Designing the generator before the shape of its output is known is the main way
this goes wrong.

The table below is the plan as written, and it names **sixteen** functions rather
than the "fifteen" the prose used to claim — `MPI_Comm_create_errhandler` and
`MPI_Comm_set_errhandler` share a row. What S1 delivered is below it.

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

**What S1 actually built**, since S2 is measured against it. **Twenty-nine** entry
points, **58 slots** — count them in `dev/s1-reference/mpiwrapper_vtable.h`,
which is the authority, and in `STAGES.md`'s S1 line. The sixteen above need thirteen more
to be *testable* at all: `MPI_Isend`/`MPI_Irecv` to have requests for
`MPI_Waitall`, `MPI_Comm_rank`, `MPI_Type_commit`/`_free`, `MPI_Comm_free`,
`MPI_Op_create` (which the `MPI_Allreduce` row assumes but does not name) and
`MPI_Op_free`, `MPI_File_close`, `MPI_Comm_f2c`, `MPI_Wtime`,
`MPI_Type_create_struct_c` for the `_c` pairing, and `MPI_Get_version` for the
bootstrap's behavioural probe. Nine of the twenty-nine are hand-written
(`src/mpiwrapper/handwritten.c`, and its header is the S1 `HAND_WRITTEN` ledger);
the other **twenty** are in `dev/s1-reference/wrappers.c`, written the way a generator
would write them — one `const` local per parameter, in parameter order, named after
the parameter with `abi_` dropped, and one body macro instantiated twice. Those
twenty are what S2 must reproduce.

**What S2 delivered.** The generator (`dev/generate.py`), the availability probe
(`dev/probe_impl.py`) and the reproduction check
(`dev/check_prototype.py`). All 688 entry points and all 1376 slots exist:
**473 generated, 120 in the ledger (ten with bodies), 95 deferred to S3**, and
`gen/report.txt` names every one. Of S1's 194 comparable items, **190 are
reproduced exactly and four are exempted with a reason** (§3). Green against
MPICH 4.3.1 at two ranks and Open MPI 5.0.10, 6.1.0a1 and 6.1.0a1-native as
singletons.

**What it was tested against.** MPICH 4.3.1 at two ranks, and Open MPI 5.0.6
(built from source) as a singleton — no Open MPI 5.0.x launcher works on macOS 26,
with or without any of this code, so the Open MPI column is one-rank coverage
until a working 5.x launcher or a native 6.x build is available. That is enough to
exercise every conversion in both directions, since a singleton call still crosses
the boundary twice; it is not enough for the two-rank message-passing paths, and
`test/README.md` says so rather than leaving it to be inferred from a green run.
The two implementations disagree about 13 predefined handles (98 mapped against
85), which is the kind of difference the maps exist for.

Four files are S1 stand-ins for generated output and left `src/` in S2, frozen
into `dev/s1-reference/` rather than deleted, because an exit check is only
worth something if it can fail *later*: `mpiwrapper_vtable.h`, `entrypoints.c`,
`wrappers.c` and `constants.c`. `constants.c` was produced mechanically
from `gen/include/mpiabi.h` rather than typed, because 103 predefined handles and
80 error classes (62 `MPI_ERR_*` plus 18 `MPI_T_ERR_*`; the header's 63rd
`MPI_ERR_*` is `MPI_ERR_LASTCODE`, a bound rather than a class) is exactly where
a typo survives review; the throwaway script that
emitted it is not committed, since S2's generator supersedes it. Everything else in
`src/` is permanent.

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
  §6.3. **The branch exists now**, on all eleven entry points with an inout
  request: one relaxed load and a compare against zero where the application
  never uses the routines that stage.
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

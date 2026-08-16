# MPI ABI wrapper — history

What was tried and abandoned, what a measurement overturned, and what each
implementation stage settled. One of three documents:

| | holds |
|---|---|
| `CODE.md` | what the repository contains now, and the number behind every claim |
| `NOTES.md` | the design, its reasons, and what is missing, broken or undecided |
| **`HISTORY.md`** | roads not taken, beliefs that were measured false, and the stage record |

**The rule for what lands here rather than in `NOTES.md`:** if a wrong answer is
still reachable from the current code — if changing one line of `bootstrap.c`
would reintroduce it — the reason lives in `NOTES.md`, because that is where
someone about to change that line will look. If it is a road the code no longer
touches, it lives here. `RTLD_GLOBAL` is the boundary case and it is in both:
`NOTES.md` §2 keeps the three reasons it is harmful, because the flag is one
character away in `bootstrap.c`; this file keeps the story of an earlier draft
recommending it.

**Why keep any of it.** Four separate results in this design came from probes
that would otherwise have been re-run, and at least three abandoned approaches
are the *first* thing a reader invents on meeting the problem — folding `PMPI`
onto one slot, converting arrays in place, `#ifdef`ing on the implementation's
own name for a constant. Each is recorded with what killed it, so the second
person to think of it spends a minute rather than a session.

This file is not maintained as a narrative of the project. It is a lookup table
for "was this tried?".

---

## 1. Approaches tried and abandoned

### 1.1 Conversions in `libmpi_abi`, with the implementation `dlsym`ed

The original architecture put the conversion layer in `libmpi_abi` itself. That
library then had to include the implementation's `mpi.h`, `#define` every one of
its ~700 function names aside to avoid colliding with the ABI functions it
defines, `dlsym` all 688, and obtain predefined handle values through a helper
library whose only purpose was to give them linker-visible names.

Moving the conversions behind an ABI-typed vtable deletes all of it: the wrapper
links the MPI library normally and calls `MPI_Send` as ordinary code. The cost
is one extra *direct* call per MPI call; the number of indirect calls is
unchanged at one. `NOTES.md` §2 has the resulting shape.

### 1.2 Routing `MPI_X` and `PMPI_X` to a single vtable slot

Cheaper — one slot per entry point instead of two, half the generated bodies — and wrong at
the second of the two levels where interposition can occur. With one slot, an
application calling `PMPI_Send` to bypass profiling still passes through a tool
interposed between `libmpiwrapper` and `libmpi`; it bypassed the ABI-level layer
only. `PMPI_X` means "the implementation with no profiling wrapper", and a
layered shim must not silently reintroduce one.

A second reason, nearly as good: two slots make the ledger 1:1, so "each entry
point has exactly one slot and one body" is a uniform invariant rather than a
2:1 mapping with a special case. Cost: +5.5 KB of vtable and a doubled emitted
body count, from one template per function differing only in the call target.

### 1.3 A configure probe for the shifted names, falling back to `MPI_X`

Proposed when the notes still believed `PMPI_*` might be missing somewhere. It
would have silently reintroduced exactly the defect 1.2 avoids: a fallback that
does not announce itself. Both names always exist and reach the same code when
nothing is interposed, so a link error naming the missing symbol is both the
better outcome and free. This is the worked example behind §5.9's rule — handle
it at run time when the degradation is observable, fail the build when it is
not.

### 1.4 `RTLD_GLOBAL` for the wrapper

An early draft recommended it, on the reasoning that the wrapper's dependencies
should be visible. It is actively harmful for three independent reasons, all of
which `NOTES.md` §2 keeps because the flag is one edit away. The one that decided
it is memory safety rather than correctness: the implementation's own internals
call MPI in places (Open MPI's ROMIO and io components), and a captured
`MPI_Recv` hands our 32-byte ABI status writer a 24-byte
`ompi_status_public_t`.

`RTLD_LOCAL` alone does not fix the capture either — `LOCAL`/`GLOBAL` controls
what the loaded object *exports*, not how its own references resolve. Isolation
is mandatory, not an optimization.

`dev/dlopen-probe/` measured all of it, on a mock of the three-library structure
with no MPI in it, on Linux (glibc 2.36, aarch64 and arm32v7) and macOS (arm64).
Three tests: the wrapper's own `MPI_Send` call (T1), the *implementation's
internal* `MPI_Send` call (T2, modelling ROMIO), and a second later-`dlopen`ed
plugin (T3, modelling mpi4py plus a second extension).

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
handed ABI-typed arguments. 32-bit matched 64-bit exactly.

**These are statements about the mock's loader behaviour, not about which
configurations are usable** — the mock has no MPI in it and therefore `dlopen`s
no components, which is exactly what makes its `dlmopen` row not generalize
(§1.5). The macOS `-flat_namespace` row is the one that proves the two-level
namespace is load-bearing rather than incidental.

### 1.5 `dlmopen(LM_ID_NEWLM)` as the isolation default, and as the sanitizer fallback

`dlmopen` isolates the mock in `dev/dlopen-probe/` perfectly, and it was the
designated fallback for the case where `RTLD_DEEPBIND` disturbs the sanitizers.
**It does not survive contact with a real MPI.** Both MPICH 4.1 and Open MPI 4.1
segfault in `MPI_Init`, in glibc's own loader:

```
add_to_global_resize (elf/dl-open.c:126)
_dl_open (".../pmix/pmix_mca_pcompress_zlib.so", mode=RTLD_LAZY|RTLD_GLOBAL, nsid=-2)
PMIx_Init  <-  PMPI_Init  <-  mpiwrapper_w_MPI_Init  <-  libmpi_abi MPI_Init
```

Every modern MPI loads its components with `dlopen`, and PMIx asks for
`RTLD_GLOBAL`; glibc cannot add to the global scope of a namespace created by
`dlmopen`, because that namespace has no main map. The failure is structural,
not one implementation's quirk.

Confirmed from the other side: **MPICH 3.1.4 runs fine under `dlmopen`** — all
tests pass — and it predates PMIx and loads no components at run time. So
`dlmopen` itself is not broken; it is unusable with any MPI that `dlopen`s
plugins, which is every current one. The mode stays selectable because it costs
one environment variable and a component-free MPI is a real configuration, but
nothing may be *planned* on it. `NOTES.md` §11 lists what S9 has instead.

An earlier `examples/mpi_abi_side.c` still *defaulted* to `dlmopen` on Linux
after `src/` had moved to `RTLD_DEEPBIND`; see §5 below.

### 1.6 An atomic acquire-load guard on the vtable pointer

The bootstrap was to be an idempotent lazy-init behind an acquire load, "so that
a plugin `dlopen`ed before the constructor runs still works". There is no such
window: anything that can call an entry point must link `libmpi_abi`, hence
depends on it, hence its own constructors run after ours, and a plugin
`dlopen`ed later is no exception because loading it loads `libmpi_abi` first.

`dev/dispatch-bench/` then measured what the guard would have cost, and it is
not the time:

| shape | trivial callee | instructions (gcc) | `.text` ×1376 |
|---|---|---|---|
| vtable via pointer | 1.084 ns | 5 | 22,252 B |
| pointer + atomic acquire + lazy branch | 1.630 ns | **23** | **95,436 B** |

A possible cold call to the initializer forces a stack frame into every entry
point. On a call that does real work the time is invisible (−0.15%), so this was
a code-size decision: 95 KB of text instead of 22 KB, for a window that cannot
occur.

### 1.7 Copying the vtable into our own storage

Measured against the single-pointer shape: 1.075 ns against 1.084 ns, and the
`.text` is byte-identical. The extra load is off the dependency chain, so an
out-of-order core issues it in parallel. Keeping the single pointer also leaves
8 bytes of writable function pointer in our data instead of 5.5 KB of it.

### 1.8 In-place conversion of array arguments

Contemplated wherever the standard allows it and there is space. It does not
survive contact with the standard, for four independent reasons — `const` may
mean physically read-only (`.rodata`), the same array may legally be in two MPI
calls at once, a nonblocking operation has no restore point, and every error
path would have to un-convert. `NOTES.md` §5.7 keeps the full argument, because
the tempting version of this ("just for the blocking calls") is still tempting.

The narrower idea of **expanding status arrays in place** is sound and was
rejected anyway: the implementation's 20/24-byte statuses do fit in the front of
the user's 32-byte array and expand backwards without overlap (destination of
element *i* starts at 32*i*; source of element *i*−1 ends at 24*i*). It saves one
allocation and costs a non-obvious invariant.

### 1.9 Staging an array whenever the element type is not the *same C type*

An earlier rule said to stage any array whose element type was not identical on
both sides, since the ABI's `MPI_Aint` and an implementation's may be distinct
types. That reasons from the language, and this project implements a **system
ABI**: what an ABI fixes is representation. `dev/type-identity/` measured size
and signedness identical for `MPI_Aint`, `MPI_Count`, `MPI_Offset` and
`MPI_Fint` in every implementation and platform tried; where the *spellings*
differ (glibc's `int64_t` is `long` while both MPIs spell `MPI_Count` as `long
long`) the cost is a cast. So displacement arrays and the `_c` forms' count
arrays pass straight through, and only the datatype arrays beside them are
staged.

### 1.10 A lifetime flag in the request map

§5.7 asked for a flag distinguishing a nonblocking operation's temporaries
(freed at completion) from a persistent one's (freed at `MPI_Request_free`).
The implementation already keeps that state: it nulls a nonblocking request at
completion and a persistent one at `MPI_Request_free`, and *not* at a persistent
request's mere completion, which is exactly the case an early free would
corrupt. So "release the pre-call handle wherever the post-call handle is null"
is the whole rule, emitted identically at every completion site, with no
per-entry state for the generator to get wrong.

### 1.11 A registry for generalized requests and datarep names

§5.6 closed with "Generalized requests and datarep names need the same
treatment". They do not: both families' registrars take an `extra_state`
argument that MPI hands back to every callback, so the `{user_fn, user_extra}`
pair *is* the registry. `MPI_T`'s events are the one family that genuinely
cannot do this, because `MPI_T_event_set_dropped_handler` has no `user_data`
parameter — which is why `toolevents.c` exists and `extrastate.c` serves
everything else.

### 1.12 A remembered thread level

The plan asked for "an initialization state machine and a thread level the
wrapper answers from". The state machine is real; the thread level had no
reader. Every shared table here is fixed-capacity and lock-free, so the wrapper
neither caps what the implementation provides nor degrades anything when the
level is low, and `MPI_Query_thread` answers from the implementation through the
ordinary conversion table. A stored copy would have been a field nobody reads.

### 1.13 A status validity marker and a synthesis fallback

An earlier status design added a magic word in `MPI_internal[0]` plus
reconstruction via `MPI_Status_set_elements_x` for statuses the implementation
had never touched. Both are unnecessary. In the generalized-request flow the
blob is always valid; for a genuinely uninitialized status, garbage out is what
the native implementation does for the same user error. So all 20 bytes hold the
blob and there is no second code path.

### 1.14 Forwarding the Fortran status converters, and `_toint`/`_fromint`

Two separate temptations, both wrong, and for opposite reasons.

`MPI_Status_c2f` **must not** be forwarded: it produces a valid status in the
implementation's layout, and MPICH's Fortran status puts the named fields at
indices 2, 3 and 4 where `MPI_F_SOURCE` — an ABI constant fixed at 0 by MPI-5.0
§20.4.3 — indexes a private byte. The four converters are a 32-byte copy and the
implementation is not involved.

`MPI_Comm_toint`/`_fromint` **must not** be forwarded either, and here the
reason is the standard rather than a layout: §20.4 puts the C–Fortran converters
*outside* the ABI, so forwarding `c2f`/`f2c` is free, while §20.4.5 pins
serialization to the ABI's own predefined values — "for all predefined handles,
the integer value must be the same as the values listed in Section A". So the 22
serialization forms never call the implementation, even where it has them.

### 1.15 A `_Static_assert` on `MPI_MAX_*`

Considered and rejected: both limits are known at the call site, so the
situation is handleable, and a build failure would block porting to a new MPI
over a contingency that may never fire for the functions that user needs.

### 1.16 A vtable handshake that accepts a smaller `size`

The contract used to say a wrapper may accept a smaller `size` and serve the
common prefix. That was never reachable: `abi_version`, `abi_subversion` and a
`layout_hash` taken over the *whole* slot list each demand exact equality, so a
caller built from a shorter slot list is refused before `size` is looked at. A
provision no input can reach is not forward compatibility but a story about it,
and it invited planning on growth the handshake does not implement. If additive
growth is ever wanted, the honest way is to hash only the slots up to `size`.

The `size` check itself stays, because it is the one thing the hash cannot see:
the hash is over the slot list's *text*, so a 32-bit `libmpi_abi` against a
64-bit `libmpiwrapper` hashes identically and differs in `sizeof`.

### 1.17 Name-tagging the wrapper library, and colocating the prefix

`libmpiwrapper-mpich-4.3.so`, so that several wrappers could share one prefix.
Dropped: on real HPC systems the things that make two wrappers incompatible are
mostly not in the name — loaded modules, compiler and its runtime, fabric
libraries, MPI build options — so the name gives a false sense of safety while
adding complexity. The version-and-layout handshake catches mismatches at load
time instead.

An earlier draft also said to install "ideally beside the MPI it wraps, so the
paths baked into it and the module environment stay together". That reads well
and is wrong: `mpi.h`, `mpicc` and — because MPI-5.0 §20.2.1 requires the ABI
library to be named `mpi_abi` — `libmpi_abi` are all names the wrapped MPI also
installs. Installing beside it does not risk a clash, it *is* one, on the two
files that decide which MPI a consumer compiles against. Worse, it breaks the
build that produced it: the no-self-wrapping check then refuses to configure
against that MPI, and the failure arrives at the next configure on someone
else's machine.

### 1.18 Sorted arrays for the predefined-handle reverse map

`dev/handle-map-bench/` measured the alternatives against the *real* 77
predefined MPICH datatype values and against Open MPI-shaped addresses (ns per
lookup):

| | mpich hot | mpich sweep | ompi hot | ompi sweep |
|---|---|---|---|---|
| perfect hash | **1.104** | **1.093** | **1.103** | **1.085** |
| open-addressing hash | 1.094 | 1.355 | 1.099 | 1.532 |
| sorted + binary search | 3.879 | 3.724 | 3.893 | 3.717 |
| sorted + interpolation search | **88.067** | 82.523 | 1.367 | 1.633 |

Binary search costs 3.4×: seven dependent, unpredictable comparisons.
**Interpolation search is a trap** on the distribution that actually occurs — it
assumes uniform keys, and MPICH's are one value at `0x0c000000`, a dense cluster
at `0x4c00xxxx`, and one at `0x8c000004`, where it degenerates toward a linear
scan with a floating-point divide per step and runs eighty times slower. On Open
MPI's uniform addresses it is fine and still no better than a hash.

The perfect hash also beats the open-addressing hash originally designed here,
because removing the probe loop removes the only data-dependent branch.

### 1.19 Three ways of asking what the implementation has

Decision 6 says "generated `#ifdef` stubs" without saying what the `#ifdef`
tests, and every answer that does not involve a configure test is wrong:

- **A version test under-reports.** Open MPI 5.0.10 announces MPI-3.1 and has
  sessions and partitioned communication. The gap is not small: `#if MPI_VERSION
  >= 4` would stub a couple of hundred entry points that are there.
- **`nm` over-reports.** It cannot see what the header provides as a macro,
  which is how Open MPI provides `MPI_Aint_add`.
- **`#ifdef` on the implementation's own name for a constant is worse than
  either, because it fails silently.** `#ifdef` sees macros and does not see
  enumerators, and implementations use both: MPICH spells `MPI_COMBINER_*` and
  `MPI_CART` as enumerators, Open MPI spells `MPI_THREAD_SINGLE`,
  `MPI_COMM_TYPE_SHARED` and `MPI_IDENT` that way. The `#ifdef` answers *no* for
  a constant that is right there, the case drops out of the conversion table,
  the default arm passes the ABI value through unmapped, and nothing fails.
  **Measured rather than argued:** MPICH 4.3.1 has `MPI_COMBINER_VALUE_INDEX` as
  `= 20` in an enum, and a draft that guarded it with `#ifdef` stopped
  translating that combiner without failing anything.

`dev/probe_impl.py` asks the compiler instead. `CODE.md` describes it.

### 1.20 Alternatives to a generator

- **A model writing each of the 688 by hand.** The artifact is ~40k lines nobody
  will re-read, every cross-cutting change is a 688-site edit, and the
  uniformity claim becomes unverifiable. The initial writing is not the cost;
  the second month is.
- **Porting mpif's `dev/mpiapi.jl`.** Its transferable content is ~200 lines of
  tables and its discipline; its 1150-line dispatch is Fortran-specific.
- **A hand-written per-function spec file** (wi4mpi's `functions.json`, 328 KB).
  Duplicates what `apis.json` already knows and must be re-maintained per MPI
  version.
- **X-macros / preprocessor wrappers.** Macros cannot do type-directed
  per-argument dispatch over varying arity without an X-macro list that *is* the
  table a generator would emit, with worse diagnostics.

### 1.21 A special case collapsing `MPI_ABI_` onto `MPIABI_`

The header renamer briefly special-cased `MPI_ABI_VERSION` →
`MPIABI_VERSION` (dropping the inner "ABI"), which reads better than
`MPIABI_ABI_VERSION`. It collides: `MPI_VERSION` — the MPI *standard* level, an
unrelated macro — renames to the same spelling. The plain uniform rule turned
out to be both simpler and correct, so the special case was removed rather than
kept.

---

## 2. Beliefs a measurement overturned

### 2.1 "`MPI_*` is a weak alias of `PMPI_*`"

Read off one pair of Linux builds and true there:

```
MPICH   libmpich.so   0x159d40 W MPI_Send   0x159d40 T PMPI_Send
OpenMPI libmpi.so     0x08d690 W MPI_Send   0x08d690 T PMPI_Send
```

The *binding* half does not generalize even on Linux. Ubuntu 24.04/aarch64:

```
MPICH 4.1     libmpich.so  0x10fe20 T MPI_Send  0x10fe20 T PMPI_Send   668 T, 0 W
Open MPI 4.1  libmpi.so    0x084630 T MPI_Send  0x084630 T PMPI_Send   432 T, 0 W
```

Same address, both *strong*. MPICH 3.1.4, from 2014, is the other way round —
385 weak `MPI_*` against 385 strong `PMPI_*` — so the original observation was
accurate for its time and MPICH changed.

And it does not generalize off ELF at all:

```
MPICH 4.3.1 (conda-forge)  libmpi.dylib   T MPI_Send,  no PMPI_ symbols at all
                           libpmpi.dylib  T PMPI_Send
Open MPI 5.0.10            libmpi.dylib   T MPI_Send  @0xae214
                                          T PMPI_Send @0x6fa1c   (two definitions)
```

So on macOS MPICH ships a separate profiling library and Open MPI compiles two
distinct functions rather than aliasing. What survives, and is all the design
needs, is that **both names exist and resolve to the same code when no tool is
interposed** — which is why the wrapper must link what `mpicc` links rather than
a library it names itself, since `-lmpi` alone leaves every `PMPI_*` undefined
on that MPICH.

### 2.2 "Weak `MPI_*` in the implementation predicts capture on macOS"

The tempting rule, and it is wrong in both directions:

| library | `MPI_*` | `PMPI_*` | wrapping it |
|---|---|---|---|
| MPICH 4.3.1 `libmpi.dylib` / `libpmpi.dylib` | 674 strong, 0 weak | 78 + 672 strong | works |
| Open MPI 5.0.6 `libmpi.dylib` | 472 strong, 0 weak | 468 strong | works |
| Open MPI 6.1.0a1 built natively | **0 strong, 698 weak** | 698 strong | **works anyway** |
| Open MPI 6.1.0a1 built `--enable-standard-abi` | **0 strong, 683 weak** | 683 strong | **refused at load** |

The last two differ only in a configure flag, both are all-weak, and only one
captures. Strong `MPI_*` is therefore *sufficient* and not necessary, and the
deciding factor is narrower than symbol binding — the Mach-O header flags
(`WEAK_DEFINES BINDS_TO_WEAK`) and install names are the same in both. It has
not been pinned down.

An earlier note said that if a native macOS build with weak `MPI_*` ever
appeared the wrapper would refuse it; native 6.1.0a1 is exactly that and wraps
correctly.

### 2.3 "`dladdr` answers the question that matters"

It does not, and this is the sharpest measurement in the project. Wrapping an
Open MPI built for the standard ABI on macOS:

```
dladdr(&MPI_Send) inside the wrapper   ->  the implementation   (correct)
the wrapper's actual call to MPI_Send  ->  libmpi_abi           (captured)
```

dyld coalesces weak definitions across images, so our *strong* `MPI_Send` wins
over the implementation's weak one even under a two-level namespace, while
taking the symbol's address still resolves through the namespace record. The
symptom is not recursion but **silent double execution**: the operation runs
once at each level and returns the right answer. It surfaced only because the
second pass tried to attach a staged temporary to a request already in the
table.

That is what the behavioural probe was added for. `NOTES.md` §2 has the
mechanism, including why the decoy table must fill *every* slot.

### 2.4 "The behavioural probe is a complete detector"

It is not. With the wrapper deliberately built `-flat_namespace` over Open MPI
6.1.0a1, capture is **partial**: `MPI_Send` and the datatype conversions recurse
until the stack is exhausted, while `MPI_Get_version` — the probe's own call —
resolves outward and reports the right answer. So the probe returns "not
captured" and the process dies later of recursion.

Both the address check and the behavioural probe are therefore *sound and
incomplete*: what they report is true, and what they miss is a partial capture
whose sampled call happens to bind outward. Sampling more entry points would
narrow the gap and not close it; closing it would need the loader to answer
"where does this call site bind", which neither `dladdr` nor `dlsym` will do.
`test/check_isolation.cmake` treats a crash as an acceptable outcome for this
reason, and fails only on a *successful* run of an unisolated wrapper.

### 2.5 "`MPI_Wtime` is legal before `MPI_Init`"

It is the obvious choice for a load-time probe and it is wrong: MPICH 4.3.1
answers a pre-`MPI_Init` `MPI_Wtime` with "Attempting to use an MPI routine
before initializing", and the standard's own list of what may be called before
initialization does not include it. The probe is `MPI_Get_version`. Two separate
drafts made this mistake, and it would have failed on MPICH and only on MPICH.

### 2.6 "A request handle identifies an operation"

`dev/request-identity/` measured it false on both implementations, two ways:

| | MPICH 4.3.1 | Open MPI 5.0.6 |
|---|---|---|
| `MPI_Isend` to `MPI_PROC_NULL` ×4 | `0x6c000001` ×4 | `0x1013f7920` ×4 |
| `MPI_Ibarrier` on `MPI_COMM_SELF` ×4 | `0x6c00000b` ×4 | `0x1013f7920` ×4 |
| `MPI_Ialltoallw` on `MPI_COMM_SELF` ×4 | distinct | distinct |
| a completed handle, then a new operation | value reused | value reused |

An operation already complete on return needs no per-operation object, and
neither implementation allocates one: MPICH has one built-in per operation kind,
Open MPI a single `ompi_request_empty` shared across all of them. The important
part is that `MPI_Ibarrier` is in that list — the shortcut is not confined to
point-to-point. `NOTES.md` §6.3 has what the table does about it.

### 2.6a "…but not to the staged family"

The third row above was read as saying so, and both `NOTES.md` §6.3 and §13.2
went on to record the refusal as a conformance bug that was *unreachable*.
Wrong, and reachable in Open MPI's default configuration: `probe-staged.c` and
`reproduce.c` (added 2026-08-16) get one shared `ompi_request_empty` from a
zero-work `MPI_Ialltoallw` and from a degree-0 `MPI_Ineighbor_alltoallw`, and
`MPI_ERR_INTERN` out of the wrapper for two of either. Open MPI answers *any*
libnbc collective with an empty schedule that way — `NBC_Schedule_request`, one
rule over all of them — and a rank that exchanges nothing with anybody has an
empty schedule.

The row is not wrong, it is unrepresentative: `probe.c` posts distinct buffers
and a count of 1, which is the one shape that builds a non-empty schedule on a
one-rank communicator. So this is the §2 pattern twice over — a claim about what
an implementation would not do, and a measurement whose *scope* was read as
wider than it was. `NOTES.md` §13.2 has the fix, now five mechanisms deep.

### 2.7 "An OUT array's stated maximum is safe to forward"

`dev/get-contents-extent/`: Open MPI 5.0.6 walks the whole of
`MPI_Type_get_contents`' `max_datatypes` and dereferences each entry it finds
there — which for an OUT parameter is whatever the caller's memory held — and
segfaults on a legal program with no wrapper involved. The wrapper passes the
envelope's count instead.

### 2.8 "`*resultlen` equals `strlen`"

Open MPI 5.0.6's `MPI_Get_library_version` reports **119 for a 118-character
string**, confirmed against it natively rather than through the wrapper. So the
contract is `string[*resultlen] == '\0'`, not the equality; asserting the
stronger reading would fail on a correct wrapper over a real MPI.

### 2.9 "An implementation returns error *classes*"

Measured natively, without the wrapper: MPICH 4.3.1 returns 604597509 for
`MPI_Comm_rank(MPI_COMM_NULL)`, 807037698 for a bad count and 874557989 for a
missing file — each an encoded error stack whose class comes back from
`MPI_Error_class`. Open MPI 5.0.6 returns the plain class in all three. The
generated switch's default arm had answered `MPI_ERR_OTHER` for anything it
could not name, which meant *every* MPICH error reached the application as
"other" and no application could ever be told `MPI_ERR_NO_SUCH_FILE`. Hence the
registry interns the implementation's codes as well as the application's.

### 2.10 "The ABI's single bitmask enum means a single bitmask mapper"

The ABI puts file modes (1..256) and window asserts (1024..16384) in one enum
with disjoint bits, and an earlier draft concluded a single mapper serves both
roles. It does not, in the out direction: Open MPI numbers its window asserts
`NOCHECK 1, NOPRECEDE 2, NOPUT 4, NOSTORE 8, NOSUCCEED 16`, which are *the same
bits* it gives `MPI_MODE_CREATE`, `RDONLY`, `WRONLY`, `RDWR` and
`DELETE_ON_CLOSE`. An implementation-side `1` is `CREATE` or `NOCHECK` depending
only on which parameter it came from.

MPICH keeps the two families disjoint, which is what made the single mapper
round-trip there — one more case where the same code passes every test on one
implementation and is wrong on the other.

### 2.11 Two benchmarks that reported confidently wrong numbers

Both from `dev/dispatch-bench/`, and both are why the session-hygiene rule says
to check a benchmark against its own disassembly:

- measuring each shape to completion let thermal drift land on one shape,
  reporting **+213%** for one extra load;
- building the vtable from a `static` in the same translation unit let the
  compiler devirtualize two shapes into direct calls. Only the disassembly
  caught it.

What is being measured at this layer is exactly what an optimizer most wants to
remove.

### 2.12 A watchdog's file descriptors

The `mpiexec` filter's watchdog held the stdout pipe runtests reads to EOF, so
*every* test appeared to take exactly the timeout — three tests at 195 seconds
each, all passing, which reads as a slow machine rather than as a bug.
Redirecting the watchdog's own descriptors is the fix.

### 2.13 "No Open MPI 5.0.x launcher works on macOS 26"

Written down in four places, and wrong in each. The evidence was good and the
inference was not: two independent 5.0.x prefixes — conda-forge's 5.0.10 for
osx-arm64 and a 5.0.6 built from source — failed identically and failed with no
part of this project loaded, while a 6.1.0a1 built from `main` worked. Two
implementations of a version failing where a third version succeeds does look
like a property of the version.

It was a property of the **machine**, which the control had not isolated
because both controls ran on the same one. The development laptop has the macOS
application firewall enabled; `socketfilterfw --listapps` carries an explicit
"Block incoming connections" entry for `build/mpi/*/bin/prte`, as it does for
every locally built binary that has ever listened, because an unsigned or
ad-hoc-signed executable earns a dialog no non-GUI session can answer and the
default is deny. PMIx 5 removed its Unix-socket transport, so all client-server
PMIx traffic is TCP, and PMIx drops loopback devices from its interface list by
default — so `prterun` advertised the `en0` address, the ranks connected there,
and the firewall killed each connection. 6.1.0a1's newer PMIx keeps loopback and
advertises `127.0.0.1`, which is the whole of the difference the version
comparison was reading.

The measurement that settles it needs no MPI at all: a thirty-line C program
that connects to its own listening socket gets its five bytes on `127.0.0.1`
and `ENOTCONN` on the `en0` address — the same `error 57:Socket is not
connected` that `prterun`'s handshake reports. `scripts/host-env.sh` carries the
three variables that keep everything on loopback — alongside the `FI_PROVIDER`
that MPICH on this host has needed all along, since both quirks are the same
one — and both 5.0.x prefixes then pass `test/`'s suite 13/13 with two ranks.

Two things this cost. The S1 results against Open MPI were taken as singletons
under `-DMPI_ABI_TEST_USE_LAUNCHER=OFF` — sound, and weaker than they needed to
be. And the MPICH suite's Open MPI row was moved into a Linux container for a
reason that had stopped being true; it is still there, now by choice.

The transferable part is not about firewalls. **Two implementations agreeing is
not a control when they share a host**, and "identically without any of this
project involved" only rules out this project — it does not rule out the
environment both runs were made in. The cheap way to get the real control is to
reproduce the failure with less and less of the stack until nothing recognizable
is left, which here took one C file.

### 2.14 "The Linux MPICH row runs at two ranks"

It ran at one, twice over, and every run was green. `CODE.md` §11 claimed
"**works**, 6/6, two ranks (Ubuntu 24.04, aarch64, Docker)" for MPICH, and
`ctest -V` in that container says otherwise on every one of the five black-box
tests:

```
7: abi_prototype_test: 1 rank
7: abi_prototype_test: 0 failure(s) across 1 ranks
7: abi_prototype_test: 1 rank
7: abi_prototype_test: 0 failure(s) across 1 ranks
```

Two copies of the same line, because `mpiexec -n 2` started two processes that
never found each other. **Ubuntu 24.04's MPICH cannot run a multi-rank job at
all.** Its `libmpi.so.12` is built `--with-pmix`, links `libpmix.so.2` and
imports `PMIx_Init` and no other PMI entry point; the `mpiexec.hydra` in the
same package is a PMI-1 server, and sets `PMI_FD`, `PMI_RANK` and `PMI_SIZE`
that this `libmpi` never reads. `PMIx_Init` finds no server, MPICH takes the
singleton path, and each process exits 0. Nothing in the image can bridge it:
Ubuntu 24.04 packages no PMIx launcher at all — no `prrte`, no `prte`, no
`prun` — and hydra's `-pmi-port` is still PMI-1. Debian 13's MPICH 4.2.1 links
no `libpmix`, so hydra's PMI-1 is what its `libmpi` speaks, and `mpiexec -n 2`
is a job; `run-linux-docker.sh` now sends the MPICH row there and leaves the
Open MPI row on Ubuntu 24.04, where `xfail-openmpi.txt`'s 168 lines are
calibrated against the 4.1.6 it ships.

**Why nothing caught it is the more useful half.** The five tests support one
rank as well as two, deliberately: that is what makes
`-DMPI_ABI_TEST_USE_LAUNCHER=OFF` a real configuration rather than a way of
skipping. A tolerated configuration is indistinguishable from a degraded one
when the only thing consulted is the test's own opinion — each singleton took
the supported one-rank path and passed, honestly, at a job size nobody asked
for. The tests were not wrong. Nothing had ever said what the *run* was
supposed to be.

So the build says it now. CMake puts the rank count it passed to the launcher
into each test's environment, and `test/expect_ranks.h` fails a test handed a
different one — including a test handed *more* ranks than asked for, since a
build configured for singletons is written for that path. Running a binary by
hand sets nothing and keeps the old tolerance.

The transferable part: **a range of supported configurations is not a
specification of the run.** Wherever a test says "1 or 2 is fine" and a harness
picks one, something has to record which was picked, or the harness silently
choosing the weaker one reads exactly like a pass. This is the same discipline
as `check-tap.py`'s both-directions gate — an expected failure that passes is a
failure — arrived at from the opposite end.

---

## 3. What each stage settled

Eight stages ran. The plan was ten sessions; S3 and S4 took two each and S7's
triage dominated its own. Each entry below records what the stage delivered and
what it settled that its plan did not name. Live rules have moved into
`NOTES.md`; the numbers here are as of that stage and are superseded by
`CODE.md`.

### S0 — Repository skeleton and header generation

Vendored the mpi-abi-stubs `mpi.h` and `apis.json`, applied `doc/mpi.h.patch`,
generated both headers. Exit check: both compile standalone, the extracted
entry-point list is exactly 688, `MPI_*`/`PMPI_*` symmetric.

**Four sharp edges in the renaming, none visible from the three rules.** All
four are live and `NOTES.md` §2 keeps them: `MPI_ABI_VERSION` renaming to
`MPIABI_ABI_VERSION` under the plain rule (§1.21 above is the special case that
failed); `MPIX_TYPECLASS_LOGICAL`, the one enumerator not spelled `MPI_*`;
`MPI_T_cb_safety`/`MPI_T_source_order`, whose tag and typedef are spelled
identically so rule 2 has to be broken for them; and
`MPI_Aint`/`MPI_Offset`/`MPI_Count`, whose two spellings rename to the *same*
name and so cannot be renamed line by line at all.

### S1 — The prototype, end to end

Planned at sixteen entry points, chosen for the argument class each one forces:

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

**Delivered 29, in 58 slots**, because that list is not testable on its own. The
thirteen additions are `MPI_Isend`/`MPI_Irecv` (to have requests for
`MPI_Waitall`), `MPI_Comm_rank`, `MPI_Type_commit`/`_free`, `MPI_Comm_free`,
`MPI_Op_create` (which the `MPI_Allreduce` row assumes but does not name) and
`MPI_Op_free`, `MPI_File_close`, `MPI_Comm_f2c`, `MPI_Wtime`,
`MPI_Type_create_struct_c` for the `_c` pairing, and `MPI_Get_version` for the
bootstrap's behavioural probe.

Nine were hand-written; the other twenty were written the way a generator
would write them — one `const` local per parameter, in parameter order, named
after the parameter with `abi_` dropped, and one body macro instantiated twice.
Those twenty are what S2 had to reproduce, and `dev/s1-reference/` is them.

Most of §2's loader findings are S1's: the macOS symbol shapes (§2.1 above), the
weak-coalescing capture (§2.3), the behavioural probe, and the per-platform
status table now in `CODE.md`. It also found that **no released Open MPI meets
the MPI-4.0 expectation** — 5.0.10 defines `MPI_VERSION 3`/`MPI_SUBVERSION 1`
and has no `_c` entry point at all — which turned the MPI-4.0 configure check
from a hard error into a warning and made decision 6's run-time-reporting stubs
load-bearing on day one rather than a contingency.

Two more renaming edges, both about what "the same type on both sides"
requires: **`MPI_Status` needed a struct tag**, which `doc/mpi.h.patch` now adds
as `MPI_ABI_Status` with a guarded definition and an unguarded typedef so either
include order works; and **`gen/include` holding `mpi.h` beside `mpiabi.h` is a
trap**, because putting our directory on the wrapper's include path makes
`<mpi.h>` find the ABI header, which *compiles and links* and then fails at run
time with the implementation rejecting `comm=0x101`.

### S2 — Generator core and the mechanical argument classes

Delivered `dev/generate.py`, `dev/probe_impl.py` and `dev/check_prototype.py`:
473 generated, 120 in the ledger, 95 deferred. Exit check: the generator
reproduces the S1 prototype, 190 of 194 items exactly.

It went past its plan because the exit check demanded it — buffer sentinels, the
mode bitmasks and the scalar out-status were on S3's list, and without them
`MPI_Send`, `MPI_File_open` and `MPI_Recv` could not be generated at all.

Two things it had to build that the plan did not name: **`dev/probe_impl.py`**
(§1.19 above is why nothing simpler works) and **`dev/check_prototype.py`** over
a frozen `dev/s1-reference/`, so that the exit check keeps running instead of
having been asserted once.

Three findings that are still live rules: **one implementation declaration
disagrees with the standard** — Open MPI 5.0.x declares `MPI_Pready_list(int
length, int partition_list[], MPI_Request)` where MPI-4.0 says `const int
array_of_partitions[]`, handled as a named `(routine, parameter)` entry rather
than a cast applied wherever a call fails to compile; **`MPI_IN_PLACE` is not in
`apis.json`**, so which sites accept it is a table, and it needs four sentinel
mappers rather than three because `MPI_Scatter` and `MPI_Scatterv` take it at a
*receive* buffer; and **two enum tags really are two types**, so the four
`MPI_T` forwarders that pass `MPI_T_cb_safety`/`MPI_T_source_order` are the only
place in `entrypoints.c` where a cast is correct.

### S3a — Arrays, statuses and lifetimes

518 generated, 118 in the ledger, 52 deferred. `MPI_Waitall` and
`MPI_Ialltoallw` left the ledger.

Settled: **the lifetime rules need no flag** (§1.10 above); **extents that
`apis.json` gives as `*` are not one class** — the communicator's size, two
different degrees, the last entry of an index array, a sum of degrees — each a
named entry, and the ones that must ask the implementation go through
`src/mpiwrapper/extents.c` *before* the call they serve, so a failure returns
the implementation's own error and nothing has been allocated; **four OUT arrays
are filled only partly** by the implementation, so converting the tail would
convert uninitialized elements and they carry an allocation extent and a smaller
conversion extent; **two arrays must not be read at all** when `MPI_IN_PLACE` is
at `sendbuf`, since MPI-5.0 §6.11 makes them ignored and a legal program may
pass null (measured: both implementations accept such a call with all three
null); **`MPI_Type_get_contents`' `max_datatypes` is not forwarded** (§2.7
above); and **`MPI_Group_range_incl`'s `ranges` crosses unconverted** although
`apis.json` calls the whole triplet a `RANK`, because the third column is a
*stride* and a stride of −1 would reach MPICH as −2.

### S3b — Keyvals, output strings, callbacks, `MPI_T`

565 generated, 118 in the ledger, 5 answered by `libmpi_abi` itself, **0
deferred** — and the zero is frozen, so a future `apis.json` carrying a class
the generator cannot place fails there rather than quietly emitting one more
stub.

Settled: **where the callback boundary falls**, as a rule rather than a list — a
callback-*typed parameter*, which puts `MPI_T_event_handle_free` in the ledger
and leaves the two `MPI_T_event_callback_*_info` calls generated, since a
`CALLBACK_SAFETY` is an enumerator and not a function; **MPI_T's null OUT
pointers**, which all five query functions permit and which make every converted
write-back conditional, hoisted into a declared local so the "no ABI-typed
parameter reaches the implementation call" assertion stays a grep;
**`src/mpiwrapper/keyvals.c`**, whose ABI-side value is ours to choose and so is
drawn from a base far above any predefined key, putting §5.6's collision beyond
reach by construction; **`src/mpiwrapper/toolobj.c`**, because `obj_handle`'s
class is not in its own argument list — it is whatever a prior `get_info`
reported in `bind`; and **MPI_T's six handle classes are deliberately not the
eleven's machinery**, since each has at most two predefined values and so is one
or two compares.

**A fifth thing, after the exit check had already passed:** S3b's own check
passed on every implementation and the *identity* configuration still would not
link. `libmpi_abi` from Open MPI main declares all 688 entry points and defines
683; the five it omits are the attribute functions MPI-2.0 deprecated and
MPI-3.0 deleted. `dev/probe_impl.py` asks the *compiler*, which is exact against
a conventional MPI whose header and library agree and not against one built from
the ABI header — and the failure mode is the worst available, since decision 6
promises a run-time report and instead the whole wrapper fails to link. A **link
stage** in the probe would fix the general case and is still the right answer for
any future entry point an implementation declares and does not define; it is not
implemented (`NOTES.md` §13.3). For these five the better fix was to stop
forwarding: `libmpi_abi` answers them itself in terms of their replacements, so
they now work over *any* implementation with the MPI-2 attribute interface.

### S4a — The converter face

70 of the ledger's 110 remaining bodies: the 44 handle converters, the four
status converters, the ten status-consuming functions, the ten output-string
buffers and the six `MPI_Abi_*` calls.

Settled: **`_toint`/`_fromint` are ABI-side rather than converters** and the
four status converters are a copy rather than a forward (§1.14 above); hence
**`src/mpiwrapper/serialize.c`**, because a dynamic ABI handle is a 64-bit
address on Open MPI and no cast to `int` recovers it — an append-only intern
table, used **even on MPICH where a cast would work**, so that one code path is
exercised on both implementations rather than the interesting one running only
where it is needed; and **`dev/probe_impl.py` now reads `src/mpiwrapper/` too**,
because a hand-written body's `MPIWRAPPER_HAVE_` guard was silently false while
the probe read only the generated sources. The rule it replaced the old one with
is the one that was actually meant: whatever guards, gets probed.

The exit check was the weakest in S4 and was weak in a specific way — a
round-trip through our own code in both directions passes even if the Fortran
integer we hand out is not the one the implementation's Fortran side would
recognise. Two of the tests written turned out stronger than that: `_toint` of a
predefined handle is compared against the ABI header's own constant, which
MPI-5.0 §20.4.5 requires and no round trip can detect, and a status converted to
the Fortran form and back is asked `MPI_Get_count`, which only succeeds if the
implementation's private bytes travelled with it. So the weakness applies to the
22 `_c2f`/`_f2c` forms alone; mpif in S8 is their oracle.

### S4b — The state the wrapper owns

The last 40: lifecycle, the 13 remaining callback registrars, the 12 buffer
attach/detach forms, the six dynamic error-code forms, the two spawn forms and
`MPI_Pcontrol`. `src/mpiwrapper/handwritten.c` disappeared with them, each of
S1's eight bodies joining the family file that now exists for it.

Settled: **the thread level is not wrapper state** (§1.12 above); **the
error-code registry has to intern the implementation's codes too** (§2.9);
**generalized requests and datareps need no registry** (§1.11);
**`MPI_BUFFER_AUTOMATIC` is emulated with a fixed 8 MiB buffer** where the
implementation lacks the mode, which is an approximation rather than the
standard's unbounded "buffer of sufficient size" and is named in
`gen/report.txt`; and **`MPI_Pcontrol`'s trailing arguments are dropped**, which
§14.2.2 permits because they belong to a profiling library above this one.

**Three rows had no oracle, and it is the implementations' doing rather than the
test's.** No implementation available supports a user datarep at all (MPICH's
ROMIO: "Read and Write datarep conversions are currently not supported by
MPI-IO"); MPICH declares every `MPI_T` event entry point and reports zero event
types while Open MPI 5.0.6 has no event interface, so no registration handle
exists to reach `toolevents.c`'s map with; and `MPI_Comm_spawn` *hangs* under
MPICH's hydra on macOS 26 with no wrapper involved — a fifteen-line C program
does the same, while the same program under MPICH in a Linux container returns a
clean error.

### S5 — Appendix A.3 cross-check

`dev/check-c-bindings.py`: 688 signatures matched against the standard's own C
bindings, parsed out of `doc/mpi50-report.pdf`, with eight named exemptions.

**The appendix numbers itself A.3, not A.2** — the report's own table of
contents gives A.2 to "Summary of the Semantics of all Op.-Related Routines".
Earlier drafts pointed at a section that does not hold what they said it holds.

Unlike mpif's Fortran precedent, a C signature has no `INTENT`/`::` list to
anchor a self-check on, so the parse validates itself differently: A.3 is a run
of signatures and nothing else once headings and margin numbers are stripped, so
its open- and close-paren counts must equal the signature count found, one pair
each. Array parameters are folded to their pointer-decay form before comparison,
which is what the language says rather than an approximation.

All eight exemptions are about *names* rather than semantics, because C bindings
carry no per-argument prose to disagree over. `NOTES.md` §10 lists them.

### S6 — Build, packaging, CI matrix

Three consumption routes, five legs in `ci-scripts/check-install.sh`, and MPI
installers for CI. All five legs pass on macOS against a distro Open MPI and on
Linux against MPICH 4.3.1 and Open MPI 5.0.6 built from source by this stage's
own scripts — the whole chain run end to end rather than inferred from the
scripts reading right.

**Decision 5's "falling back to a build-time path" had never been
implemented.** Before this stage `mpi_abi`'s only fallback was a bare filename,
which depends on the loader's default search path finding a same-named library —
true in the build tree by accident, false in an installed exclusive prefix.

**A macOS-specific finding:** `bin/mpicc` invokes `CMAKE_C_COMPILER` directly,
and on macOS that path can resolve to the Xcode toolchain's own compiler binary
rather than the `/usr/bin/cc` shim — which fails outright, `'stdio.h' file not
found`, without an explicit `-isysroot`. Measured directly: identical
invocation, only the flag differing.

**The `FindMPI` shim is two mechanisms.** CMake's own bundled `FindMPI` already
finds this project unassisted through its compiler-wrapper interrogation, which
`bin/mpicc.in` answers the way a real one would. The shipped
`cmake/FindMPI.cmake` adds only the case interrogation cannot reach: a consumer
that names neither `mpi_abi` nor a compiler.

**Visibility presets are not the whole story.** `-fvisibility`/
`C_VISIBILITY_PRESET` reach every symbol this project writes, but not the
handful the linker inserts into every shared object (`_init`, `_fini`, `_edata`,
`_end`, `__bss_start` on ELF) — measured on Linux, where they showed up as
extern and defined under plain `nm --defined-only --extern-only`, with no
visibility attribute able to touch them. The version script's `local: *;`
catch-all and ld64's `-exported_symbols_list` are what actually remove them.

**The MPI-3.0 floor row cost a real attempt.** Building MPICH 3.1.4 surfaced a
genuine coupling bug: `--disable-fortran`, added to save build time, silently
drops the *implementations* of `MPI_Type_create_f90_{real,complex,integer}` —
plain C entry points MPI-5.0 requires and that release's `mpi.h` declares
unconditionally — so the compile-only probe reports them available and only the
wrapper's link step fails. Both installers now run a stock configure with no
Fortran-disabling flag. What remains is the release's own limitation: 3.1.4's
configure rejects any Fortran compiler modern enough to warn rather than error
on a mismatched-argument call, gcc 11's and gcc 13's gfortran included, so that
row needs a pinned older toolchain when someone next picks it up.

### S7 — MPICH's C test suite

Around 900 programs that know only the MPI standard, run against this project
over MPICH and over Open MPI 4.1.6, through the wrapper's `mpicc` and
`libmpi_abi` and nothing else. **This is the first oracle in the project that
nothing in this repository wrote, and it found three conversion bugs no in-house
check could have seen.** All three are fixed; `NOTES.md` §5 carries the rules
they produced.

- **An attribute's *value* is a converted class, and no signature says so.**
  Every rule in §5 keys on a parameter's type; `MPI_Comm_get_attr` hands back a
  `void *` whose meaning is whatever the *keyval* says it is, and for five of
  the thirteen predefined keys that meaning is a family this library maps.
  `attr/baseattr2` asked for `MPI_HOST`, got MPICH's `MPI_PROC_NULL` (−1), and
  read it as the ABI's `MPI_ANY_SOURCE`; a window made by `MPI_Win_create`
  reported flavour 1, which is not one of the ABI's four. Nothing in the
  generator's assertions could see any of it — the emitted text passes every
  one, the call returns `MPI_SUCCESS`, and the frozen tallies are unmoved.
- **A single completion must not touch `status.MPI_ERROR`** (MPI-5.0 §3.2.5).
  The wrapper hands the implementation a status temporary of its own, so the
  implementation cannot honour the rule for it, and the field came back as
  whatever the temporary held. 13 tests caught it, and *our own*
  `abi_prototype_test` had asserted the wrong thing since S1.
- **`MPI_DISPLACEMENT_CURRENT` is a sentinel and was not translated.**
  `(MPI_Offset)-1` in the ABI and `-54278278` in ROMIO, so `MPI_File_set_view`
  with it returned `MPI_ERR_ARG`. It survived four stages because the existing
  sentinels are pointers, and those parameters already have a conversion point
  in the emitted body that a case can be added to; an `OFFSET` scalar is emitted
  as a bare passthrough local, so there was nothing to add a case *to*.

**And one finding that is bigger than a bug**, still open: an *erroneous*
argument the implementation would have diagnosed becomes a crash, because
conversion interposes a local. `NOTES.md` §13.3 carries it.

Three more things the plan did not name. **The suite tests us as a generic MPI,
and that is decided by a macro**: its configure compiles `return 1 + MPICH;` and
answers "Is the MPI derived from MPICH... no", so wrapping MPICH does not
quietly turn its own suite into a friendlier one. **The build failures the plan
expected are here for a different reason** than "a test reaches for MPICH
internals": they are the MPI-1 entry points MPI-3.0 *deleted* (`MPI_LB`,
`MPI_UB`, `MPI_Type_extent`, `MPI_Errhandler_create`), which an MPI-5.0 ABI
header does not declare, plus MPICH's QMPI — and they do not appear in the bulk
`make` at all, because `--enable-strictmpi` drops them from `noinst_PROGRAMS`
while leaving them in the testlists. **The suite decides whether the `threads`
directory exists by *running* an MPI program**, so a host where `MPI_Finalize`
fails for an unrelated reason silently drops 8 tests rather than failing;
`run-suite.sh` prints that decision, because a green run that covered less than
the last one is the failure mode a test count cannot show.

**It also put numbers on "not reclaimed".** `attr/fkeyvaltype` creates 32,768
keyvals in one process against 1024 slots, and `coll/nonblocking3` creates
several thousand user ops against 1024 trampolines and frees each one while a
nonblocking reduction using it is still in flight — which is exactly the case
§6.2's table calls "none observable". Neither is fixable by raising the number:
the op pool is *code*, and the keyval rule that forbids reclamation is the
standard's.

**The model split held exactly.** The harness took one pass; every hour after it
went into deciding whether a failure was ours, the test's assumption, or the
implementation's.

---

## 4. Counts that were wrong

Every one was one `grep` from being right, which is why `CODE.md` now carries an
authority column and the generator freezes each tally.

| claim | was | is | authority |
|---|---|---|---|
| deprecated entry points | 31 | **12** | `grep '; /\* deprecated' gen/include/mpi.h` |
| predefined handles | 104 | **103** | `PREDEF(...)` rows in `gen/mpiwrapper/constants.c` |
| error classes | 81 | **80** | 62 `MPI_ERR_*` + 18 `MPI_T_ERR_*`; `MPI_ERR_LASTCODE` is a bound |
| callback registration functions | 16 | **15**, then 16 again | §6.1's table, plus `MPI_T_event_handle_free` found on its own terms |
| hand-written set | ~50 | ~90, then **120** | §8's own list added up; then the generator's ledger |
| S1 prototype | 28 entry points, 56 slots, 19 generated-shape | **29 / 58 / 20** | `dev/s1-reference/mpiwrapper_vtable.h` |
| S1 stand-in files | "three" | **four** | they are named on the same line |
| planned prototype size | fifteen | **sixteen** | the table has sixteen functions in thirteen rows |
| mechanical entry points | "roughly 600", then 568 | **563** generated + 5 ABI-side | `gen/report.txt` |
| **vtable slots** | **1376** | **1366** | `gen/report.txt`; 683 × 2, the five deleted entry points having no slot |
| MPICH suite failures | 45, then 43 | **41** | `wc -l` on `ci-scripts/suite/xfail-mpich.txt` |
| Open MPI suite failures | 171 | **168** | ditto for `xfail-openmpi.txt` |

The 1376 line is the instructive one. It was right until S3b's follow-up gave
the five deleted entry points to `libmpi_abi`, and it stayed in eight places
afterwards — including a *generated* comment in
`gen/include/mpiwrapper_vtable.h`, directly above a struct with 1366 members,
because the generator interpolated the entry-point count where it wanted the
slot count. The exported-symbol count is still 1376 and always was, which is
what made the wrong number look right at every glance.

---

## 5. `examples/` drifts, and why that is the point

`examples/` holds narrated excerpts of each shape. They compile, and `check.sh`
compiles them — and compiling proved nothing about agreement in five of the six
divergences found so far:

1. the reverse-map tables were `static const uint64_t` arrays initialized from
   handle macros, which is not a constant expression on an implementation whose
   handles are addresses, so they do not compile against Open MPI at all;
2. the bitmask mapper needed splitting by role (§2.10 above);
3. the `dlopen` narration predated the `RTLD_LOCAL`-plus-isolation correction;
4. `mpiwrapper_convert.c` compared the ABI protocol version against
   `MPIABI_VERSION` (the MPI standard level, 5) instead of `MPIABI_ABI_VERSION`
   (the handshake version, 1) — it rejected every valid pairing and compiled
   cleanly, because both macros exist;
5. `mpi_abi_side.c` still *defaulted* to `dlmopen` on Linux, which `src/` never
   did and which is now known not to work with a real MPI at all;
6. the slot counts (§4 above).

The pattern — *compiles, therefore unchecked* — is why `src/` is the reference
and a shape that exists in both places is a second source of truth.

---

## 6. The documents this file replaces

Three documents were folded into the current three and deleted.

**`STAGES.md`** sequenced the work into ten stages and carried each stage's exit
check. Its durable content is §3 above (what each stage settled), `NOTES.md` §11
(the two stages that remain, and the model-choice principle), and `CODE.md` (the
tallies each stage left behind). Code comments that cited `STAGES.md S<n>` now
cite `HISTORY.md`.

**`REVIEW-BRIEF.md`** was written at the end of the design session that produced
`NOTES.md`, to hand over the one thing that dies with a session's context —
which claims were superseded, and when. The review it asked for ran in a fresh
session and found five contradictions, eight wrong counts, and five divergences
in `examples/`; its outcome is §4 and §5 above, and the contradictions
themselves are the abandoned approaches of §1. It also settled three things
immediately afterwards, all now in `NOTES.md` §2: the handshake requires exact
equality on all four fields, the comment over the map construction says what the
CAS actually guarantees, and the capture diagnostic advises the actual default.

Its closing advice is the reason this file exists, and it held: *anything
asserted rather than measured is where the next surprise comes from*, and *every
count should be checked against the artifact rather than against another
sentence*. A second review, run after S7, found the 1366/1376 split, the four
disagreeing suite counts, and three findings that had never reached the section
that owns the rule.

**`TODO.md`** survives as the maintainer's own open-questions list and is not
part of the three-document set.

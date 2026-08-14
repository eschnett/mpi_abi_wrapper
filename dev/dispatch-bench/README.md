# `dev/dispatch-bench/`

What the ABI side's dispatch costs per call, and which of the candidate shapes to
use. `libcallee` stands in for `libmpiwrapper` and owns the vtable; `libdispatch`
stands in for `libmpi_abi` and implements five dispatch shapes; `bench` calls them
across the DSO boundary in a loop.

```sh
./run.sh [iters] [reps]
docker build -t dispatch-bench . && docker run --rm dispatch-bench
```

Two callees: a trivial one (models `MPI_Wtime`, `MPI_Comm_rank`, a satisfied
`MPI_Test`) and one calibrated to ~320 ns (a plausible small `MPI_Send`). The
second is what decides whether any of this matters.

## Results

**Linux, gcc 12.2, aarch64** — ns per call, minimum of 9 interleaved bursts:

| shape | trivial callee | vs best | ~320 ns callee |
|---|---|---|---|
| direct call (linked, via PLT) | 1.350 | +0.275 | +0.00% |
| static function pointers | 1.085 | +0.010 | −1.32% |
| vtable copied into our storage | **1.075** | — | −0.82% |
| vtable via pointer | 1.084 | +0.009 | −0.86% |
| vtable via pointer + atomic acquire + lazy branch | **1.630** | +0.555 | −0.15% |

**macOS 26, Apple clang, arm64** — same ordering, but the atomic shape is not
penalised (1.089 vs 1.036), because `ldapr` is cheap on Apple silicon and clang
emits 11 instructions where gcc emits 23.

**Instructions per entry point** (aarch64):

| shape | gcc | clang |
|---|---|---|
| copied vtable / static pointers | 4 | 3 |
| vtable via pointer | 5 | 4 |
| atomic + lazy branch | **23** | 11 |

**`.text` for all 1376 entry points** (gcc -O2, measured with `size` on a generated
library, not extrapolated):

| shape | `.text` |
|---|---|
| atomic + lazy branch | **95,436 bytes** |
| vtable via pointer | 22,252 bytes |
| vtable copied in | 22,252 bytes |

## Conclusions

**1. Drop the atomic acquire and the lazy-init branch.** It is the one shape that
measures worse: +0.55 ns per trivial call under gcc, and 23 instructions instead of
4 because the possible cold call to the initializer forces a stack frame into every
entry point. Across 1376 entry points that is **95 KB of text instead of 22 KB** —
a 73 KB saving and a real instruction-cache argument. On a call that does actual
work it is invisible (−0.15%), so this is a code-size decision, not a latency one.

**2. Copying the vtable buys nothing measurable.** 1.075 against 1.084 ns is inside
the noise, and the `.text` is byte-identical. The extra dependent load is not on the
dependency chain — the vtable pointer is loop-invariant and the address of the slot
does not depend on the previous call's result — so an out-of-order core issues it in
parallel with everything else. Keep the single pointer: it is simpler, and it leaves
8 bytes of writable function pointer in our data rather than 5.5 KB of it.

**3. So the intuition about modern hardware was right, for the load, and wrong for
the branch.** The pointer chase really is free. The atomic-plus-branch is not free,
but what it costs is code size rather than time.

## Two ways this benchmark was wrong first

Recorded because both produced confident, plausible, wrong numbers.

**Measuring each shape to completion in turn.** Frequency and thermal drift then
lands entirely on whichever shape happened to be running. That version reported
`+213%` overhead for one extra load, and a dispatch shape that beat a direct call.
Fixed by measuring round-robin — one burst per shape per rep — and keeping each
shape's minimum.

**Letting the compiler see the vtable.** When the mock built the vtable from a
`static` in the same translation unit as the dispatch code, gcc and clang proved
each pointer could hold only one value and devirtualized two of the five shapes into
a bare `b callee_cheap`. Those rows then matched the direct call exactly, for
entirely the wrong reason. Only the disassembly showed it. Fixed by moving the
vtable into `libcallee` and fetching it through a cross-DSO call, which is what
actually happens with `dlsym`.

The lesson for the real project: any benchmark of this layer has to be checked
against its own disassembly, because the thing being measured is exactly the thing
an optimizer most wants to remove.

## Caveats

- One machine, one microarchitecture (Apple M-series, aarch64), two compilers. The
  instruction counts will differ on x86-64, where the atomic acquire load is a
  plain `mov` and the gap should narrow.
- The trivial-callee column is a lower bound on relative cost that no real MPI
  function reaches; `MPI_Wtime` is the closest and it still does a `clock_gettime`.
- Measured with everything hot in L1. A cold vtable line would cost a cache miss
  once, identically for every shape that uses one.

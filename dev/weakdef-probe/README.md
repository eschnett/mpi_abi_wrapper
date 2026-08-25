# `dev/weakdef-probe/`

What decides capture on macOS when the implementation's `MPI_*` symbols and
ours share names: their **weak/strong binding**, or their library's
**two-level/flat namespace**? `HISTORY.md` §2.2 measured one all-weak Open MPI
build wrapping correctly and another being refused, and left the discriminator
"not pinned down"; `HISTORY.md` §2.3 attributed the refusal to dyld's
weak-definition coalescing. This probe pins it down: **the namespace mode is
the whole story, and weak vs strong is irrelevant.** `HISTORY.md` §2.18.

```sh
./run.sh        # macOS only; dev/dlopen-probe/ owns the ELF side
```

## The structure

`abi.c` stands in for `libmpi_abi` (strong `foo`/`pfoo`, loaded first because
the application links it), `wrap.c` for `libmpiwrapper` (two-level, calls the
implementation by name), `impl.c` for the implementation — with the profiling
shape every real library ships: `MPI_X` (`foo`, weak or strong per variant)
whose body forwards to a strong `PMPI_X` (`pfoo`), plus an internal caller
modelling ROMIO-style traffic. The wrapper is `dlopen`ed `RTLD_LOCAL`, exactly
as `bootstrap.c` does on macOS.

| | question | correct answer |
|---|---|---|
| **T1** | the wrapper's `MPI_X` call | stays inside the implementation |
| **T2** | the wrapper's `PMPI_X` call | stays inside the implementation |
| **T3** | the implementation's own internal `MPI_X` call | stays inside the implementation |

## Results

**macOS 26.6.2, arm64, Apple clang 21** — only the implementation varies:

| implementation | T1 | T2 | T3 |
|---|---|---|---|
| `MPI_X` strong, two-level | OK | OK | OK |
| `MPI_X` weak, two-level | OK | OK | OK |
| `MPI_X` strong, `-flat_namespace` | **CAPTURED at the `PMPI_X` hop** | OK | **CAPTURED at `MPI_X`** |
| `MPI_X` weak, `-flat_namespace` | **CAPTURED at the `PMPI_X` hop** | OK | **CAPTURED at the `PMPI_X` hop** |

Three things fall out, each checkable in the table:

1. **A two-level implementation is never captured, weak or strong.** The
   wrapper's references bind by library ordinal, and dyld's weak-definition
   coalescing chooses among images that *have* weak definitions — our
   all-strong `libmpi_abi` never participates, so there is nothing for it to
   win. (`DYLD_PRINT_BINDINGS=1` prints the choice: "looking for weak-def
   symbol '_MPI_Send': using ... libmpi_abi.1.dylib" — the implementation's
   own definition, with ours loaded first.)
2. **A flat implementation is always captured, and at its own internal
   references.** A flat image resolves even calls to symbols it defines itself
   by global load order, and `libmpi_abi` loads first. The capture point is
   the shim's `MPI_X → PMPI_X` forward (T1's hop) — reached *through* a
   perfectly sound two-level bind from the wrapper — plus, when `MPI_X` is
   strong, the internal `MPI_X` call itself (T3). Note T2 stays OK in every
   row: the captured edges are all the *implementation's*, so no choice of
   call target on our side (`MPI_X` vs `PMPI_X`) avoids them, and we must
   export both names regardless.
3. **Weak definitions never decide anything here.** In the flat rows they
   merely move the capture point (weak-def references go through coalescing,
   whose participants exclude us; strong references go through flat lookup,
   which we win by load order).

## Against the real libraries

The refused configuration (`build/ompi-identity`, wrapping mpif's
`openmpi-gcc` standard-ABI build) reproduces cell for cell: that library is
missing the `TWOLEVEL` header flag (`otool -hv`; it is a gcc/libtool
`-flat_namespace` build), and under `DYLD_PRINT_BINDINGS=1` exactly **614 of
its references — every one a `PMPI_*` name — bind into our
`libmpi_abi.dylib`**, while all of the wrapper's own binds reach the
implementation. The behavioural probe's refusal is correct; its recorded
mechanism was not.

The counter-configuration is the ground truth: Open MPI main built
`--enable-standard-abi` *by this repository* (`build/mpi/ompi-main-prefix`) is
two-level with 657 weak `MPI_*`, and the wrapper built against it loads,
passes both isolation checks, and runs — zero implementation binds land in our
library. `NOTES.md` §10 oracle 5 has what its tests establish.

# `examples/`

Worked examples of the shapes the generator has to produce, and of the hand-written
runtime they call into. Illustrative, not part of the build — but they *do* compile,
and `check.sh` compiles them, because an example that has never been through a
compiler is a guess.

**Since S1 these are narrated excerpts, not the reference.** The reference is
`src/`, which compiles against two implementations and passes tests; a shape that
exists in both places is a second source of truth, and the tested one wins. Three
things here were wrong until S1 ran the real version — the reverse-map tables were
`static`, which does not compile against an implementation whose handles are
addresses; the bitmask mapper needed splitting by role; and the `dlopen` narration
predated the `RTLD_LOCAL`-plus-isolation correction — and all three are corrected
below.

| file | corresponds to | written by |
|---|---|---|
| `mpiabi.h` | `gen/include/mpiabi.h` | generator |
| `mpiwrapper_vtable.h` | `gen/include/mpiwrapper_vtable.h` | generator |
| `mpi_abi_side.c` | `src/mpi_abi/bootstrap.c` + `gen/mpi_abi/entrypoints.c` | hand + generator |
| `mpiwrapper_wrappers.c` | `gen/mpiwrapper/wrappers.c` | generator |
| `mpiwrapper_convert.c` | `src/mpiwrapper/*.c` | hand |

The headers are excerpts: six vtable slots of 688, and only the types and constants
the examples reference.

## Checking them

```sh
./check.sh
```

It needs two include directories and finds them itself if `mpif`'s build tree is
present:

- an **ABI** `mpi.h` (the mpi-abi-stubs header), for `mpi_abi_side.c`
- an **implementation** `mpi.h`, for the two wrapper files

Override with `ABI_INCLUDE=` and `MPI_INCLUDE=`. Both must be *different* headers:
compiling the wrapper side against the ABI header is the self-wrapping mistake that
`CMakeLists.txt` refuses at configure time.

## What each example is for

**`mpi_abi_side.c`** — that the ABI side contains no conversion at all. Every entry
point is one line, arguments pass through without a cast, and `MPI_X`/`PMPI_X` are
two definitions reaching two *different* slots. The interesting code is the
bootstrap: the environment variable, why the load must be `RTLD_LOCAL` **plus**
active isolation (`RTLD_DEEPBIND` or `dlmopen` on Linux, the two-level namespace on
macOS) and never `RTLD_GLOBAL`, why the binding mode defaults to `RTLD_LAZY`, and
the version/hash/size handshake. There is no lazy guard: the constructor-ordering
argument in `src/mpi_abi/bootstrap.c` explains why one is unnecessary and
`dev/dispatch-bench/` says what it would cost.

**`mpiwrapper_wrappers.c`** — the generated body shapes, in increasing difficulty:
`MPI_Send` (the base case, and why `dest` and `tag` go through different
functions), `MPI_Waitall` (staged arrays, `MPI_STATUSES_IGNORE`, and why write-back
must happen on the error path), `MPI_File_open` (an out-handle local and a bitmask),
`MPI_Error_string` (output-string staging and truncation).

**`mpiwrapper_convert.c`** — the runtime: the dense ABI→implementation switch, the
initialization-time reverse map and why it cannot be a switch, the rank/tag split,
the status blob with its `_Static_assert`s, the bitmask decomposition, the staging
helper, and the op trampoline pool with the one place a slot may be released.

The full reasoning for every choice is in `../NOTES.md`; the examples carry only
the reasoning you need while reading the code in front of you.

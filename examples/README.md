# `examples/`

Worked examples of the shapes the generator has to produce, and of the hand-written
runtime they call into. Illustrative, not part of the build — but they *do* compile,
and `check.sh` compiles them, because an example that has never been through a
compiler is a guess.

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
point is one line, arguments pass through without a cast, and `MPI_*`/`PMPI_*` are
two definitions reaching one slot. The interesting code is the bootstrap: the
environment variable, `RTLD_NOW | RTLD_GLOBAL` and why, the version/hash/size
handshake, and why there is both a constructor *and* a lazy guard.

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
